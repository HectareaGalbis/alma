
#include "type_system.hpp"
#include "alma.hpp"
#include "debug.hpp"

static ObjectRef<Symbol> intern_type(const std::string& name)
{
    return Alma::alma.intern_alma_symbol(name);
}

TypeSystem::TypeSystem()
{
    ObjectRef<Symbol> t = intern_type("t");
    this->types.try_emplace(ObjectTrackedKeyRef<Symbol>(*this, t), *this, t);

    ObjectRef<Symbol> object = intern_type("object");
    this->insert_type(object, t);
    for (const std::string name : { "character", "character-macro", "cons", "environment",
             "integer", "package", "string", "symbol", "type-system" })
        this->insert_type(intern_type(name), object);

    ObjectRef<Symbol> procedure = intern_type("procedure");
    this->insert_type(procedure, object);
    ObjectRef<Symbol> function = intern_type("function");
    this->insert_type(function, procedure);
    ObjectRef<Symbol> macro = intern_type("macro");
    this->insert_type(macro, procedure);
    this->insert_type(intern_type("function-user"), function);
    this->insert_type(intern_type("macro-user"), macro);
}

ObjectRef<Object> TypeSystem::type(ObjectRef<Object> self [[maybe_unused]]) const
{
    return Alma::alma.intern_alma_symbol("type-system");
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

bool TypeSystem::subtypep(ObjectRef<Symbol> type, ObjectRef<Symbol> supertype)
{
    ObjectRef<Symbol> t = Alma::alma.intern_alma_symbol("t");

    while (type != t) {
        if (type == supertype)
            return true;

        auto parent = this->types.find(type);
        massert(parent != this->types.end(),
            "The type " << type->get_name() << " does not exist");
        type = parent->second;
    }

    return supertype == t;
}
