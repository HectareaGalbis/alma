
#include "type_system.hpp"
#include "alma.hpp"
#include "debug.hpp"

TypeSystem::TypeSystem()
{
    ObjectRef<Symbol> t = Alma::alma.intern_alma_symbol("t");
    this->types.try_emplace(ObjectTrackedKeyRef<Symbol>(*this, t), *this, t);
}

void TypeSystem::insert_type(ObjectRef<Symbol> type, ObjectRef<Symbol> parent)
{
    aassert(this->types.contains(parent),
        "The type " << parent->get_name() << " does not exist");
    this->types.try_emplace(ObjectTrackedKeyRef<Symbol>(*this, type), *this, parent);
}

bool TypeSystem::has_type(ObjectRef<Symbol> type)
{
    return this->types.contains(type);
}
