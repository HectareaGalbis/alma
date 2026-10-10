
#include "type.hpp"

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

std::unique_ptr<Type> ValueType::clone() const
{
    return std::make_unique<ValueType>(*this);
}

std::string ValueType::to_string(const std::string& core) const
{
    std::string result = specifiers_to_string(specifiers, core);
    return name + " " + result;
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

FunctionType::FunctionType(const std::unique_ptr<Type>& _result, const std::vector<std::unique_ptr<ValueType>>& _arguments, const std::vector<Specifier>& _specifiers)
    : result(_result->clone())
    , specifiers(_specifiers)
{
    for (const std::unique_ptr<ValueType>& argument : _arguments)
        arguments.emplace_back(argument->clone());
}

FunctionType::FunctionType(const FunctionType& other)
    : result(other.result->clone())
    , specifiers(other.specifiers)
{
    for (const std::unique_ptr<Type>& argument : other.arguments)
        arguments.emplace_back(argument->clone());
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
