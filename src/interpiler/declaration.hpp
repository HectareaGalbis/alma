
#pragma once

#include "type.hpp"
#include <unordered_map>

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

public:
    void add(const Declaration& declaration);
    bool contains(const std::string& name) const;
    const Declaration& get(const std::string& name) const;

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
