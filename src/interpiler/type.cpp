
#include "type.hpp"
#include "../alma.hpp"
#include "../debug.hpp"
#include "../object/cons.hpp"
#include "../object/integer.hpp"
#include "../object/symbol.hpp"
#include <unordered_map>

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
    if (Alma::alma.symbolp(obj))
        return std::make_unique<ValueType>(obj.as<Symbol>()->get_name(), std::vector<Specifier> {});
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

        std::vector<Specifier> specifiers;
        for (size_t i = 2; i < list.size(); i++)
            specifiers.push_back(parse_specifier(list[i]));
        aassert(are_valid_function_specifiers(specifiers),
            "A function type cannot be an array: " << Alma::alma.to_string(cons));

        return std::make_unique<FunctionType>(result, arguments, variadic, specifiers);
    }

    aassert(Alma::alma.symbolp(list[0]), "Expected a type name but found " << Alma::alma.to_string(list[0]));
    std::vector<Specifier> specifiers;
    for (size_t i = 1; i < list.size(); i++)
        specifiers.push_back(parse_specifier(list[i]));
    return std::make_unique<ValueType>(list[0].as<Symbol>()->get_name(), specifiers);
}

static std::string parenthesize_if_pointer(const std::string& core)
{
    if (!core.empty() && core.front() == '*')
        return "(" + core + ")";
    return core;
}

std::string specifier_to_string(const Specifier& specifier, const std::string& core)
{
    if (std::holds_alternative<Pointer>(specifier))
        return "*" + core;
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
static int arithmetic_rank(const std::string& name)
{
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

// -----------------------------------------------------------------------------

ValueType::ValueType(const std::string& _name, const std::vector<Specifier>& _specifiers)
    : name(_name)
    , specifiers(_specifiers)
{
}

ValueType::ValueType(const ValueType& other)
    : name(other.name)
    , specifiers(other.specifiers)
{
}

bool ValueType::operator==(const Type& other) const
{
    return other == *this;
}
bool ValueType::operator==(const ValueType& other) const
{
    return name == other.name && specifiers == other.specifiers;
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
    if (declarator.empty())
        return name;
    return name + " " + declarator;
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
    return std::make_unique<ValueType>(name, decayed);
}

const std::string& ValueType::get_name() const
{
    return name;
}

const std::vector<Specifier>& ValueType::get_specifiers() const
{
    return specifiers;
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
    std::unique_ptr<Type> decayed_from = from.decay();
    if (to == *decayed_from)
        return true;

    const ValueType* value_to = dynamic_cast<const ValueType*>(&to);
    const ValueType* value_from = dynamic_cast<const ValueType*>(decayed_from.get());

    if (value_to && value_from && value_to->is_arithmetic() && value_from->is_arithmetic())
        return true;

    if (to.is_pointer() && decayed_from->is_pointer()) {
        if ((value_to && value_to->is_void_pointer()) || (value_from && value_from->is_void_pointer()))
            return true;
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
    return type.clone();
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
        return std::make_unique<ValueType>(value->get_name(), specifiers);
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
        return std::make_unique<ValueType>(value->get_name(), specifiers);
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
    if (*decayed_a == *decayed_b)
        return true;
    const ValueType* value_a = as_value_type(*decayed_a);
    const ValueType* value_b = as_value_type(*decayed_b);
    return (value_a && value_a->is_void_pointer()) || (value_b && value_b->is_void_pointer());
}
