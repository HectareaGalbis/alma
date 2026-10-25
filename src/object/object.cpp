
#include "object.hpp"
#include "alma.hpp"
#include "debug.hpp"
#include "package.hpp"
#include "util.hpp"
#include <iostream>
#include <optional>

void Object::protect_object(GCNode& node)
{
    Alma::alma.gc.track_root_object(node);
}

void Object::unprotect_object(GCNode& node)
{
    Alma::alma.gc.untrack_root_object(node);
}

ObjectRef<Object> Object::eval(ObjectRef<Object> self, ObjectRef<class Environment> environment)
{
    ObjectRef<Object> expansion = Alma::alma.expand(self, environment);
    if (expansion == self)
        return self;
    return Alma::alma.eval(expansion, environment);
}

std::string Object::to_string(ObjectRef<Object> self [[maybe_unused]])
{
    return "<object>";
}

ObjectRef<Object> Object::type(ObjectRef<Object> self [[maybe_unused]]) const
{
    return Alma::alma.intern_alma_symbol("object");
}

Object::operator bool()
{
    return this != Alma::alma.intern_alma_symbol("nil").get();
}
