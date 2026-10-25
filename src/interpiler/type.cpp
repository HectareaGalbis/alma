
#include "type.hpp"
#include "../alma.hpp"
#include "../debug.hpp"
#include "../object/cons.hpp"
#include "../object/integer.hpp"
#include "../object/symbol.hpp"
#include <unordered_map>
#include <unordered_set>

static bool is_valid_function_argument(const Type& argument)
{
    return !argument.is_function();
}

static bool are_valid_function_specifiers(const std::vector<Specifier>& specifiers)
{
    return specifiers.empty() || !std::holds_alternative<Dimension>(specifiers.front());
}

static bool is_symbol_named(const ObjectRef<Object>& obj, const std::string& name)
{
    return Alma::alma.symbolp(obj) && obj.as<Symbol>()->get_name() == name;
}

std::unique_ptr<Type> Type::parse_object(const ObjectRef<Object>& obj)
{
    if (Alma::alma.symbolp(obj)) {
        aassert(!is_symbol_named(obj, "const"), "Expected a type after const");
        return std::make_unique<ValueType>(obj.as<Symbol>()->get_name(), std::vector<Specifier> {});
    }
    aassert(Alma::alma.consp(obj), "Expected a type but found " << Alma::alma.to_string(obj));
    return Type::parse(ObjectRef<Cons>(obj));
}

static Specifier parse_specifier(const ObjectRef<Object>& obj)
{
    if (is_symbol_named(obj, "*"))
        return Pointer {};
    aassert(Alma::alma.typep(obj, Alma::alma.intern_alma_symbol("integer")),
        "Expected '*' or an integer as specifier but found " << Alma::alma.to_string(obj));
    return Dimension { static_cast<size_t>(obj.as<Integer>()->get_value()) };
}

/** Parse the specifiers from a position of a list. A const after * qualifies that pointer. */
static std::vector<Specifier> parse_specifiers(const std::vector<ObjectRef<Object>>& list, size_t start)
{
    std::vector<Specifier> specifiers;
    for (size_t i = start; i < list.size(); i++) {
        if (is_symbol_named(list[i], "const")) {
            aassert(!specifiers.empty() && std::holds_alternative<Pointer>(specifiers.back()),
                "const can only be written before the type name or after *");
            Pointer& pointer = std::get<Pointer>(specifiers.back());
            aassert(!pointer.constant, "Duplicate const");
            pointer.constant = true;
        } else {
            specifiers.push_back(parse_specifier(list[i]));
        }
    }
    return specifiers;
}

std::unique_ptr<Type> Type::parse(const ObjectRef<Cons>& cons)
{
    auto [list, tail] = cons->to_list();
    aassert(Alma::alma.null(tail), "Expected a proper list as type but found " << Alma::alma.to_string(cons));

    if (list.size() >= 2 && (Alma::alma.consp(list[1]) || Alma::alma.null(list[1]))) {
        std::unique_ptr<Type> result = Type::parse_object(list[0]);
        aassert(!result->is_function(), "A function cannot return a function: " << Alma::alma.to_string(cons));
        aassert(!result->is_array(), "A function cannot return an array: " << Alma::alma.to_string(cons));

        std::vector<std::unique_ptr<Type>> arguments;
        bool variadic = false;
        if (Alma::alma.consp(list[1])) {
            auto [argument_list, argument_tail] = list[1].as<Cons>()->to_list();
            aassert(Alma::alma.null(argument_tail),
                "Expected a proper list of arguments but found " << Alma::alma.to_string(list[1]));
            for (size_t i = 0; i < argument_list.size(); i++) {
                const ObjectRef<Object>& argument_object = argument_list[i];
                if (is_symbol_named(argument_object, "&rest")) {
                    aassert(i == argument_list.size() - 1,
                        "&rest must be the last argument: " << Alma::alma.to_string(list[1]));
                    aassert(i > 0, "A variadic function needs at least one argument: " << Alma::alma.to_string(list[1]));
                    variadic = true;
                    break;
                }
                std::unique_ptr<Type> argument = Type::parse_object(argument_object);
                aassert(is_valid_function_argument(*argument),
                    "A function argument cannot be a function: " << Alma::alma.to_string(argument_object));
                arguments.push_back(std::move(argument));
            }
        }

        std::vector<Specifier> specifiers = parse_specifiers(list, 2);
        aassert(are_valid_function_specifiers(specifiers),
            "A function type cannot be an array: " << Alma::alma.to_string(cons));

        return std::make_unique<FunctionType>(result, arguments, variadic, specifiers);
    }

    // (const name specifier...)
    bool constant = is_symbol_named(list[0], "const");
    size_t name_position = constant ? 1 : 0;
    aassert(name_position < list.size(), "Expected a type after const: " << Alma::alma.to_string(cons));
    const ObjectRef<Object>& name = list[name_position];
    aassert(Alma::alma.symbolp(name) && !is_symbol_named(name, "const"),
        "Expected a type name but found " << Alma::alma.to_string(name));
    std::vector<Specifier> specifiers = parse_specifiers(list, name_position + 1);
    return std::make_unique<ValueType>(name.as<Symbol>()->get_name(), specifiers, constant);
}

static std::string parenthesize_if_pointer(const std::string& core)
{
    if (!core.empty() && core.front() == '*')
        return "(" + core + ")";
    return core;
}

std::string specifier_to_string(const Specifier& specifier, const std::string& core)
{
    if (std::holds_alternative<Pointer>(specifier)) {
        if (!std::get<Pointer>(specifier).constant)
            return "*" + core;
        return core.empty() ? "*const" : "*const " + core;
    }
    else
        return parenthesize_if_pointer(core) + "[" + std::to_string(std::get<Dimension>(specifier).n) + "]";
}

std::string specifiers_to_string(const std::vector<Specifier>& specifiers, const std::string& core)
{
    std::string result = core;
    for (auto it = specifiers.crbegin(); it != specifiers.crend(); ++it)
        result = specifier_to_string(*it, result);
    return result;
}

/** Rank used by the usual arithmetic conversions. 0 means that the type is not arithmetic.
    Every type with rank 1 is promoted to int. */
static std::unordered_set<std::string>& enum_types()
{
    static std::unordered_set<std::string> types;
    return types;
}

void declare_enum_type(const std::string& name)
{
    enum_types().insert(name);
}

static int arithmetic_rank(const std::string& name)
{
    // An enum is an integer type that is promoted to int
    if (enum_types().contains(name))
        return 1;
    static const std::unordered_map<std::string, int> ranks = {
        { "bool", 1 }, { "_Bool", 1 }, { "char", 1 }, { "signed", 1 }, { "short", 1 }, { "int", 1 },
        { "int8_t", 1 }, { "int16_t", 1 }, { "int32_t", 1 }, { "uint8_t", 1 }, { "uint16_t", 1 },
        { "wchar_t", 1 }, { "sig_atomic_t", 1 },
        { "unsigned", 2 }, { "uint32_t", 2 },
        { "long", 3 }, { "int64_t", 3 }, { "uint64_t", 3 }, { "size_t", 3 }, { "ptrdiff_t", 3 },
        { "intptr_t", 3 }, { "uintptr_t", 3 }, { "time_t", 3 }, { "clock_t", 3 },
        { "float", 4 },
        { "double", 5 }
    };
    auto it = ranks.find(name);
    return it == ranks.end() ? 0 : it->second;
}

/** The specifiers with the syntax of alma: * [const] or a dimension */
static void push_specifiers(std::vector<ObjectRef<Object>>& list, const std::vector<Specifier>& specifiers)
{
    for (const Specifier& specifier : specifiers) {
        if (std::holds_alternative<Pointer>(specifier)) {
            list.push_back(Alma::alma.intern_alma_symbol("*"));
            if (std::get<Pointer>(specifier).constant)
                list.push_back(Alma::alma.intern_alma_symbol("const"));
        } else {
            list.push_back(Alma::alma.make<Integer>(static_cast<int64_t>(std::get<Dimension>(specifier).n)));
        }
    }
}

static ObjectRef<Object> make_list(const std::vector<ObjectRef<Object>>& elements)
{
    if (elements.empty())
        return Alma::alma.intern_alma_symbol("nil");
    return Alma::alma.make<Cons>(elements);
}

// -----------------------------------------------------------------------------

ValueType::ValueType(const std::string& _name, const std::vector<Specifier>& _specifiers, bool _constant)
    : name(_name)
    , specifiers(_specifiers)
    , constant(_constant)
{
}

ValueType::ValueType(const ValueType& other)
    : name(other.name)
    , specifiers(other.specifiers)
    , constant(other.constant)
{
}

bool ValueType::operator==(const Type& other) const
{
    return other == *this;
}
bool ValueType::operator==(const ValueType& other) const
{
    return name == other.name && specifiers == other.specifiers && constant == other.constant;
}
bool ValueType::operator==(const FunctionType& other [[maybe_unused]]) const
{
    return false;
}

std::unique_ptr<Type> ValueType::clone() const
{
    return std::make_unique<ValueType>(*this);
}

std::string ValueType::to_string(const std::string& core) const
{
    std::string declarator = specifiers_to_string(specifiers, core);
    std::string type_name = constant ? "const " + name : name;
    if (declarator.empty())
        return type_name;
    return type_name + " " + declarator;
}

ObjectRef<Object> ValueType::to_object() const
{
    if (specifiers.empty() && !constant)
        return Alma::alma.intern_alma_symbol(name);
    std::vector<ObjectRef<Object>> list;
    if (constant)
        list.push_back(Alma::alma.intern_alma_symbol("const"));
    list.push_back(Alma::alma.intern_alma_symbol(name));
    push_specifiers(list, specifiers);
    return make_list(list);
}

bool ValueType::is_function() const
{
    return false;
}

bool ValueType::is_pointer() const
{
    return !specifiers.empty() && std::holds_alternative<Pointer>(specifiers.back());
}

bool ValueType::is_array() const
{
    return !specifiers.empty() && std::holds_alternative<Dimension>(specifiers.back());
}

std::unique_ptr<Type> ValueType::decay() const
{
    if (!is_array())
        return clone();
    std::vector<Specifier> decayed = specifiers;
    decayed.back() = Pointer {};
    return std::make_unique<ValueType>(name, decayed, constant);
}

const std::string& ValueType::get_name() const
{
    return name;
}

const std::vector<Specifier>& ValueType::get_specifiers() const
{
    return specifiers;
}

bool ValueType::is_name_const() const
{
    return constant;
}

bool ValueType::is_void() const
{
    return name == "void" && specifiers.empty();
}

bool ValueType::is_void_pointer() const
{
    return name == "void" && specifiers.size() == 1 && is_pointer();
}

bool ValueType::is_arithmetic() const
{
    return specifiers.empty() && arithmetic_rank(name) > 0;
}

// -----------------------------------------------------------------------------

FunctionType::FunctionType(const std::unique_ptr<Type>& _result, const std::vector<std::unique_ptr<Type>>& _arguments,
    bool _variadic, const std::vector<Specifier>& _specifiers)
    : result(_result->clone())
    , variadic(_variadic)
    , specifiers(_specifiers)
{
    aassert(are_valid_function_specifiers(specifiers),
        "A function type cannot be an array");
    for (const std::unique_ptr<Type>& argument : _arguments) {
        aassert(is_valid_function_argument(*argument), "A function argument cannot be a function");
        arguments.emplace_back(argument->clone());
    }
}

FunctionType::FunctionType(const FunctionType& other)
    : result(other.result->clone())
    , variadic(other.variadic)
    , specifiers(other.specifiers)
{
    for (const std::unique_ptr<Type>& argument : other.arguments)
        arguments.emplace_back(argument->clone());
}

bool FunctionType::operator==(const Type& other) const
{
    return other == *this;
}
bool FunctionType::operator==(const ValueType& other [[maybe_unused]]) const
{
    return false;
}
bool FunctionType::operator==(const FunctionType& other) const
{
    if (*result != *other.result)
        return false;

    if (arguments.size() != other.arguments.size())
        return false;
    for (size_t i = 0; i < arguments.size(); i++)
        if (*arguments[i] != *other.arguments[i])
            return false;

    if (variadic != other.variadic)
        return false;

    if (specifiers != other.specifiers)
        return false;

    return true;
}

std::unique_ptr<Type> FunctionType::clone() const
{
    return std::make_unique<FunctionType>(*this);
}

std::string FunctionType::to_string(const std::string& core) const
{
    return this->to_string(core, std::vector<std::string>(arguments.size()));
}

std::string FunctionType::to_string(const std::string& core, const std::vector<std::string>& argument_names) const
{
    std::string declarator = parenthesize_if_pointer(specifiers_to_string(specifiers, core));
    declarator += "(";
    for (size_t i = 0; i < arguments.size(); i++) {
        if (i > 0)
            declarator += ", ";
        declarator += arguments[i]->to_string(argument_names[i]);
    }
    if (variadic)
        declarator += ", ...";
    else if (arguments.empty())
        declarator += "void";
    declarator += ")";
    return result->to_string(declarator);
}

ObjectRef<Object> FunctionType::to_object() const
{
    std::vector<ObjectRef<Object>> argument_list;
    for (const std::unique_ptr<Type>& argument : arguments)
        argument_list.push_back(argument->to_object());
    if (variadic)
        argument_list.push_back(Alma::alma.intern_alma_symbol("&rest"));

    std::vector<ObjectRef<Object>> list { result->to_object(), make_list(argument_list) };
    push_specifiers(list, specifiers);
    return make_list(list);
}

bool FunctionType::is_function() const
{
    return specifiers.empty();
}

bool FunctionType::is_pointer() const
{
    return !specifiers.empty() && std::holds_alternative<Pointer>(specifiers.back());
}

bool FunctionType::is_array() const
{
    return !specifiers.empty() && std::holds_alternative<Dimension>(specifiers.back());
}

std::unique_ptr<Type> FunctionType::decay() const
{
    if (is_function())
        return std::make_unique<FunctionType>(result, arguments, variadic, std::vector<Specifier> { Pointer {} });
    if (is_array()) {
        std::vector<Specifier> decayed = specifiers;
        decayed.back() = Pointer {};
        return std::make_unique<FunctionType>(result, arguments, variadic, decayed);
    }
    return clone();
}

const Type& FunctionType::get_result() const
{
    return *result;
}

const std::vector<std::unique_ptr<Type>>& FunctionType::get_arguments() const
{
    return arguments;
}

bool FunctionType::is_variadic() const
{
    return variadic;
}

const std::vector<Specifier>& FunctionType::get_specifiers() const
{
    return specifiers;
}

// -----------------------------------------------------------------------------

bool is_assignable(const Type& to, const Type& from)
{
    // The qualifiers of the value itself do not matter when it is copied
    std::unique_ptr<Type> target = unqualified(to);
    std::unique_ptr<Type> value = unqualified(*from.decay());
    if (*target == *value)
        return true;

    if (is_arithmetic(*target) && is_arithmetic(*value))
        return true;

    if (target->is_pointer() && value->is_pointer()) {
        // A pointer can point to a more qualified type (char * -> const char *), but not to a less one
        std::unique_ptr<Type> target_pointee = pointee(*target);
        std::unique_ptr<Type> value_pointee = pointee(*value);
        if (is_const(*value_pointee) && !is_const(*target_pointee))
            return false;
        std::unique_ptr<Type> plain_target = unqualified(*target_pointee);
        std::unique_ptr<Type> plain_value = unqualified(*value_pointee);
        return *plain_target == *plain_value || is_void(*plain_target) || is_void(*plain_value);
    }

    return false;
}

// -----------------------------------------------------------------------------

static const ValueType* as_value_type(const Type& type)
{
    return dynamic_cast<const ValueType*>(&type);
}

bool is_void(const Type& type)
{
    const ValueType* value = as_value_type(type);
    return value && value->is_void();
}

bool is_arithmetic(const Type& type)
{
    const ValueType* value = as_value_type(type);
    return value && value->is_arithmetic();
}

bool is_integer(const Type& type)
{
    const ValueType* value = as_value_type(type);
    return value && value->is_arithmetic() && value->get_name() != "float" && value->get_name() != "double";
}

bool is_scalar(const Type& type)
{
    std::unique_ptr<Type> decayed = type.decay();
    return is_arithmetic(*decayed) || decayed->is_pointer();
}

std::unique_ptr<Type> integer_promotion(const Type& type)
{
    const ValueType* value = as_value_type(type);
    if (arithmetic_rank(value->get_name()) == 1)
        return std::make_unique<ValueType>("int");
    return unqualified(type);
}

std::unique_ptr<Type> arithmetic_conversion(const Type& a, const Type& b)
{
    const ValueType* value_a = as_value_type(a);
    const ValueType* value_b = as_value_type(b);
    const ValueType* higher = arithmetic_rank(value_a->get_name()) >= arithmetic_rank(value_b->get_name())
        ? value_a
        : value_b;
    return integer_promotion(*higher);
}

std::unique_ptr<Type> pointer_to(const Type& type)
{
    if (const ValueType* value = as_value_type(type)) {
        std::vector<Specifier> specifiers = value->get_specifiers();
        specifiers.push_back(Pointer {});
        return std::make_unique<ValueType>(value->get_name(), specifiers, value->is_name_const());
    }
    const FunctionType& function = static_cast<const FunctionType&>(type);
    std::vector<Specifier> specifiers = function.get_specifiers();
    specifiers.push_back(Pointer {});
    return std::make_unique<FunctionType>(function.get_result().clone(), function.get_arguments(),
        function.is_variadic(), specifiers);
}

std::unique_ptr<Type> pointee(const Type& pointer)
{
    if (const ValueType* value = as_value_type(pointer)) {
        std::vector<Specifier> specifiers = value->get_specifiers();
        specifiers.pop_back();
        return std::make_unique<ValueType>(value->get_name(), specifiers, value->is_name_const());
    }
    const FunctionType& function = static_cast<const FunctionType&>(pointer);
    std::vector<Specifier> specifiers = function.get_specifiers();
    specifiers.pop_back();
    return std::make_unique<FunctionType>(function.get_result().clone(), function.get_arguments(),
        function.is_variadic(), specifiers);
}

bool are_compatible_pointers(const Type& a, const Type& b)
{
    std::unique_ptr<Type> decayed_a = a.decay();
    std::unique_ptr<Type> decayed_b = b.decay();
    if (!decayed_a->is_pointer() || !decayed_b->is_pointer())
        return false;
    std::unique_ptr<Type> pointee_a = unqualified(*pointee(*decayed_a));
    std::unique_ptr<Type> pointee_b = unqualified(*pointee(*decayed_b));
    return *pointee_a == *pointee_b || is_void(*pointee_a) || is_void(*pointee_b);
}

/** Position of the pointer that qualifies a type: the last pointer, ignoring the dimensions of an
    array, because the qualifiers of an array are the ones of its elements. -1 means the type name. */
static int qualified_pointer(const std::vector<Specifier>& specifiers)
{
    int i = static_cast<int>(specifiers.size()) - 1;
    while (i >= 0 && std::holds_alternative<Dimension>(specifiers[i]))
        i--;
    return i;
}

static std::unique_ptr<Type> with_const(const Type& type, bool constant)
{
    if (const ValueType* value = as_value_type(type)) {
        std::vector<Specifier> specifiers = value->get_specifiers();
        int position = qualified_pointer(specifiers);
        if (position < 0)
            return std::make_unique<ValueType>(value->get_name(), specifiers, constant);
        std::get<Pointer>(specifiers[position]).constant = constant;
        return std::make_unique<ValueType>(value->get_name(), specifiers, value->is_name_const());
    }
    // A function cannot be qualified, only a pointer to it
    const FunctionType& function = static_cast<const FunctionType&>(type);
    std::vector<Specifier> specifiers = function.get_specifiers();
    int position = qualified_pointer(specifiers);
    if (position < 0)
        return type.clone();
    std::get<Pointer>(specifiers[position]).constant = constant;
    return std::make_unique<FunctionType>(function.get_result().clone(), function.get_arguments(),
        function.is_variadic(), specifiers);
}

bool is_const(const Type& type)
{
    const std::vector<Specifier>& specifiers = as_value_type(type)
        ? as_value_type(type)->get_specifiers()
        : static_cast<const FunctionType&>(type).get_specifiers();
    int position = qualified_pointer(specifiers);
    if (position >= 0)
        return std::get<Pointer>(specifiers[position]).constant;
    return as_value_type(type) && as_value_type(type)->is_name_const();
}

std::unique_ptr<Type> unqualified(const Type& type)
{
    return with_const(type, false);
}

std::unique_ptr<Type> qualified(const Type& type)
{
    return with_const(type, true);
}
