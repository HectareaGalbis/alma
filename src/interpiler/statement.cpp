
#include "statement.hpp"

std::string Declaration::to_string() const
{
    return (type->is_function() ? "" : "extern ") + type->to_string(name) + ";";
}
