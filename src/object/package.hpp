
#pragma once

#include "object.hpp"
#include "symbol.hpp"
#include <map>

class Procedure;

class Package : public Object {
private:
    std::map<std::string, ObjectTrackedRef<Symbol>> symbols;
    std::map<char, ObjectTrackedRef<Procedure>> character_macros;

public:
    Package(Alma& alma);
    virtual bool typep(ObjectRef<Object> self, ObjectRef<Object> type) override;

    // Symbols
    std::optional<ObjectRef<Object>> find_symbol(const std::string& name);
    ObjectRef<Symbol> intern_symbol(const std::string& name);

    // Character macros
    std::optional<ObjectRef<Procedure>> find_character_macro(char c);
    ObjectRef<Procedure> intern_character_macro(char c, ObjectRef<Procedure> proc);
};
