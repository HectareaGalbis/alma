
#include "character_macro.hpp"
#include "alma.hpp"

CharacterMacro::CharacterMacro(ObjectRef<Procedure> _transformer, ObjectRef<Object> _obj)
    : transformer(*this, _transformer)
    , obj(*this, _obj)
{
}

ObjectRef<Object> CharacterMacro::expand(
    ObjectRef<Object> self [[maybe_unused]], ObjectRef<Environment> enviroment)
{
    return Alma::alma.apply(this->transformer, std::vector<ObjectRef<Object>> { this->obj }, enviroment);
}

ObjectRef<Object> CharacterMacro::eval(ObjectRef<Object> self, ObjectRef<Environment> enviroment)
{
    return Alma::alma.eval(this->expand(self, enviroment));
}

std::string CharacterMacro::to_string(ObjectRef<Object> self [[maybe_unused]])
{
    return "<character-macro>";
}
