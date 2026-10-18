
#pragma once

class Object;
template <typename T>
class ObjectRef;
class Environment;

class Evaluable {
public:
    virtual ObjectRef<Object> eval(ObjectRef<Object> self, ObjectRef<Environment> environment) = 0;
};
