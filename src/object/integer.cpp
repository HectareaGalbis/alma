
#include "integer.hpp"
#include "alma.hpp"

Integer::Integer(int64_t _value)
    : value(_value)
{
}

std::string Integer::to_string(ObjectRef<Object> self [[maybe_unused]])
{
    return std::to_string(this->value);
}

int64_t Integer::get_value() const
{
    return this->value;
}
