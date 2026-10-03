
#pragma once

#include "object.hpp"

class Nil : public Object {
public:
    Nil();

    virtual std::string to_string(ObjectRef<Object> self [[maybe_unused]]) override
    {
        return "nil";
    }
};
