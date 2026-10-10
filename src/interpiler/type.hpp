
#pragma once

#include <memory>
#include <string>
#include <variant>
#include <vector>

/*
Los tipos se generan recursivamente a partir de un tipo identificador core

* Dado T como core para un tipo funcion con especificadores (si tiene punteros o arrays), argumentos y resultado:

1º En orden inverso de especificadores:
Puntero -> T := *T
Array n -> T := (T)[n]

2º Argumentos
        -> T := (T)(arg1, arg2, ...)

3º Resultado
Llamada recursiva con core igual al nuevo T


* Dado T como core para un tipo con especificadores:

1º En orden inverso de especificadores:
Puntero -> T := *T
Array n -> T := (T)[n]

2º Name
Name name -> T := name T
*/

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

static std::string specifier_to_string(const Specifier& specifier, const std::string& core)
{
    if (std::holds_alternative<Pointer>(specifier))
        return "*" + core;
    else {
        return "(" + core + ")[" + std::to_string(std::get<Dimension>(specifier).n) + "]";
    }
}

static std::string specifiers_to_string(const std::vector<Specifier>& specifiers, const std::string& core)
{
    std::string result = core;
    for (auto it = specifiers.crbegin(); it != specifiers.crend(); ++it) {
        result += specifier_to_string(*it, result);
    }
    return result;
}

class ValueType;
class FunctionType;

class Type {
public:
    virtual ~Type() = default;

    virtual bool operator==(const Type& other) const = 0;
    virtual bool operator==(const ValueType& other) const = 0;
    virtual bool operator==(const FunctionType& other) const = 0;
    virtual std::unique_ptr<Type> clone() const = 0;
    virtual std::string to_string(const std::string& core) const = 0;
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

    std::string to_string(const std::string& core) const override
    {
        std::string result = specifiers_to_string(specifiers, core);
        return name + " " + result;
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

    std::string to_string(const std::string& core) const override
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
