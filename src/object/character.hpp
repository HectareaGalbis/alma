
#pragma once

#include "object.hpp"

class Character : public Object {
private:
    char c;

public:
    Character(Alma& alma, char c);

    virtual std::string to_string() override;
    virtual bool typep(ObjectRef<Object> self, ObjectRef<Object> type) override;
};
