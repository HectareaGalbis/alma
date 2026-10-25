
#include "declaration.hpp"
#include "../debug.hpp"
#include "standard.hpp"
#include <algorithm>

Declaration::Declaration(const std::string& _name, const std::unique_ptr<Type>& _type, bool _defined,
    const std::optional<std::string>& _header, bool _constant)
    : name(_name)
    , type(_type->clone())
    , defined(_defined)
    , header(_header)
    , constant(_constant)
{
}

Declaration::Declaration(const Declaration& other)
    : name(other.name)
    , type(other.type)
    , defined(other.defined)
    , header(other.header)
    , constant(other.constant)
{
}

Declaration& Declaration::operator=(const Declaration& other)
{
    if (this != &other) {
        name = other.name;
        type = other.type;
        defined = other.defined;
        header = other.header;
        constant = other.constant;
    }
    return *this;
}

Declaration::Declaration(Declaration&& other) noexcept = default;

Declaration& Declaration::operator=(Declaration&& other) noexcept = default;

const std::string& Declaration::getName() const { return name; }

const Type* Declaration::getType() const { return type.get(); }

bool Declaration::isDefined() const { return defined; }

const std::optional<std::string>& Declaration::getHeader() const { return header; }

bool Declaration::isConstant() const { return constant; }

void Declaration::define() { defined = true; }

bool Declaration::operator==(const Declaration& other) const
{
    return name == other.name && *type == *other.type;
}

void Declarations::add(const Declaration& declaration)
{
    const std::string& name = declaration.getName();
    aassert(!records.contains(name), "'" << name << "' is the name of a struct");
    aassert(!enums.contains(name), "'" << name << "' is the name of an enum");
    auto it = declarations.find(name);
    if (it == declarations.end()) {
        auto base_it = base->find(name);
        if (base_it == base->end()) {
            declarations.try_emplace(name, declaration);
            compact();
            return;
        }
        it = declarations.try_emplace(name, base_it->second).first;
    }

    if (it->second.getHeader()) {
        if (!(it->second == declaration) || declaration.isDefined())
            it->second = declaration;
        compact();
        return;
    }

    aassert(it->second == declaration, "Declaration of '" << name << "' does not match the previous declaration");

    if (declaration.isDefined()) {
        aassert(!it->second.isDefined(), "Redefinition of '" << name << "'");
        it->second.define();
    }
    compact();
}

const Declaration* Declarations::find(const std::string& name) const
{
    auto it = declarations.find(name);
    if (it != declarations.end())
        return &it->second;
    auto base_it = base->find(name);
    if (base_it != base->end())
        return &base_it->second;
    return nullptr;
}

void Declarations::compact()
{
    if (declarations.size() <= max_overlay)
        return;
    if (base.use_count() > 1)
        base = std::make_shared<DeclarationMap>(*base);
    for (auto& [name, declaration] : declarations)
        base->insert_or_assign(name, std::move(declaration));
    declarations.clear();
}

bool Declarations::contains(const std::string& name) const { return find(name) != nullptr; }

const Declaration& Declarations::get(const std::string& name) const
{
    const Declaration* declaration = find(name);
    aassert(declaration, "Declaration '" << name << "' not found");
    return *declaration;
}

std::vector<std::string> Declarations::names() const
{
    std::vector<std::string> result;
    for (const auto& [name, declaration] : declarations)
        result.push_back(name);
    for (const auto& [name, declaration] : *base)
        if (!declarations.contains(name))
            result.push_back(name);
    std::sort(result.begin(), result.end());
    return result;
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
    compact();
}

Declarations Declarations::block_scope() const
{
    Declarations scope(*this);
    scope.locals.clear();
    return scope;
}

Declarations Declarations::loop_scope() const
{
    Declarations scope = block_scope();
    scope.in_loop = true;
    return scope;
}

Declarations Declarations::switch_scope() const
{
    Declarations scope = block_scope();
    scope.in_switch = true;
    return scope;
}

bool Declarations::can_break() const
{
    return in_loop || in_switch;
}

bool Declarations::can_continue() const
{
    return in_loop;
}

Declarations Declarations::function_scope(const Declaration& _function) const
{
    Declarations scope(*this);
    scope.locals.clear();
    scope.function = _function;
    scope.labels = std::make_shared<Labels>();
    return scope;
}

void Declarations::define_label(const std::string& name)
{
    aassert(labels, "A label can only be defined inside a function");
    aassert(!labels->defined.contains(name), "Duplicate label '" << name << "'");
    labels->defined.insert(name);
}

void Declarations::use_label(const std::string& name)
{
    aassert(labels, "goto can only be used inside a function");
    labels->used.insert(name);
}

std::vector<std::string> Declarations::undefined_labels() const
{
    std::vector<std::string> undefined;
    if (labels)
        for (const std::string& name : labels->used)
            if (!labels->defined.contains(name))
                undefined.push_back(name);
    return undefined;
}

void Declarations::add_enum(const std::string& name, const std::vector<std::string>& constants)
{
    aassert(!enums.contains(name), "Redefinition of enum '" << name << "'");
    enums.emplace(name, constants);
}

bool Declarations::is_enum(const std::string& name) const
{
    return enums.contains(name);
}

const std::vector<std::string>& Declarations::get_enum_constants(const std::string& name) const
{
    auto it = enums.find(name);
    aassert(it != enums.end(), "Enum '" << name << "' not found");
    return it->second;
}

std::vector<std::string> Declarations::enum_names() const
{
    std::vector<std::string> names;
    for (const auto& [name, constants] : enums)
        names.push_back(name);
    return names;
}

Records& Declarations::get_records()
{
    return records;
}

const Records& Declarations::get_records() const
{
    return records;
}

const Record* Declarations::get_record(const Type& type) const
{
    const ValueType* value = dynamic_cast<const ValueType*>(&type);
    if (!value || !value->get_specifiers().empty() || !records.contains(value->get_name()))
        return nullptr;
    return &records.get(value->get_name());
}

bool Declarations::is_complete(const Type& type) const
{
    if (type.is_function() || is_void(type))
        return false;
    if (type.is_array())
        return is_complete(*pointee(type));
    if (const Record* record = get_record(type))
        return record->isDefined();
    return true;
}

const Declaration* Declarations::get_function() const
{
    return function ? &*function : nullptr;
}
