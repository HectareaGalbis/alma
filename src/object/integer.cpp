
#include "integer.hpp"
#include "alma.hpp"
#include "debug.hpp"
#include "interpiler/type.hpp"
#include <cstdint>

Integer::Integer(int64_t _value)
    : value(_value)
{
}

std::string Integer::to_string(ObjectRef<Object> self [[maybe_unused]])
{
    return std::to_string(this->value);
}

ObjectRef<Object> Integer::type(ObjectRef<Object> self [[maybe_unused]]) const
{
    return Alma::alma.intern_alma_symbol("integer");
}

int64_t Integer::get_value() const
{
    return this->value;
}

void Integer::emit_top_level(ObjectRef<Object> self, ObjectRef<Environment> environment [[maybe_unused]],
    Declarations& declarations [[maybe_unused]], std::stringstream& output [[maybe_unused]])
{
    athrow("The integer " << this->to_string(self) << " cannot be emitted at the top level");
}

void Integer::emit_statement(ObjectRef<Object> self, ObjectRef<Environment> environment,
    Declarations& declarations, std::stringstream& output)
{
    Alma::alma.emit_expression(self, environment, declarations, output);
    output << ";\n";
}

std::unique_ptr<Type> Integer::emit_expression(ObjectRef<Object> self [[maybe_unused]],
    ObjectRef<Environment> environment [[maybe_unused]],
    Declarations& declarations [[maybe_unused]], std::stringstream& output)
{
    if (this->value < 0)
        output << "(" << this->value << ")";
    else
        output << this->value;
    if (this->value >= INT32_MIN && this->value <= INT32_MAX)
        return std::make_unique<ValueType>("int");
    return std::make_unique<ValueType>("long");
}
