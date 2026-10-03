
#include "object.hpp"
#include "alma.hpp"
#include "debug.hpp"
#include "package.hpp"
#include "util.hpp"
#include <iostream>
#include <optional>

void Object::protect_object(GCObject** object)
{
    Alma::alma.gc.track_root_object(object);
}

void Object::unprotect_object(GCObject** object)
{
    Alma::alma.gc.untrack_root_object(object);
}

ObjectRef<Object> Object::expand(ObjectRef<Object> self, ObjectRef<Environment> enviroment [[maybe_unused]])
{
    return self;
}

ObjectRef<Object> Object::transform(
    ObjectRef<Object> self [[maybe_unused]],
    const std::vector<ObjectRef<Object>>& arg_list [[maybe_unused]],
    ObjectRef<Environment> enviroment [[maybe_unused]])
{
    athrow("The object " << this->to_string(self) << " is not transformable");
}

ObjectRef<Object> Object::eval(
    ObjectRef<Object> self, ObjectRef<Environment> environment [[maybe_unused]])
{
    return self;
}

ObjectRef<Object> Object::apply(
    ObjectRef<Object> self [[maybe_unused]],
    const std::vector<ObjectRef<Object>>& arg_list [[maybe_unused]],
    ObjectRef<Environment> enviroment [[maybe_unused]])
{
    athrow("The object " << this->to_string(self) << " is not applicable");
}

std::string Object::to_string(ObjectRef<Object> self [[maybe_unused]])
{
    return "<object>";
}

Object::operator bool()
{
    return this != this->alma.intern_alma_symbol("nil").get();
}
