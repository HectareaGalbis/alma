
#pragma once

#include "../object/object.hpp"
#include "../object/procedure.hpp"
#include "declaration.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

Eval es la combinacion de expand y reduce
    Eval es la union entre interfaz e implementacion
        Eval es la composicion de un anamorfismo y un catamorfismo

            Cambiar el sistema de evaluacion de alma
                Dado un objeto,
    primero expandirlo y luego performarlo.

    class Emitter : public Object {
    friend class EmitterTopLevel;
    friend class EmitterNonTopLevel;

private:
    virtual void emit_top_level(ObjectRef<Object> self,
        const std::vector<ObjectRef<Object>>& arg_list, ObjectRef<class Environment> environment,
        std::stringstream& output)
        = 0;

public:
    virtual void emit_non_top_level(ObjectRef<Object> self,
        const std::vector<ObjectRef<Object>>& arg_list, ObjectRef<class Environment> environment,
        std::stringstream& output)
        = 0;

protected:
    virtual void emit_impl(ObjectRef<Object> self,
        const std::vector<ObjectRef<Object>>& arg_list, ObjectRef<class Environment> environment,
        std::stringstream& output)
        = 0;

public:
    void handle(ObjectRef<Object> self,
        const std::vector<ObjectRef<Object>>& arg_list, ObjectRef<class Environment> environment,
        std::stringstream& output) override final;
};

class EmitterTopLevel : public Emitter {
private:
    void emit_top_level(ObjectRef<Object> self,
        const std::vector<ObjectRef<Object>>& arg_list, ObjectRef<class Environment> environment,
        std::stringstream& output) override final;

public:
    void emit_non_top_level(ObjectRef<Object> self,
        const std::vector<ObjectRef<Object>>& arg_list, ObjectRef<class Environment> environment,
        std::stringstream& output) override final;
};

class EmitterNonTopLevel : private Emitter {
private:
    void emit_top_level(ObjectRef<Object> self,
        const std::vector<ObjectRef<Object>>& arg_list, ObjectRef<class Environment> environment,
        std::stringstream& output) override final;

public:
    void emit_non_top_level(ObjectRef<Object> self,
        const std::vector<ObjectRef<Object>>& arg_list, ObjectRef<class Environment> environment,
        std::stringstream& output) override final;
};

class EmitterExpression : public EmitterNonTopLevel {
public:
    virtual std::unique_ptr<Type> get_type() = 0;
};
