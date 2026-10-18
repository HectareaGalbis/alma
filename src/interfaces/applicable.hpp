#pragma once

#include <vector>

class Object;
class Environment;
template <typename T>
class ObjectRef;

class Applicable {
public:
    virtual ObjectRef<Object> apply(ObjectRef<Object> self,
        const std::vector<ObjectRef<Object>>& arg_list, ObjectRef<Environment> environment)
        = 0;
};
