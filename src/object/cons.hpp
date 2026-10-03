
#pragma once

#include "object.hpp"
#include <optional>

class Cons : public Object {
private:
    ObjectTrackedRef<Object> car;
    ObjectTrackedRef<Object> cdr;

public:
    Cons(Alma& alma, ObjectRef<Object> _car, ObjectRef<Object> _cdr);
    Cons(Alma& alma, const std::vector<ObjectRef<Object>>& list);
    Cons(Alma& alma, const std::vector<ObjectRef<Object>>& list, ObjectRef<Object> non_proper_element);

    std::pair<std::vector<ObjectRef<Object>>, ObjectRef<Object>> to_list() const;

    virtual ObjectRef<Object> expand(ObjectRef<Object> self, ObjectRef<Environment> enviroment)
        override;
    virtual ObjectRef<Object> eval(ObjectRef<Object> self, ObjectRef<Environment> enviroment)
        override;
    virtual std::string to_string() override;
    virtual bool typep(ObjectRef<Object> self, ObjectRef<Object> type) override;

    ObjectRef<Object> get_car();
    ObjectRef<Object> get_cdr();
};
