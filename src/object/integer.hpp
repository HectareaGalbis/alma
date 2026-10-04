
#pragma once

#include "object.hpp"

// Integer
class Integer : public Object {
private:
    int64_t value;

public:
    Integer(int64_t _value);

    virtual std::string to_string(ObjectRef<Object> self) override;
    virtual ObjectRef<Object> type(ObjectRef<Object> self) const override;

    int64_t get_value() const;
};
