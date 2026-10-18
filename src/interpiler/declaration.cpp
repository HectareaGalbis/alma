
#include "declaration.hpp"
#include "../debug.hpp"
#include "standard.hpp"

Declaration::Declaration(const std::string& _name, const std::unique_ptr<Type>& _type, bool _defined,
    const std::optional<std::string>& _header)
    : name(_name)
    , type(_type->clone())
    , defined(_defined)
    , header(_header)
{
}

Declaration::Declaration(const Declaration& other)
    : name(other.name)
    , type(other.type->clone())
    , defined(other.defined)
    , header(other.header)
{
}

Declaration& Declaration::operator=(const Declaration& other)
{
    if (this != &other) {
        name = other.name;
        type = other.type->clone();
        defined = other.defined;
        header = other.header;
    }
    return *this;
}

Declaration::Declaration(Declaration&& other) noexcept = default;

Declaration& Declaration::operator=(Declaration&& other) noexcept = default;

const std::string& Declaration::getName() const { return name; }

const Type* Declaration::getType() const { return type.get(); }

bool Declaration::isDefined() const { return defined; }

const std::optional<std::string>& Declaration::getHeader() const { return header; }

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

    if (it->second.getHeader()) {
        if (!(it->second == declaration) || declaration.isDefined())
            it->second = declaration;
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

const Declaration& Declarations::use(const std::string& name)
{
    const Declaration& declaration = get(name);
    if (declaration.getHeader())
        headers->insert(*declaration.getHeader());
    use_type(*declaration.getType());
    return declaration;
}

void Declarations::use_type(const Type& type)
{
    if (const ValueType* value = dynamic_cast<const ValueType*>(&type)) {
        if (std::optional<std::string> header = standard_type_header(value->get_name()))
            headers->insert(*header);
        return;
    }
    const FunctionType& function_type = static_cast<const FunctionType&>(type);
    use_type(function_type.get_result());
    for (const std::unique_ptr<Type>& argument : function_type.get_arguments())
        use_type(*argument);
}

const std::set<std::string>& Declarations::get_headers() const
{
    return *headers;
}

Declarations Declarations::deep_copy() const
{
    Declarations copy(*this);
    copy.headers = std::make_shared<std::set<std::string>>(*headers);
    return copy;
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
