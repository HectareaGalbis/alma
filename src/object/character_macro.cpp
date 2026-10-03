
#include "character_macro.hpp"
#include "alma.hpp"

CharacterMacro::CharacterMacro(
    Alma& _alma, ObjectRef<Procedure> _transformer, ObjectRef<Object> _obj)
    : Object(_alma)
    , transformer(*this, _transformer)
    , obj(*this, _obj)
{
}

ObjectRef<Object> CharacterMacro::expand(
    ObjectRef<Object> self [[maybe_unused]], ObjectRef<Environment> enviroment)
{
    return this->alma.apply(this->transformer, std::vector<ObjectRef<Object>> { this->obj }, enviroment);
}

ObjectRef<Object> CharacterMacro::eval(ObjectRef<Object> self, ObjectRef<Environment> enviroment)
{
    return this->alma.eval(this->expand(self, enviroment));
}

std::string CharacterMacro::to_string()
{
    return "<character-macro>";
}

bool CharacterMacro::typep(ObjectRef<Object> self, ObjectRef<Object> type)
{
    return type == this->alma.intern_alma_symbol("character-macro") || this->Object::typep(self, type);
}
