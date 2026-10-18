#pragma once

#include <memory>
#include <sstream>

class Object;
class Environment;
class Declarations;
class Type;
template <typename T>
class ObjectRef;

class Emittable {
public:
    virtual ~Emittable() = default;

    /** Emit the object as a top level C construction (a function definition, an include...) */
    virtual void emit_top_level(ObjectRef<Object> self, ObjectRef<Environment> environment,
        Declarations& declarations, std::stringstream& output)
        = 0;

    /** Emit the object as a C statement */
    virtual void emit_statement(ObjectRef<Object> self, ObjectRef<Environment> environment,
        Declarations& declarations, std::stringstream& output)
        = 0;

    /** Emit the object as a C expression and return its type */
    virtual std::unique_ptr<Type> emit_expression(ObjectRef<Object> self, ObjectRef<Environment> environment,
        Declarations& declarations, std::stringstream& output)
        = 0;
};
