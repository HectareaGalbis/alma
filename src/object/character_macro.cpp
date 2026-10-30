
#include "character_macro.hpp"
#include "alma.hpp"
#include "interfaces/transformable.hpp"

CharacterMacro::CharacterMacro(ObjectRef<Procedure> _transformer, ObjectRef<Object> _obj)
    : transformer(*this, _transformer)
    , obj(*this, _obj)
{
}

ObjectRef<Object> CharacterMacro::expand(
    ObjectRef<Object> self [[maybe_unused]], ObjectRef<Environment> enviroment)
{
    // The transformer receives the object without evaluating it, and its result is the expansion
    ObjectRef<Object> proc = this->transformer;
    if (Alma::alma.typep(proc, Alma::alma.intern_alma_symbol("function")))
        return Alma::alma.funcall(proc, { this->obj });
    if (dynamic_cast<Transformable*>(proc.get()))
        return Alma::alma.transform(proc, std::vector<ObjectRef<Object>> { this->obj }, enviroment);
    return Alma::alma.apply(proc, std::vector<ObjectRef<Object>> { this->obj }, enviroment);
}

std::string CharacterMacro::to_string(ObjectRef<Object> self [[maybe_unused]])
{
    return "<character-macro>";
}

ObjectRef<Object> CharacterMacro::type(ObjectRef<Object> self [[maybe_unused]]) const
{
    return Alma::alma.intern_alma_symbol("character-macro");
}
