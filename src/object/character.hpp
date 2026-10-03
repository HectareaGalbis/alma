
#pragma once

#include "object.hpp"

class Character : public Object {
private:
    char c;

public:
    Character(char c);

    virtual std::string to_string(ObjectRef<Object> self) override;
};
