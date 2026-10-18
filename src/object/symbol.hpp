
#pragma once

#include "interfaces/emittable.hpp"
#include "object.hpp"
#include <string>
#include <unordered_map>
#include <vector>

class Package;
class Environment;
class Emitter;

class Symbol : public Object, public Emittable {
private:
    std::string name;
    std::unordered_map<ObjectTrackedKeyRef<Symbol>, std::vector<ObjectTrackedKeyRef<Object>>,
        ObjectRefHash, ObjectRefEqual>
        properties;

public:
    Symbol(const std::string& name);

    virtual ObjectRef<Object> eval(ObjectRef<Object> self, ObjectRef<Environment> enviroment)
        override;
    virtual std::string to_string(ObjectRef<Object> self) override;
    virtual ObjectRef<Object> type(ObjectRef<Object> self) const override;

    virtual void emit_top_level(ObjectRef<Object> self, ObjectRef<Environment> environment,
        Declarations& declarations, std::stringstream& output) override;
    virtual void emit_statement(ObjectRef<Object> self, ObjectRef<Environment> environment,
        Declarations& declarations, std::stringstream& output) override;
    virtual std::unique_ptr<Type> emit_expression(ObjectRef<Object> self, ObjectRef<Environment> environment,
        Declarations& declarations, std::stringstream& output) override;

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

    bool has_emitter();
    ObjectRef<Emitter> get_emitter();
    void set_emitter(ObjectRef<Emitter> emitter);
    void push_emitter(ObjectRef<Emitter> emitter);
    void pop_emitter();
};

// void intern_symbols();
