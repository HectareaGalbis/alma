
#pragma once

#include "object.hpp"

// Integer
class Integer : public Object {
private:
    int64_t value;

public:
    Integer(Alma& alma, int64_t _value);

    virtual std::string to_string(ObjectRef<Object> self) override;
    virtual bool typep(ObjectRef<Object> self, ObjectRef<Object> type) override;

    int64_t get_value() const;
};
