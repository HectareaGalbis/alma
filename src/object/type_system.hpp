
#pragma once

#include "object.hpp"
#include "symbol.hpp"
#include <unordered_map>

class TypeSystem : public Object {
private:
    std::unordered_map<ObjectTrackedKeyRef<Symbol>, ObjectTrackedRef<Symbol>, ObjectRefHash, ObjectRefEqual> types;

public:
    TypeSystem();

    virtual ObjectRef<Object> type(ObjectRef<Object> self) const override;

    void insert_type(ObjectRef<Symbol> type, ObjectRef<Symbol> parent);
    bool has_type(ObjectRef<Symbol> type);
    bool subtypep(ObjectRef<Symbol> type, ObjectRef<Symbol> supertype);
};
