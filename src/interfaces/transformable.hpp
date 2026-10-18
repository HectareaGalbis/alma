#pragma once

#include <vector>

class Object;
class Environment;
template <typename T>
class ObjectRef;

class Transformable {
public:
    virtual ObjectRef<Object> transform(ObjectRef<Object> self,
        const std::vector<ObjectRef<Object>>& arg_list, ObjectRef<class Environment> environment)
        = 0;
};
