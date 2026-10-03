
#include "object.hpp"
#include "alma.hpp"
#include "debug.hpp"
#include "package.hpp"
#include "util.hpp"
#include <iostream>
#include <optional>

void Object::protect_object(Alma& alma, GCObject** object)
{
    alma.gc.track_root_object(object);
}

void Object::unprotect_object(Alma& alma, GCObject** object)
{
    alma.gc.untrack_root_object(object);
}

Object::Object(Alma& _alma)
    : alma(_alma)
{
}

Object::Object(const Object& other)
    : alma(other.alma)
{
}

Object::Object(const Object&& other)
    : alma(other.alma)
{
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
    athrow("The object " << this->to_string() << " is not transformable");
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
    athrow("The object " << this->to_string() << " is not applicable");
}

std::string Object::to_string()
{
    return "<object>";
}

bool Object::typep(ObjectRef<Object> self [[maybe_unused]], ObjectRef<Object> type)
{
    return type == this->alma.intern_alma_symbol("t");
}

Object::operator bool()
{
    return this != this->alma.intern_alma_symbol("nil").get();
}
