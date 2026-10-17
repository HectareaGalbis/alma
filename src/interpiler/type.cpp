
#include "type.hpp"
#include "../alma.hpp"
#include "../debug.hpp"
#include "../object/cons.hpp"
#include "../object/integer.hpp"
#include "../object/symbol.hpp"

static bool is_valid_function_argument(const Type& argument)
{
    return !argument.is_function();
}

static bool are_valid_function_specifiers(const std::vector<Specifier>& specifiers)
{
    return specifiers.empty() || !std::holds_alternative<Dimension>(specifiers.front());
}

static std::unique_ptr<Type> parse_type_object(const ObjectRef<Object>& obj)
{
    if (Alma::alma.symbolp(obj))
        return std::make_unique<ValueType>(obj.as<Symbol>()->get_name(), std::vector<Specifier> {});
    aassert(Alma::alma.consp(obj), "Expected a type but found " << Alma::alma.to_string(obj));
    return Type::parse(ObjectRef<Cons>(obj));
}

static Specifier parse_specifier(const ObjectRef<Object>& obj)
{
    if (Alma::alma.symbolp(obj) && obj.as<Symbol>()->get_name() == "*")
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
        std::unique_ptr<Type> result = parse_type_object(list[0]);

        std::vector<std::unique_ptr<Type>> arguments;
        if (Alma::alma.consp(list[1])) {
            auto [argument_list, argument_tail] = list[1].as<Cons>()->to_list();
            aassert(Alma::alma.null(argument_tail),
                "Expected a proper list of arguments but found " << Alma::alma.to_string(list[1]));
            for (const ObjectRef<Object>& argument_object : argument_list) {
                std::unique_ptr<Type> argument = parse_type_object(argument_object);
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

        return std::make_unique<FunctionType>(result, arguments, specifiers);
    }

    aassert(Alma::alma.symbolp(list[0]), "Expected a type name but found " << Alma::alma.to_string(list[0]));
    std::vector<Specifier> specifiers;
    for (size_t i = 1; i < list.size(); i++)
        specifiers.push_back(parse_specifier(list[i]));
    return std::make_unique<ValueType>(list[0].as<Symbol>()->get_name(), specifiers);
}

std::string specifier_to_string(const Specifier& specifier, const std::string& core)
{
    if (std::holds_alternative<Pointer>(specifier))
        return "*" + core;
    else {
        return "(" + core + ")[" + std::to_string(std::get<Dimension>(specifier).n) + "]";
    }
}

std::string specifiers_to_string(const std::vector<Specifier>& specifiers, const std::string& core)
{
    std::string result = core;
    for (auto it = specifiers.crbegin(); it != specifiers.crend(); ++it) {
        result += specifier_to_string(*it, result);
    }
    return result;
}

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
    std::string result = specifiers_to_string(specifiers, core);
    return name + " " + result;
}

bool ValueType::is_function() const
{
    return false;
}

FunctionType::FunctionType(const std::unique_ptr<Type>& _result, const std::vector<std::unique_ptr<Type>>& _arguments, const std::vector<Specifier>& _specifiers)
    : result(_result->clone())
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
    std::string new_core = specifiers_to_string(specifiers, core);
    new_core = "(" + new_core + ")(";
    auto it = arguments.cbegin();
    if (it != arguments.cend()) {
        new_core += (*it)->to_string(new_core);
        it++;
    }
    for (; it != arguments.cend(); it++) {
        new_core += "+" + (*it)->to_string(new_core);
    }
    new_core += ")";
    return result->to_string(new_core);
}

bool FunctionType::is_function() const
{
    return specifiers.empty();
}
