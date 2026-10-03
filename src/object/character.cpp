
#include "character.hpp"
#include "alma.hpp"

Character::Character(Alma& _alma, char _c)
    : Object(_alma)
    , c(_c)
{
}

std::string Character::to_string()
{
    std::string str = ".";
    switch (this->c) {
    case '\n':
        str += "\\n";
        break;
    case '\b':
        str += "\\b";
        break;
    case '\t':
        str += "\\t";
        break;
    case '\\':
        str += "\\\\";
        break;
    default:
        str += this->c;
    }

    return str;
}

bool Character::typep(ObjectRef<Object> self, ObjectRef<Object> type)
{
    return type == this->alma.intern_alma_symbol("character") || this->Object::typep(self, type);
}
