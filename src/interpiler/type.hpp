
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

std::string specifier_to_string(const Specifier& specifier, const std::string& core);
std::string specifiers_to_string(const std::vector<Specifier>& specifiers, const std::string& core);

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
    ValueType(const std::string& name, const std::vector<Specifier>& specifiers);
    ValueType(const ValueType& other);

    std::unique_ptr<Type> clone() const override;
    std::string to_string(const std::string& core) const override;

    bool operator==(const Type& other) const override;
    bool operator==(const ValueType& other) const override;
    bool operator==(const FunctionType& other [[maybe_unused]]) const override;
};

class FunctionType : public Type {
private:
    std::unique_ptr<Type> result;
    std::vector<std::unique_ptr<Type>> arguments;
    std::vector<Specifier> specifiers;

public:
    FunctionType(const std::unique_ptr<Type>& _result, const std::vector<std::unique_ptr<ValueType>>& _arguments, const std::vector<Specifier>& _specifiers);
    FunctionType(const FunctionType& other);

    std::unique_ptr<Type> clone() const override;
    std::string to_string(const std::string& core) const override;

    bool operator==(const Type& other) const override;
    bool operator==(const ValueType& other [[maybe_unused]]) const override;
    bool operator==(const FunctionType& other) const override;
};
