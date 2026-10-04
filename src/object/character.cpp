
#include "character.hpp"
#include "alma.hpp"

Character::Character(char _c)
    : c(_c)
{
}

std::string Character::to_string(ObjectRef<Object> self [[maybe_unused]])
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

ObjectRef<Object> Character::type(ObjectRef<Object> self [[maybe_unused]]) const
{
    return Alma::alma.intern_alma_symbol("character");
}
