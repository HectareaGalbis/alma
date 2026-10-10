
#pragma once

#include <memory>
#include <string>
#include <variant>
#include <vector>

class Pointer {
public:
    bool operator==(const Pointer&) const = default;
};
class Dimension {
public:
    size_t n;

    bool operator==(const Dimension&) const = default;
};

using Specifier = std::variant<Pointer, Dimension>;

class ValueType;
class FunctionType;

class Type {
public:
    virtual ~Type() = default;

    virtual bool operator==(const Type& other) const = 0;
    virtual bool operator==(const ValueType& other) const = 0;
    virtual bool operator==(const FunctionType& other) const = 0;
    virtual std::unique_ptr<Type> clone() const = 0;
};

class ValueType : public Type {
private:
    std::string name;
    std::vector<Specifier> specifiers;

public:
    ValueType(const std::string& name, const std::vector<Specifier>& specifiers)
        : name(name)
        , specifiers(specifiers)
    {
    }

    ValueType(const ValueType& other)
        : name(other.name)
        , specifiers(other.specifiers)
    {
    }

    std::unique_ptr<Type> clone() const override
    {
        return std::make_unique<ValueType>(*this);
    }

    bool operator==(const Type& other) const override
    {
        return other == *this;
    }
    bool operator==(const ValueType& other) const override
    {
        return name == other.name && specifiers == other.specifiers;
    }
    bool operator==(const FunctionType& other [[maybe_unused]]) const override
    {
        return false;
    }
};

class FunctionType : public Type {
private:
    std::unique_ptr<Type> result;
    std::vector<std::unique_ptr<Type>> arguments;
    std::vector<Specifier> specifiers;

public:
    FunctionType(const std::unique_ptr<Type>& _result, const std::vector<std::unique_ptr<ValueType>>& _arguments, const std::vector<Specifier>& _specifiers)
        : result(_result->clone())
        , specifiers(_specifiers)
    {
        for (const std::unique_ptr<ValueType>& argument : _arguments)
            arguments.emplace_back(argument->clone());
    }

    FunctionType(const FunctionType& other)
        : result(other.result->clone())
        , specifiers(other.specifiers)
    {
        for (const std::unique_ptr<Type>& argument : other.arguments)
            arguments.emplace_back(argument->clone());
    }

    std::unique_ptr<Type> clone() const override
    {
        return std::make_unique<FunctionType>(*this);
    }

    bool operator==(const Type& other) const override
    {
        return other == *this;
    }
    bool operator==(const ValueType& other [[maybe_unused]]) const override
    {
        return false;
    }
    bool operator==(const FunctionType& other) const override
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
};
