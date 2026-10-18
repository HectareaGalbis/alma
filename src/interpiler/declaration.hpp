
#pragma once

#include "type.hpp"
#include <optional>
#include <unordered_map>
#include <unordered_set>

class Declaration {
private:
    std::string name;
    std::unique_ptr<Type> type;
    bool defined;

public:
    Declaration(const std::string& name, const std::unique_ptr<Type>& type, bool defined = false);
    Declaration(const Declaration& other);
    Declaration& operator=(const Declaration& other);
    Declaration(Declaration&& other) noexcept;
    Declaration& operator=(Declaration&& other) noexcept;

    const std::string& getName() const;
    const Type* getType() const;
    bool isDefined() const;

    void define();

    bool operator==(const Declaration& other) const;
};

class Declarations {
private:
    std::unordered_map<std::string, Declaration> declarations;
    std::unordered_set<std::string> locals;
    std::optional<Declaration> function;

public:
    void add(const Declaration& declaration);
    bool contains(const std::string& name) const;
    const Declaration& get(const std::string& name) const;

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
