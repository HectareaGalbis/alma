
#pragma once

#include "object.hpp"

class Instruction : public Object {
public:
    virtual void emit(const std::vector<ObjectRef<Object>>& arg_list) = 0;
};
