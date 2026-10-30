#include "package.hpp"
#include "alma.hpp"
#include "debug.hpp"
#include "symbol.hpp"
#include <algorithm>

static std::string character_name(char c)
{
    return std::string("#") + c;
}

Package::Package(const std::string& _name, bool _keyword)
    : name(_name)
    , deleted(false)
    , keyword(_keyword)
{
}

std::string Package::to_string(ObjectRef<Object> self [[maybe_unused]])
{
    if (this->deleted)
        return "<deleted package>";
    return "<package " + this->name + ">";
}

ObjectRef<Object> Package::type(ObjectRef<Object> self [[maybe_unused]]) const
{
    return Alma::alma.intern_alma_symbol("package");
}

// -----------------------------------------------------------------------------
// Name

const std::string& Package::get_name() const
{
    return this->name;
}

void Package::set_name(const std::string& _name)
{
    this->name = _name;
}

bool Package::is_deleted() const
{
    return this->deleted;
}

bool Package::is_keyword() const
{
    return this->keyword;
}

// -----------------------------------------------------------------------------
// Symbols

bool Package::conflicts(const std::string& _name, ObjectRef<Symbol> symbol)
{
    std::optional<Lookup> found = this->find_accessible_symbol(_name);
    return found && found->symbol != symbol && !this->shadowing_names.contains(_name);
}

void Package::add_present_symbol(ObjectRef<Symbol> symbol)
{
    this->symbols.try_emplace(symbol->get_name(), *this, symbol);
    if (!symbol->get_home_package())
        symbol->set_home_package(ObjectRef<Package>(this));
}

void Package::remove_present_symbol(const std::string& _name)
{
    auto it = this->symbols.find(_name);
    if (it == this->symbols.end())
        return;
    ObjectRef<Symbol> symbol = it->second;
    this->symbols.erase(it);
    this->external_names.erase(_name);
    this->shadowing_names.erase(_name);
    std::optional<ObjectRef<Package>> home = symbol->get_home_package();
    if (home && home->get() == this)
        symbol->clear_home_package();
}

std::optional<ObjectRef<Symbol>> Package::find_present_symbol(const std::string& _name)
{
    auto it = this->symbols.find(_name);
    if (it == this->symbols.end())
        return std::nullopt;
    return ObjectRef<Symbol>(it->second);
}

std::optional<ObjectRef<Symbol>> Package::find_external_symbol(const std::string& _name)
{
    if (!this->external_names.contains(_name))
        return std::nullopt;
    return this->find_present_symbol(_name);
}

std::optional<Package::Lookup> Package::find_accessible_symbol(const std::string& _name)
{
    std::optional<ObjectRef<Symbol>> present = this->find_present_symbol(_name);
    if (present)
        return Lookup { *present, this->is_external(_name) ? Status::External : Status::Internal };
    for (ObjectRef<Package> used : this->get_use_list()) {
        std::optional<ObjectRef<Symbol>> inherited = used->find_external_symbol(_name);
        if (inherited)
            return Lookup { *inherited, Status::Inherited };
    }
    return std::nullopt;
}

std::optional<ObjectRef<Object>> Package::find_symbol(const std::string& _name)
{
    std::optional<Lookup> found = this->find_accessible_symbol(_name);
    if (!found)
        return std::nullopt;
    return ObjectRef<Object>(found->symbol);
}

bool Package::is_external(const std::string& _name) const
{
    return this->external_names.contains(_name);
}

ObjectRef<Symbol> Package::intern_symbol(const std::string& _name)
{
    std::optional<Lookup> found = this->find_accessible_symbol(_name);
    if (found)
        return found->symbol;
    ObjectRef<Symbol> symbol = Alma::alma.make<Symbol>(_name);
    this->add_present_symbol(symbol);
    if (this->keyword) {
        this->external_names.insert(_name);
        symbol->push_value(symbol);
    }
    return symbol;
}

ObjectRef<Symbol> Package::intern_external_symbol(const std::string& _name)
{
    ObjectRef<Symbol> symbol = this->intern_symbol(_name);
    if (this->find_present_symbol(_name))
        this->external_names.insert(_name);
    return symbol;
}

bool Package::unintern_symbol(ObjectRef<Symbol> symbol)
{
    const std::string& _name = symbol->get_name();
    std::optional<ObjectRef<Symbol>> present = this->find_present_symbol(_name);
    if (!present || *present != symbol)
        return false;
    if (this->shadowing_names.contains(_name)) {
        std::optional<ObjectRef<Symbol>> inherited;
        for (ObjectRef<Package> used : this->get_use_list()) {
            std::optional<ObjectRef<Symbol>> other = used->find_external_symbol(_name);
            if (!other)
                continue;
            aassert(!inherited || *inherited == *other,
                "Uninterning " << _name << " from " << this->name
                               << " causes a name conflict between the symbols inherited from its used packages");
            inherited = other;
        }
    }
    this->remove_present_symbol(_name);
    return true;
}

void Package::export_symbol(ObjectRef<Symbol> symbol)
{
    const std::string& _name = symbol->get_name();
    std::optional<Lookup> found = this->find_accessible_symbol(_name);
    aassert(found && found->symbol == symbol,
        "The symbol " << Alma::alma.to_string(symbol) << " is not accessible in the package " << this->name);
    if (found->status == Status::External)
        return;
    for (ObjectRef<Package> user : this->get_used_by_list())
        aassert(!user->conflicts(_name, symbol),
            "Exporting " << _name << " from " << this->name << " causes a name conflict in " << user->get_name());
    if (found->status == Status::Inherited)
        this->add_present_symbol(symbol);
    this->external_names.insert(_name);
}

void Package::unexport_symbol(ObjectRef<Symbol> symbol)
{
    const std::string& _name = symbol->get_name();
    aassert(!this->keyword, "Cannot unexport a keyword");
    std::optional<ObjectRef<Symbol>> present = this->find_present_symbol(_name);
    aassert(present && *present == symbol,
        "The symbol " << Alma::alma.to_string(symbol) << " is not present in the package " << this->name);
    this->external_names.erase(_name);
}

void Package::import_symbol(ObjectRef<Symbol> symbol)
{
    const std::string& _name = symbol->get_name();
    std::optional<Lookup> found = this->find_accessible_symbol(_name);
    if (found && found->symbol == symbol) {
        if (found->status == Status::Inherited)
            this->add_present_symbol(symbol);
        return;
    }
    aassert(!found,
        "Importing " << Alma::alma.to_string(symbol) << " into " << this->name
                     << " causes a name conflict with " << Alma::alma.to_string(found->symbol));
    this->add_present_symbol(symbol);
}

void Package::shadowing_import_symbol(ObjectRef<Symbol> symbol)
{
    const std::string& _name = symbol->get_name();
    std::optional<ObjectRef<Symbol>> present = this->find_present_symbol(_name);
    if (present && *present != symbol)
        this->remove_present_symbol(_name);
    if (!present || *present != symbol)
        this->add_present_symbol(symbol);
    this->shadowing_names.insert(_name);
}

void Package::shadow(const std::string& _name)
{
    if (!this->find_present_symbol(_name))
        this->add_present_symbol(Alma::alma.make<Symbol>(_name));
    this->shadowing_names.insert(_name);
}

std::vector<ObjectRef<Symbol>> Package::get_present_symbols()
{
    std::vector<ObjectRef<Symbol>> result;
    for (auto& [_name, symbol] : this->symbols)
        result.push_back(symbol);
    return result;
}

std::vector<ObjectRef<Symbol>> Package::get_shadowing_symbols()
{
    std::vector<ObjectRef<Symbol>> result;
    for (const std::string& _name : this->shadowing_names)
        result.push_back(*this->find_present_symbol(_name));
    return result;
}

// -----------------------------------------------------------------------------
// Use list

void Package::use_package(ObjectRef<Package> package)
{
    aassert(package.get() != this, "The package " << this->name << " cannot use itself");
    for (ObjectRef<Package> used : this->get_use_list())
        if (used == package)
            return;
    for (const std::string& _name : package->external_names)
        aassert(!this->conflicts(_name, *package->find_present_symbol(_name)),
            "Using " << package->get_name() << " from " << this->name << " causes a name conflict with " << _name);
    for (char c : package->external_character_macros)
        aassert(!this->character_macro_conflicts(c, *package->find_present_character_macro(c)),
            "Using " << package->get_name() << " from " << this->name << " causes a conflict with the character macro "
                     << character_name(c));
    this->use_list.emplace_back(*this, package);
    package->used_by_list.emplace_back(*package, ObjectRef<Package>(this));
}

void Package::unuse_package(ObjectRef<Package> package)
{
    std::erase_if(this->use_list, [&](const ObjectTrackedKeyRef<Package>& used) { return used.get() == package.get(); });
    std::erase_if(package->used_by_list, [&](const ObjectTrackedKeyRef<Package>& user) { return user.get() == this; });
}

std::vector<ObjectRef<Package>> Package::get_use_list()
{
    std::vector<ObjectRef<Package>> result;
    for (const ObjectTrackedKeyRef<Package>& used : this->use_list)
        result.push_back(used);
    return result;
}

std::vector<ObjectRef<Package>> Package::get_used_by_list()
{
    std::vector<ObjectRef<Package>> result;
    for (const ObjectTrackedKeyRef<Package>& user : this->used_by_list)
        result.push_back(user);
    return result;
}

// -----------------------------------------------------------------------------
// Local names

void Package::add_local_name(const std::string& local_name, ObjectRef<Package> package)
{
    aassert(!local_name.empty(), "A local name cannot be empty");
    std::optional<ObjectRef<Package>> current = this->find_local_name(local_name);
    aassert(!current || *current == package,
        "The local name " << local_name << " already denotes the package " << (*current)->get_name() << " in "
                          << this->name);
    if (!current)
        this->local_names.try_emplace(local_name, *this, package);
}

bool Package::remove_local_name(const std::string& local_name)
{
    return this->local_names.erase(local_name) > 0;
}

std::optional<ObjectRef<Package>> Package::find_local_name(const std::string& local_name)
{
    auto it = this->local_names.find(local_name);
    if (it == this->local_names.end())
        return std::nullopt;
    return ObjectRef<Package>(it->second);
}

std::vector<std::pair<std::string, ObjectRef<Package>>> Package::get_local_names()
{
    std::vector<std::pair<std::string, ObjectRef<Package>>> result;
    for (auto& [local_name, package] : this->local_names)
        result.emplace_back(local_name, package);
    return result;
}

void Package::destroy()
{
    for (ObjectRef<Package> user : this->get_used_by_list())
        user->unuse_package(ObjectRef<Package>(this));
    for (ObjectRef<Package> used : this->get_use_list())
        this->unuse_package(used);
    for (ObjectRef<Symbol> symbol : this->get_present_symbols()) {
        std::optional<ObjectRef<Package>> home = symbol->get_home_package();
        if (home && home->get() == this)
            symbol->clear_home_package();
    }
    this->local_names.clear();
    this->deleted = true;
}

// -----------------------------------------------------------------------------
// Character macros

bool Package::character_macro_conflicts(char c, ObjectRef<Procedure> proc)
{
    if (this->find_present_character_macro(c))
        return false;
    std::optional<ObjectRef<Procedure>> inherited = this->find_character_macro(c);
    return inherited && *inherited != proc;
}

std::optional<ObjectRef<Procedure>> Package::find_present_character_macro(char c)
{
    auto it = this->character_macros.find(c);
    if (it == this->character_macros.end())
        return std::nullopt;
    return ObjectRef<Procedure>(it->second);
}

std::optional<ObjectRef<Procedure>> Package::find_external_character_macro(char c)
{
    if (!this->external_character_macros.contains(c))
        return std::nullopt;
    return this->find_present_character_macro(c);
}

std::optional<ObjectRef<Procedure>> Package::find_character_macro(char c)
{
    std::optional<ObjectRef<Procedure>> present = this->find_present_character_macro(c);
    if (present)
        return present;
    for (ObjectRef<Package> used : this->get_use_list()) {
        std::optional<ObjectRef<Procedure>> inherited = used->find_external_character_macro(c);
        if (inherited)
            return inherited;
    }
    return std::nullopt;
}

ObjectRef<Procedure> Package::intern_character_macro(char c, ObjectRef<Procedure> proc)
{
    if (!this->character_macros.contains(c))
        this->character_macros.try_emplace(c, *this, proc);
    return proc;
}

void Package::set_character_macro(char c, ObjectRef<Procedure> proc)
{
    if (this->external_character_macros.contains(c))
        for (ObjectRef<Package> user : this->get_used_by_list())
            aassert(!user->character_macro_conflicts(c, proc),
                "Defining the exported character macro " << character_name(c) << " in " << this->name
                                                         << " causes a conflict in " << user->get_name());
    auto it = this->character_macros.find(c);
    if (it != this->character_macros.end())
        it->second = proc;
    else
        this->character_macros.try_emplace(c, *this, proc);
}

void Package::export_character_macro(char c)
{
    std::optional<ObjectRef<Procedure>> proc = this->find_character_macro(c);
    aassert(proc, "The character macro " << character_name(c) << " is not defined in the package " << this->name);
    if (this->external_character_macros.contains(c))
        return;
    for (ObjectRef<Package> user : this->get_used_by_list())
        aassert(!user->character_macro_conflicts(c, *proc),
            "Exporting the character macro " << character_name(c) << " from " << this->name
                                             << " causes a conflict in " << user->get_name());
    if (!this->find_present_character_macro(c))
        this->character_macros.try_emplace(c, *this, *proc);
    this->external_character_macros.insert(c);
}

void Package::unexport_character_macro(char c)
{
    aassert(this->find_present_character_macro(c),
        "The character macro " << character_name(c) << " is not defined in the package " << this->name);
    this->external_character_macros.erase(c);
}

void Package::import_character_macro(char c, ObjectRef<Procedure> proc)
{
    std::optional<ObjectRef<Procedure>> present = this->find_present_character_macro(c);
    if (present) {
        aassert(*present == proc,
            "Importing the character macro " << character_name(c) << " into " << this->name
                                             << " causes a conflict with its own definition");
        return;
    }
    this->set_character_macro(c, proc);
}
