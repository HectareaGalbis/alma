
#pragma once

#include "object.hpp"
#include <optional>
#include <string>
#include <vector>

class Package;

class Symbol : public Object {
private:
    std::string name;
    std::unordered_map<ObjectTrackedKeyRef<Symbol>, std::vector<ObjectTrackedKeyRef<Object>>,
        ObjectRefHash, ObjectRefEqual>
        properties;

public:
    Symbol(Alma& alma, const std::string& name);

    virtual ObjectRef<Object> eval(ObjectRef<Object> self, ObjectRef<Environment> enviroment)
        override;
    virtual std::string to_string() override;
    virtual bool typep(ObjectRef<Object> self, ObjectRef<Object> type) override;

    std::string& get_name();
    const std::string& get_name() const;

    bool has_property(ObjectRef<Symbol> property);
    ObjectRef<Object> get_property(ObjectRef<Symbol> property);
    void set_property(ObjectRef<Symbol> property, ObjectRef<Object> value);
    void push_property(ObjectRef<Symbol> property, ObjectRef<Object> value);
    void pop_property(ObjectRef<Symbol> property);

    bool has_value();
    ObjectRef<Object> get_value();
    void set_value(ObjectRef<Object> value);
    void push_value(ObjectRef<Object> value);
    void pop_value();

    bool has_package();
    ObjectRef<Package> get_package();
    void set_package(ObjectRef<Package> package);
    void push_package(ObjectRef<Package> package);
    void pop_package();
};

// void intern_symbols();
