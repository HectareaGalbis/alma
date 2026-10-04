
#include "string.hpp"
#include "alma.hpp"

String::String(const std::string& _content)
    : content(_content)
{
}

std::string String::to_string(ObjectRef<Object> self [[maybe_unused]])
{
    std::string ss;
    ss.push_back('"');
    for (char c : this->content) {
        switch (c) {
        case '"':
            ss += "\\\"";
            break;
        case '\\':
            ss += "\\\\";
            break;
        default:
            ss.push_back(c);
            break;
        }
    }
    ss.push_back('"');
    return ss;
}

ObjectRef<Object> String::type(ObjectRef<Object> self [[maybe_unused]]) const
{
    return Alma::alma.intern_alma_symbol("string");
}
