
#include "type.hpp"
#include "../alma.hpp"
#include "../debug.hpp"
#include "../object/cons.hpp"
#include "../object/integer.hpp"
#include "../object/symbol.hpp"
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

// -----------------------------------------------------------------------------

ValueType::ValueType(const std::string& name, const std::vector<Specifier>& specifiers)
    : name(name)
    , specifiers(specifiers)
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
    static const std::unordered_set<std::string> arithmetic_types = {
        "char", "short", "int", "long", "float", "double", "bool", "_Bool",
        "signed", "unsigned", "size_t", "ptrdiff_t",
        "int8_t", "int16_t", "int32_t", "int64_t",
        "uint8_t", "uint16_t", "uint32_t", "uint64_t"
    };
    return specifiers.empty() && arithmetic_types.contains(name);
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
