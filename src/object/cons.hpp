
#pragma once

#include "environment.hpp"
#include "interfaces/emittable.hpp"
#include "interfaces/expandable.hpp"
#include "object.hpp"
#include <vector>

class Cons : public Object, public Expandable, public Emittable {
private:
    ObjectTrackedRef<Object> car;
    ObjectTrackedRef<Object> cdr;

public:
    Cons(ObjectRef<Object> _car, ObjectRef<Object> _cdr);
    Cons(const std::vector<ObjectRef<Object>>& list);
    Cons(const std::vector<ObjectRef<Object>>& list, ObjectRef<Object> non_proper_element);

    std::pair<std::vector<ObjectRef<Object>>, ObjectRef<Object>> to_list() const;

    virtual ObjectRef<Object> expand(ObjectRef<Object> self, ObjectRef<Environment> enviroment)
        override;
    virtual ObjectRef<Object> eval(ObjectRef<Object> self, ObjectRef<Environment> enviroment)
        override;
    virtual void emit_top_level(ObjectRef<Object> self, ObjectRef<Environment> environment,
        Declarations& declarations, std::stringstream& output) override;
    virtual void emit_statement(ObjectRef<Object> self, ObjectRef<Environment> environment,
        Declarations& declarations, std::stringstream& output) override;
    virtual std::unique_ptr<Type> emit_expression(ObjectRef<Object> self, ObjectRef<Environment> environment,
        Declarations& declarations, std::stringstream& output) override;
    virtual std::string to_string(ObjectRef<Object> self) override;
    virtual ObjectRef<Object> type(ObjectRef<Object> self) const override;

    ObjectRef<Object> get_car();
    ObjectRef<Object> get_cdr();
};
