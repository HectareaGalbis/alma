
#pragma once

#include "type.hpp"
#include <memory>
#include <optional>
#include <set>
#include <unordered_map>
#include <unordered_set>

class Declaration {
private:
    std::string name;
    std::unique_ptr<Type> type;
    bool defined;
    std::optional<std::string> header;
    bool constant;

public:
    Declaration(const std::string& name, const std::unique_ptr<Type>& type, bool defined = false,
        const std::optional<std::string>& header = std::nullopt, bool constant = false);
    Declaration(const Declaration& other);
    Declaration& operator=(const Declaration& other);
    Declaration(Declaration&& other) noexcept;
    Declaration& operator=(Declaration&& other) noexcept;

    const std::string& getName() const;
    const Type* getType() const;
    bool isDefined() const;
    /** Header of the C standard library where it is declared, if any */
    const std::optional<std::string>& getHeader() const;
    /** A constant, like the macros of the standard library (EOF, INT_MAX...) */
    bool isConstant() const;

    void define();

    bool operator==(const Declaration& other) const;
};

class Declarations {
private:
    std::unordered_map<std::string, Declaration> declarations;
    std::unordered_set<std::string> locals;
    std::optional<Declaration> function;
    std::shared_ptr<std::set<std::string>> headers = std::make_shared<std::set<std::string>>();

public:
    /** Add a top level declaration. A declaration of the C standard library is replaced by
        any other declaration with the same name, unless both are equal. */
    void add(const Declaration& declaration);
    bool contains(const std::string& name) const;
    const Declaration& get(const std::string& name) const;

    /** Like get, but remembers the header of the declaration to include it in the output */
    const Declaration& use(const std::string& name);
    /** Remember the headers of the standard types used by a type */
    void use_type(const Type& type);
    /** Headers used by the emitted code */
    const std::set<std::string>& get_headers() const;

    /** Copy that does not share the used headers with these declarations.
        Scopes of functions share them so that every use inside a function is remembered. */
    Declarations deep_copy() const;

    /** Add a declaration that belongs to the current scope. It shadows any previous
        declaration with the same name, but cannot be declared twice in the same scope. */
    void add_local(const Declaration& declaration);

    /** Return a copy of these declarations to check the body of a function.
        The copy starts a new scope and remembers the function it belongs to. */
    Declarations function_scope(const Declaration& function) const;

    /** Return the function whose body is being checked, if any */
    const Declaration* get_function() const;

    template <typename... Args>
    void emplace(Args&&... args)
    {
        Declaration declaration(std::forward<Args>(args)...);
        if (!contains(declaration.getName())) {
            std::string name = declaration.getName();
            declarations.emplace(std::move(name), std::move(declaration));
            return;
        }
        add(declaration);
    }
};
