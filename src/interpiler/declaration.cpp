
#include "declaration.hpp"
#include "../debug.hpp"

Declaration::Declaration(const std::string& name, const std::unique_ptr<Type>& type, bool defined)
    : name(name)
    , type(type->clone())
    , defined(defined)
{
}

Declaration::Declaration(const Declaration& other)
    : name(other.name)
    , type(other.type->clone())
    , defined(other.defined)
{
}

Declaration& Declaration::operator=(const Declaration& other)
{
    if (this != &other) {
        name = other.name;
        type = other.type->clone();
        defined = other.defined;
    }
    return *this;
}

Declaration::Declaration(Declaration&& other) noexcept = default;

Declaration& Declaration::operator=(Declaration&& other) noexcept = default;

const std::string& Declaration::getName() const { return name; }

const Type* Declaration::getType() const { return type.get(); }

bool Declaration::isDefined() const { return defined; }

void Declaration::define() { defined = true; }

bool Declaration::operator==(const Declaration& other) const
{
    return name == other.name && *type == *other.type;
}

void Declarations::add(const Declaration& declaration)
{
    const std::string& name = declaration.getName();
    auto it = declarations.find(name);
    if (it == declarations.end()) {
        declarations.try_emplace(name, declaration);
        return;
    }

    aassert(it->second == declaration, "Declaration of '" << name << "' does not match the previous declaration");

    if (declaration.isDefined()) {
        aassert(!it->second.isDefined(), "Redefinition of '" << name << "'");
        it->second.define();
    }
}

bool Declarations::contains(const std::string& name) const { return declarations.contains(name); }

const Declaration& Declarations::get(const std::string& name) const
{
    auto it = declarations.find(name);
    aassert(it != declarations.end(), "Declaration '" << name << "' not found");
    return it->second;
}

void Declarations::add_local(const Declaration& declaration)
{
    const std::string& name = declaration.getName();
    aassert(!locals.contains(name), "Redeclaration of '" << name << "'");
    declarations.insert_or_assign(name, declaration);
    locals.insert(name);
}

Declarations Declarations::function_scope(const Declaration& _function) const
{
    Declarations scope(*this);
    scope.locals.clear();
    scope.function = _function;
    return scope;
}

const Declaration* Declarations::get_function() const
{
    return function ? &*function : nullptr;
}
