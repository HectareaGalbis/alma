
#include "operators.hpp"
#include "../alma.hpp"
#include "../debug.hpp"
#include "../object/cons.hpp"
#include "../object/symbol.hpp"
#include "emitter.hpp"

/*
C operators. Every operation is written between parentheses, so the precedence of C
does not matter.

  (+ a b ...)  (- a b ...)  (* a b ...)  (/ a b ...)  (% a b ...)
  (+ a)  (- a)                                     unary plus and minus
  (* p)  (& x)  (aref a i)                         *p, &x, a[i]
  (== a b)  (!= a b)  (< a b)  (> a b)  (<= a b)  (>= a b)
  (and a b ...)  (or a b ...)  (not a)             &&, ||, !
  (bit-and a b ...)  (bit-or a b ...)  (bit-xor a b ...)  (bit-not a)  (<< a b)  (>> a b)
  (= x v)  (+= x v)  (-= x v)  (*= x v)  (/= x v)  (%= x v)  (<<= x v)  (>>= x v)
  (bit-and= x v)  (bit-or= x v)  (bit-xor= x v)
  (++ x)  (-- x)  (post++ x)  (post-- x)
  (? c a b)  (comma a b ...)  (sizeof type-or-expression)  (cast type v)
*/

using Arguments = std::vector<ObjectRef<Object>>;

static std::string type_string(const Type& type)
{
    return type.to_string("");
}

static std::unique_ptr<Type> emit(const ObjectRef<Object>& obj, ObjectRef<Environment> environment,
    Declarations& declarations, std::stringstream& output)
{
    return Alma::alma.emit_expression(obj, environment, declarations, output);
}

static void check_arguments(const std::string& name, const Arguments& arg_list, size_t min, size_t max)
{
    if (min == max)
        aassert(arg_list.size() == min,
            "'" << name << "' expects " << min << " arguments but received " << arg_list.size());
    aassert(arg_list.size() >= min,
        "'" << name << "' expects at least " << min << " arguments but received " << arg_list.size());
    aassert(arg_list.size() <= max,
        "'" << name << "' expects at most " << max << " arguments but received " << arg_list.size());
}

static const size_t many = SIZE_MAX;

[[noreturn]] static void invalid_operand(const std::string& name, const Type& type)
{
    athrow("Invalid operand for '" << name << "': " << type_string(type));
}

[[noreturn]] static void invalid_operands(const std::string& name, const Type& a, const Type& b)
{
    athrow("Invalid operands for '" << name << "': " << type_string(a) << " and " << type_string(b));
}

/** Pointer to an object type, so it can be used in pointer arithmetic or be dereferenced */
static bool is_object_pointer(const Type& type)
{
    if (!type.is_pointer())
        return false;
    std::unique_ptr<Type> pointed = pointee(type);
    return !is_void(*pointed) && !pointed->is_function();
}

static std::optional<std::string> symbol_name(const ObjectRef<Object>& obj)
{
    if (!Alma::alma.symbolp(obj))
        return std::nullopt;
    return obj.as<Symbol>()->get_name();
}

/** Objects that designate a place in memory: variables, (* p) and (aref a i) */
static bool is_lvalue(const ObjectRef<Object>& obj, const Declarations& declarations)
{
    if (std::optional<std::string> name = symbol_name(obj))
        return declarations.contains(*name) && !declarations.get(*name).getType()->is_function();
    if (!Alma::alma.consp(obj))
        return false;
    auto [list, tail] = obj.as<Cons>()->to_list();
    if (!Alma::alma.null(tail))
        return false;
    std::optional<std::string> name = symbol_name(list[0]);
    if (name && ObjectRef<Symbol>(list[0])->has_emitter())
        return (*name == "*" && list.size() == 2) || (*name == "aref" && list.size() == 3);

    // Access to members: (variable member...) or (expression member...)
    if (name) {
        if (!declarations.contains(*name))
            return false;
        const Type* type = declarations.get(*name).getType();
        return !type->is_function() && !(type->is_pointer() && dynamic_cast<const FunctionType*>(type));
    }
    return is_lvalue(list[0], declarations);
}

static void check_modifiable(const std::string& name, const ObjectRef<Object>& obj, const Type& type,
    const Declarations& declarations)
{
    aassert(is_lvalue(obj, declarations) && !type.is_array() && !type.is_function(),
        "The operand of '" << name << "' must be modifiable but found " << Alma::alma.to_string(obj));
}

// -----------------------------------------------------------------------------
// Types of the binary operations

static std::unique_ptr<Type> additive_type(const std::string& name, bool plus, const Type& a, const Type& b)
{
    std::unique_ptr<Type> decayed_a = a.decay();
    std::unique_ptr<Type> decayed_b = b.decay();
    if (is_arithmetic(*decayed_a) && is_arithmetic(*decayed_b))
        return arithmetic_conversion(*decayed_a, *decayed_b);
    if (is_object_pointer(*decayed_a) && is_integer(*decayed_b))
        return decayed_a;
    if (plus && is_integer(*decayed_a) && is_object_pointer(*decayed_b))
        return decayed_b;
    if (!plus && is_object_pointer(*decayed_a) && *decayed_a == *decayed_b)
        return std::make_unique<ValueType>("ptrdiff_t");
    invalid_operands(name, a, b);
}

static std::unique_ptr<Type> arithmetic_type(const std::string& name, bool integer_only, const Type& a, const Type& b)
{
    bool valid = integer_only ? is_integer(a) && is_integer(b) : is_arithmetic(a) && is_arithmetic(b);
    if (!valid)
        invalid_operands(name, a, b);
    return arithmetic_conversion(a, b);
}

// -----------------------------------------------------------------------------

class EmitterOperator : public Emitter {
protected:
    std::string op;

public:
    EmitterOperator(const std::string& _name, const std::string& _op)
        : Emitter(_name)
        , op(_op)
    {
    }
};

/** + and -: unary or a left to right sequence of additions or subtractions */
class EmitterAdditive : public EmitterOperator {
public:
    using EmitterOperator::EmitterOperator;

    std::unique_ptr<Type> emit_expression(ObjectRef<Object> self [[maybe_unused]], const Arguments& arg_list,
        ObjectRef<Environment> environment, Declarations& declarations, std::stringstream& output) override
    {
        check_arguments(get_name(), arg_list, 1, many);
        output << "(";
        if (arg_list.size() == 1) {
            output << op;
            std::unique_ptr<Type> type = emit(arg_list[0], environment, declarations, output);
            output << ")";
            if (!is_arithmetic(*type))
                invalid_operand(get_name(), *type);
            return integer_promotion(*type);
        }
        std::unique_ptr<Type> type = emit(arg_list[0], environment, declarations, output);
        for (size_t i = 1; i < arg_list.size(); i++) {
            output << " " << op << " ";
            std::unique_ptr<Type> next = emit(arg_list[i], environment, declarations, output);
            type = additive_type(get_name(), op == "+", *type, *next);
        }
        output << ")";
        return type;
    }
};

/** *: dereference with one argument or a sequence of multiplications */
class EmitterStar : public EmitterOperator {
public:
    EmitterStar()
        : EmitterOperator("*", "*")
    {
    }

    std::unique_ptr<Type> emit_expression(ObjectRef<Object> self [[maybe_unused]], const Arguments& arg_list,
        ObjectRef<Environment> environment, Declarations& declarations, std::stringstream& output) override
    {
        check_arguments(get_name(), arg_list, 1, many);
        output << "(";
        if (arg_list.size() == 1) {
            output << "*";
            std::unique_ptr<Type> type = emit(arg_list[0], environment, declarations, output)->decay();
            output << ")";
            if (!type->is_pointer() || is_void(*pointee(*type)))
                invalid_operand(get_name(), *type);
            return pointee(*type);
        }
        std::unique_ptr<Type> type = emit(arg_list[0], environment, declarations, output);
        for (size_t i = 1; i < arg_list.size(); i++) {
            output << " * ";
            std::unique_ptr<Type> next = emit(arg_list[i], environment, declarations, output);
            type = arithmetic_type(get_name(), false, *type, *next);
        }
        output << ")";
        return type;
    }
};

/** /, % and the bitwise and, or and xor: a left to right sequence of operations */
class EmitterArithmetic : public EmitterOperator {
private:
    bool integer_only;

public:
    EmitterArithmetic(const std::string& _name, const std::string& _op, bool _integer_only)
        : EmitterOperator(_name, _op)
        , integer_only(_integer_only)
    {
    }

    std::unique_ptr<Type> emit_expression(ObjectRef<Object> self [[maybe_unused]], const Arguments& arg_list,
        ObjectRef<Environment> environment, Declarations& declarations, std::stringstream& output) override
    {
        check_arguments(get_name(), arg_list, 2, many);
        output << "(";
        std::unique_ptr<Type> type = emit(arg_list[0], environment, declarations, output);
        for (size_t i = 1; i < arg_list.size(); i++) {
            output << " " << op << " ";
            std::unique_ptr<Type> next = emit(arg_list[i], environment, declarations, output);
            type = arithmetic_type(get_name(), integer_only, *type, *next);
        }
        output << ")";
        return type;
    }
};

/** << and >> */
class EmitterShift : public EmitterOperator {
public:
    using EmitterOperator::EmitterOperator;

    std::unique_ptr<Type> emit_expression(ObjectRef<Object> self [[maybe_unused]], const Arguments& arg_list,
        ObjectRef<Environment> environment, Declarations& declarations, std::stringstream& output) override
    {
        check_arguments(get_name(), arg_list, 2, 2);
        output << "(";
        std::unique_ptr<Type> a = emit(arg_list[0], environment, declarations, output);
        output << " " << op << " ";
        std::unique_ptr<Type> b = emit(arg_list[1], environment, declarations, output);
        output << ")";
        if (!is_integer(*a) || !is_integer(*b))
            invalid_operands(get_name(), *a, *b);
        return integer_promotion(*a);
    }
};

/** ==, !=, <, >, <= and >= */
class EmitterComparison : public EmitterOperator {
public:
    using EmitterOperator::EmitterOperator;

    std::unique_ptr<Type> emit_expression(ObjectRef<Object> self [[maybe_unused]], const Arguments& arg_list,
        ObjectRef<Environment> environment, Declarations& declarations, std::stringstream& output) override
    {
        check_arguments(get_name(), arg_list, 2, 2);
        output << "(";
        std::unique_ptr<Type> a = emit(arg_list[0], environment, declarations, output);
        output << " " << op << " ";
        std::unique_ptr<Type> b = emit(arg_list[1], environment, declarations, output);
        output << ")";
        if (!(is_arithmetic(*a) && is_arithmetic(*b)) && !are_compatible_pointers(*a, *b))
            invalid_operands(get_name(), *a, *b);
        return std::make_unique<ValueType>("int");
    }
};

/** && and ||: a sequence of scalar operands */
class EmitterLogical : public EmitterOperator {
public:
    using EmitterOperator::EmitterOperator;

    std::unique_ptr<Type> emit_expression(ObjectRef<Object> self [[maybe_unused]], const Arguments& arg_list,
        ObjectRef<Environment> environment, Declarations& declarations, std::stringstream& output) override
    {
        check_arguments(get_name(), arg_list, 2, many);
        output << "(";
        for (size_t i = 0; i < arg_list.size(); i++) {
            if (i > 0)
                output << " " << op << " ";
            std::unique_ptr<Type> type = emit(arg_list[i], environment, declarations, output);
            if (!is_scalar(*type))
                invalid_operand(get_name(), *type);
        }
        output << ")";
        return std::make_unique<ValueType>("int");
    }
};

/** !, ~ */
class EmitterUnary : public EmitterOperator {
private:
    bool integer_only;

public:
    EmitterUnary(const std::string& _name, const std::string& _op, bool _integer_only)
        : EmitterOperator(_name, _op)
        , integer_only(_integer_only)
    {
    }

    std::unique_ptr<Type> emit_expression(ObjectRef<Object> self [[maybe_unused]], const Arguments& arg_list,
        ObjectRef<Environment> environment, Declarations& declarations, std::stringstream& output) override
    {
        check_arguments(get_name(), arg_list, 1, 1);
        output << "(" << op;
        std::unique_ptr<Type> type = emit(arg_list[0], environment, declarations, output);
        output << ")";
        if (integer_only) {
            if (!is_integer(*type))
                invalid_operand(get_name(), *type);
            return integer_promotion(*type);
        }
        if (!is_scalar(*type))
            invalid_operand(get_name(), *type);
        return std::make_unique<ValueType>("int");
    }
};

/** = */
class EmitterAssign : public EmitterOperator {
public:
    EmitterAssign()
        : EmitterOperator("=", "=")
    {
    }

    std::unique_ptr<Type> emit_expression(ObjectRef<Object> self [[maybe_unused]], const Arguments& arg_list,
        ObjectRef<Environment> environment, Declarations& declarations, std::stringstream& output) override
    {
        check_arguments(get_name(), arg_list, 2, 2);
        output << "(";
        std::unique_ptr<Type> target = emit(arg_list[0], environment, declarations, output);
        output << " = ";
        std::unique_ptr<Type> value = emit(arg_list[1], environment, declarations, output);
        output << ")";
        check_modifiable(get_name(), arg_list[0], *target, declarations);
        aassert(is_assignable(*target, *value),
            "Cannot assign a value of type " << type_string(*value) << " to " << Alma::alma.to_string(arg_list[0])
                                             << " of type " << type_string(*target));
        return target;
    }
};

/** +=, -=, *=, /=, %=, <<=, >>=, &=, |= and ^= */
class EmitterCompoundAssign : public EmitterOperator {
public:
    enum class Kind {
        Additive,
        Arithmetic,
        Integer
    };

private:
    Kind kind;

public:
    EmitterCompoundAssign(const std::string& _name, const std::string& _op, Kind _kind)
        : EmitterOperator(_name, _op)
        , kind(_kind)
    {
    }

    std::unique_ptr<Type> emit_expression(ObjectRef<Object> self [[maybe_unused]], const Arguments& arg_list,
        ObjectRef<Environment> environment, Declarations& declarations, std::stringstream& output) override
    {
        check_arguments(get_name(), arg_list, 2, 2);
        output << "(";
        std::unique_ptr<Type> target = emit(arg_list[0], environment, declarations, output);
        output << " " << op << " ";
        std::unique_ptr<Type> value = emit(arg_list[1], environment, declarations, output);
        output << ")";
        check_modifiable(get_name(), arg_list[0], *target, declarations);

        bool valid;
        switch (kind) {
        case Kind::Additive:
            valid = (is_arithmetic(*target) && is_arithmetic(*value))
                || (is_object_pointer(*target) && is_integer(*value));
            break;
        case Kind::Arithmetic:
            valid = is_arithmetic(*target) && is_arithmetic(*value);
            break;
        default:
            valid = is_integer(*target) && is_integer(*value);
            break;
        }
        if (!valid)
            invalid_operands(get_name(), *target, *value);
        return target;
    }
};

/** ++ and --, prefix or postfix */
class EmitterIncrement : public EmitterOperator {
private:
    bool prefix;

public:
    EmitterIncrement(const std::string& _name, const std::string& _op, bool _prefix)
        : EmitterOperator(_name, _op)
        , prefix(_prefix)
    {
    }

    std::unique_ptr<Type> emit_expression(ObjectRef<Object> self [[maybe_unused]], const Arguments& arg_list,
        ObjectRef<Environment> environment, Declarations& declarations, std::stringstream& output) override
    {
        check_arguments(get_name(), arg_list, 1, 1);
        output << "(";
        if (prefix)
            output << op;
        std::unique_ptr<Type> type = emit(arg_list[0], environment, declarations, output);
        if (!prefix)
            output << op;
        output << ")";
        check_modifiable(get_name(), arg_list[0], *type, declarations);
        if (!is_arithmetic(*type) && !is_object_pointer(*type))
            invalid_operand(get_name(), *type);
        return type;
    }
};

/** & */
class EmitterAddress : public EmitterOperator {
public:
    EmitterAddress()
        : EmitterOperator("&", "&")
    {
    }

    std::unique_ptr<Type> emit_expression(ObjectRef<Object> self [[maybe_unused]], const Arguments& arg_list,
        ObjectRef<Environment> environment, Declarations& declarations, std::stringstream& output) override
    {
        check_arguments(get_name(), arg_list, 1, 1);
        output << "(&";
        std::unique_ptr<Type> type = emit(arg_list[0], environment, declarations, output);
        output << ")";
        aassert(is_lvalue(arg_list[0], declarations) || type->is_function(),
            "Cannot take the address of " << Alma::alma.to_string(arg_list[0]));
        return pointer_to(*type);
    }
};

/** a[i] */
class EmitterSubscript : public Emitter {
public:
    EmitterSubscript()
        : Emitter("aref")
    {
    }

    std::unique_ptr<Type> emit_expression(ObjectRef<Object> self [[maybe_unused]], const Arguments& arg_list,
        ObjectRef<Environment> environment, Declarations& declarations, std::stringstream& output) override
    {
        check_arguments(get_name(), arg_list, 2, 2);
        std::unique_ptr<Type> array = emit(arg_list[0], environment, declarations, output)->decay();
        output << "[";
        std::unique_ptr<Type> index = emit(arg_list[1], environment, declarations, output);
        output << "]";
        if (!is_object_pointer(*array) || !is_integer(*index))
            invalid_operands(get_name(), *array, *index);
        return pointee(*array);
    }
};

/** c ? a : b */
class EmitterConditional : public Emitter {
public:
    EmitterConditional()
        : Emitter("?")
    {
    }

    std::unique_ptr<Type> emit_expression(ObjectRef<Object> self [[maybe_unused]], const Arguments& arg_list,
        ObjectRef<Environment> environment, Declarations& declarations, std::stringstream& output) override
    {
        check_arguments(get_name(), arg_list, 3, 3);
        output << "(";
        std::unique_ptr<Type> condition = emit(arg_list[0], environment, declarations, output);
        output << " ? ";
        std::unique_ptr<Type> a = emit(arg_list[1], environment, declarations, output)->decay();
        output << " : ";
        std::unique_ptr<Type> b = emit(arg_list[2], environment, declarations, output)->decay();
        output << ")";

        if (!is_scalar(*condition))
            invalid_operand(get_name(), *condition);
        if (is_arithmetic(*a) && is_arithmetic(*b))
            return arithmetic_conversion(*a, *b);
        if (*a == *b)
            return a;
        if (are_compatible_pointers(*a, *b))
            return std::make_unique<ValueType>("void", std::vector<Specifier> { Pointer {} });
        invalid_operands(get_name(), *a, *b);
    }
};

/** a, b, ... */
class EmitterComma : public Emitter {
public:
    EmitterComma()
        : Emitter("comma")
    {
    }

    std::unique_ptr<Type> emit_expression(ObjectRef<Object> self [[maybe_unused]], const Arguments& arg_list,
        ObjectRef<Environment> environment, Declarations& declarations, std::stringstream& output) override
    {
        check_arguments(get_name(), arg_list, 1, many);
        output << "(";
        std::unique_ptr<Type> type;
        for (size_t i = 0; i < arg_list.size(); i++) {
            if (i > 0)
                output << ", ";
            type = emit(arg_list[i], environment, declarations, output);
        }
        output << ")";
        return type;
    }
};

/** sizeof(type) or sizeof(expression). A declared symbol, a call or the use of an emitter is an
    expression; anything else is a type. */
class EmitterSizeof : public Emitter {
public:
    EmitterSizeof()
        : Emitter("sizeof")
    {
    }

    std::unique_ptr<Type> emit_expression(ObjectRef<Object> self [[maybe_unused]], const Arguments& arg_list,
        ObjectRef<Environment> environment, Declarations& declarations, std::stringstream& output) override
    {
        check_arguments(get_name(), arg_list, 1, 1);
        const ObjectRef<Object>& arg = arg_list[0];

        std::unique_ptr<Type> type;
        output << "sizeof(";
        if (is_expression(arg, declarations)) {
            type = emit(arg, environment, declarations, output);
        } else {
            type = Type::parse_object(arg);
            declarations.use_type(*type);
            output << type->to_string("");
        }
        output << ")";

        if (type->is_function() || is_void(*type))
            invalid_operand(get_name(), *type);
        return std::make_unique<ValueType>("size_t");
    }

private:
    static bool is_expression(const ObjectRef<Object>& obj, const Declarations& declarations)
    {
        Alma& alma = Alma::alma;
        if (alma.symbolp(obj))
            return declarations.contains(obj.as<Symbol>()->get_name());
        if (!alma.consp(obj))
            return true;
        ObjectRef<Object> car = obj.as<Cons>()->get_car();
        if (!alma.symbolp(car))
            return false;
        ObjectRef<Symbol> symbol(car);
        return symbol->has_emitter() || declarations.contains(symbol->get_name());
    }
};

/** (type) value */
class EmitterCast : public Emitter {
public:
    EmitterCast()
        : Emitter("cast")
    {
    }

    std::unique_ptr<Type> emit_expression(ObjectRef<Object> self [[maybe_unused]], const Arguments& arg_list,
        ObjectRef<Environment> environment, Declarations& declarations, std::stringstream& output) override
    {
        check_arguments(get_name(), arg_list, 2, 2);
        std::unique_ptr<Type> target = Type::parse_object(arg_list[0]);
        declarations.use_type(*target);
        output << "((" << target->to_string("") << ")";
        std::unique_ptr<Type> value = emit(arg_list[1], environment, declarations, output)->decay();
        output << ")";

        if (is_void(*target))
            return target;
        bool valid = is_scalar(*target) && !target->is_array() && is_scalar(*value);
        if (valid && target->is_pointer())
            valid = value->is_pointer() || is_integer(*value);
        if (valid && value->is_pointer())
            valid = target->is_pointer() || is_integer(*target);
        if (!valid)
            athrow("Cannot cast a value of type " << type_string(*value) << " to " << type_string(*target));
        return target;
    }
};

// -----------------------------------------------------------------------------

template <typename T, typename... AS>
static void intern_operator(const std::string& name, AS&&... args)
{
    ObjectRef<Symbol> sym = Alma::alma.intern_alma_symbol(name);
    sym->push_emitter(Alma::alma.make<T>(std::forward<AS>(args)...));
}

void intern_operators()
{
    using Kind = EmitterCompoundAssign::Kind;

    intern_operator<EmitterAdditive>("+", "+", "+");
    intern_operator<EmitterAdditive>("-", "-", "-");
    intern_operator<EmitterStar>("*");
    intern_operator<EmitterArithmetic>("/", "/", "/", false);
    intern_operator<EmitterArithmetic>("%", "%", "%", true);
    intern_operator<EmitterArithmetic>("bit-and", "bit-and", "&", true);
    intern_operator<EmitterArithmetic>("bit-or", "bit-or", "|", true);
    intern_operator<EmitterArithmetic>("bit-xor", "bit-xor", "^", true);
    intern_operator<EmitterShift>("<<", "<<", "<<");
    intern_operator<EmitterShift>(">>", ">>", ">>");

    for (const std::string op : { "==", "!=", "<", ">", "<=", ">=" })
        intern_operator<EmitterComparison>(op, op, op);

    intern_operator<EmitterLogical>("and", "and", "&&");
    intern_operator<EmitterLogical>("or", "or", "||");
    intern_operator<EmitterUnary>("not", "not", "!", false);
    intern_operator<EmitterUnary>("bit-not", "bit-not", "~", true);

    intern_operator<EmitterAssign>("=");
    intern_operator<EmitterCompoundAssign>("+=", "+=", "+=", Kind::Additive);
    intern_operator<EmitterCompoundAssign>("-=", "-=", "-=", Kind::Additive);
    intern_operator<EmitterCompoundAssign>("*=", "*=", "*=", Kind::Arithmetic);
    intern_operator<EmitterCompoundAssign>("/=", "/=", "/=", Kind::Arithmetic);
    intern_operator<EmitterCompoundAssign>("%=", "%=", "%=", Kind::Integer);
    intern_operator<EmitterCompoundAssign>("<<=", "<<=", "<<=", Kind::Integer);
    intern_operator<EmitterCompoundAssign>(">>=", ">>=", ">>=", Kind::Integer);
    intern_operator<EmitterCompoundAssign>("bit-and=", "bit-and=", "&=", Kind::Integer);
    intern_operator<EmitterCompoundAssign>("bit-or=", "bit-or=", "|=", Kind::Integer);
    intern_operator<EmitterCompoundAssign>("bit-xor=", "bit-xor=", "^=", Kind::Integer);

    intern_operator<EmitterIncrement>("++", "++", "++", true);
    intern_operator<EmitterIncrement>("--", "--", "--", true);
    intern_operator<EmitterIncrement>("post++", "post++", "++", false);
    intern_operator<EmitterIncrement>("post--", "post--", "--", false);

    intern_operator<EmitterAddress>("&");
    intern_operator<EmitterSubscript>("aref");
    intern_operator<EmitterConditional>("?");
    intern_operator<EmitterComma>("comma");
    intern_operator<EmitterSizeof>("sizeof");
    intern_operator<EmitterCast>("cast");
}
