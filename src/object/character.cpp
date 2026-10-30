
#include "character.hpp"
#include "alma.hpp"

Character::Character(char _c)
    : c(_c)
{
}

static const std::pair<const char*, char> character_names[] = {
    { "space", ' ' },
    { "newline", '\n' },
    { "tab", '\t' },
    { "return", '\r' },
    { "backspace", '\b' },
    { "nul", '\0' },
};

std::optional<char> Character::from_name(const std::string& name)
{
    for (auto& [character_name, c] : character_names)
        if (name == character_name)
            return c;
    return std::nullopt;
}

std::optional<std::string> Character::name_of(char c)
{
    for (auto& [character_name, character] : character_names)
        if (c == character)
            return character_name;
    return std::nullopt;
}

std::string Character::to_string(ObjectRef<Object> self [[maybe_unused]])
{
    std::optional<std::string> name = Character::name_of(this->c);
    return "\\" + (name ? *name : std::string(1, this->c));
}

ObjectRef<Object> Character::type(ObjectRef<Object> self [[maybe_unused]]) const
{
    return Alma::alma.intern_alma_symbol("character");
}

char Character::get_char() const
{
    return this->c;
}
