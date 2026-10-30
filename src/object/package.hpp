#pragma once

#include "object.hpp"
#include "symbol.hpp"
#include <map>
#include <optional>
#include <set>
#include <vector>

class Procedure;

class Package : public Object {
public:
    enum class Status { Internal,
        External,
        Inherited };

    struct Lookup {
        ObjectRef<Symbol> symbol;
        Status status;
    };

private:
    std::string name;
    bool deleted;
    bool keyword;
    std::map<std::string, ObjectTrackedRef<Symbol>> symbols;
    std::set<std::string> external_names;
    std::set<std::string> shadowing_names;
    std::vector<ObjectTrackedKeyRef<Package>> use_list;
    std::vector<ObjectTrackedKeyRef<Package>> used_by_list;
    std::map<std::string, ObjectTrackedRef<Package>> local_names;
    std::map<char, ObjectTrackedRef<Procedure>> character_macros;

    bool conflicts(const std::string& name, ObjectRef<Symbol> symbol);
    void add_present_symbol(ObjectRef<Symbol> symbol);
    void remove_present_symbol(const std::string& name);

public:
    Package(const std::string& name, bool keyword = false);
    virtual std::string to_string(ObjectRef<Object> self) override;
    virtual ObjectRef<Object> type(ObjectRef<Object> self) const override;

    // Name
    const std::string& get_name() const;
    void set_name(const std::string& name);
    bool is_deleted() const;
    bool is_keyword() const;

    // Symbols
    std::optional<ObjectRef<Symbol>> find_present_symbol(const std::string& name);
    std::optional<ObjectRef<Symbol>> find_external_symbol(const std::string& name);
    std::optional<Lookup> find_accessible_symbol(const std::string& name);
    std::optional<ObjectRef<Object>> find_symbol(const std::string& name);
    bool is_external(const std::string& name) const;
    ObjectRef<Symbol> intern_symbol(const std::string& name);
    /** Intern a symbol and export it without checking name conflicts */
    ObjectRef<Symbol> intern_external_symbol(const std::string& name);
    bool unintern_symbol(ObjectRef<Symbol> symbol);
    void export_symbol(ObjectRef<Symbol> symbol);
    void unexport_symbol(ObjectRef<Symbol> symbol);
    void import_symbol(ObjectRef<Symbol> symbol);
    void shadowing_import_symbol(ObjectRef<Symbol> symbol);
    void shadow(const std::string& name);
    std::vector<ObjectRef<Symbol>> get_present_symbols();
    std::vector<ObjectRef<Symbol>> get_shadowing_symbols();

    // Use list
    void use_package(ObjectRef<Package> package);
    void unuse_package(ObjectRef<Package> package);
    std::vector<ObjectRef<Package>> get_use_list();
    std::vector<ObjectRef<Package>> get_used_by_list();

    // Local names
    void add_local_name(const std::string& local_name, ObjectRef<Package> package);
    bool remove_local_name(const std::string& local_name);
    std::optional<ObjectRef<Package>> find_local_name(const std::string& local_name);
    std::vector<std::pair<std::string, ObjectRef<Package>>> get_local_names();

    /** Remove every relation with other packages and the home package of its symbols */
    void destroy();

    // Character macros
    std::optional<ObjectRef<Procedure>> find_character_macro(char c);
    ObjectRef<Procedure> intern_character_macro(char c, ObjectRef<Procedure> proc);
};
