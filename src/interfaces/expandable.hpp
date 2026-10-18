#pragma once

class Object;
class Environment;
template <typename T>
class ObjectRef;

class Expandable {
public:
    virtual ObjectRef<Object> expand(ObjectRef<Object> self, ObjectRef<class Environment> environment) = 0;
};
