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
class Object;
class Cons;
template <typename T>
class ObjectRef;

class Type {
public:
    virtual ~Type() = default;

    static std::unique_ptr<Type> parse(const ObjectRef<Cons>& cons);
    static std::unique_ptr<Type> parse_object(const ObjectRef<Object>& obj);

    virtual bool operator==(const Type& other) const = 0;
    virtual bool operator==(const ValueType& other) const = 0;
    virtual bool operator==(const FunctionType& other) const = 0;

    virtual std::unique_ptr<Type> clone() const = 0;
    virtual std::string to_string(const std::string& core) const = 0;
    virtual bool is_function() const = 0;
    virtual bool is_pointer() const = 0;
    virtual bool is_array() const = 0;

    /** Return the type that results of using a value of this type as an expression:
        arrays become pointers and functions become pointers to functions. */
    virtual std::unique_ptr<Type> decay() const = 0;
};

class ValueType : public Type {
private:
    std::string name;
    std::vector<Specifier> specifiers;

public:
    ValueType(const std::string& name, const std::vector<Specifier>& specifiers = {});
    ValueType(const ValueType& other);

    bool operator==(const Type& other) const override;
    bool operator==(const ValueType& other) const override;
    bool operator==(const FunctionType& other [[maybe_unused]]) const override;

    std::unique_ptr<Type> clone() const override;
    std::string to_string(const std::string& core) const override;
    bool is_function() const override;
    bool is_pointer() const override;
    bool is_array() const override;
    std::unique_ptr<Type> decay() const override;

    const std::string& get_name() const;
    const std::vector<Specifier>& get_specifiers() const;
    bool is_void() const;
    bool is_void_pointer() const;
    bool is_arithmetic() const;
};

class FunctionType : public Type {
private:
    std::unique_ptr<Type> result;
    std::vector<std::unique_ptr<Type>> arguments;
    bool variadic;
    std::vector<Specifier> specifiers;

public:
    FunctionType(const std::unique_ptr<Type>& _result, const std::vector<std::unique_ptr<Type>>& _arguments,
        bool _variadic, const std::vector<Specifier>& _specifiers);
    FunctionType(const FunctionType& other);

    bool operator==(const Type& other) const override;
    bool operator==(const ValueType& other [[maybe_unused]]) const override;
    bool operator==(const FunctionType& other) const override;

    std::unique_ptr<Type> clone() const override;
    std::string to_string(const std::string& core) const override;
    bool is_function() const override;
    bool is_pointer() const override;
    bool is_array() const override;
    std::unique_ptr<Type> decay() const override;

    const Type& get_result() const;
    const std::vector<std::unique_ptr<Type>>& get_arguments() const;
    bool is_variadic() const;
    const std::vector<Specifier>& get_specifiers() const;

    /** Return the declarator of a function with named parameters, e.g. "main(int argc, char **argv)" */
    std::string to_string(const std::string& core, const std::vector<std::string>& argument_names) const;
};

/** Check if a value of type 'from' can be used where a value of type 'to' is expected,
    following the basic C conversion rules */
bool is_assignable(const Type& to, const Type& from);
