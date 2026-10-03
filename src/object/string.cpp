
#include "string.hpp"
#include "alma.hpp"

String::String(Alma& _alma, const std::string& _content)
    : Object(_alma)
    , content(_content)
{
}

std::string String::to_string()
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

bool String::typep(ObjectRef<Object> self, ObjectRef<Object> type)
{
    return type == this->alma.intern_alma_symbol("string") || this->Object::typep(self, type);
}
