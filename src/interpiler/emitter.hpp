#pragma once

#include "../object/object.hpp"
#include "declaration.hpp"
#include "type.hpp"
#include <memory>
#include <sstream>
#include <string>
#include <vector>

/*
Eval es la combinacion de expand y reduce
    Eval es la union entre interfaz e implementacion
        Eval es la composicion de un anamorfismo y un catamorfismo

            Cambiar el sistema de evaluacion de alma
                Dado un objeto,
    primero expandirlo y luego performarlo.
*/

class Environment;

/** An emitter is associated to a symbol and emits the lists whose first element is that symbol.
    By default it cannot be emitted anywhere; each emitter overrides the positions it supports. */
class Emitter : public Object {
private:
    std::string name;

public:
    Emitter(const std::string& name);

    const std::string& get_name() const;

    virtual void emit_top_level(ObjectRef<Object> self, const std::vector<ObjectRef<Object>>& arg_list,
        ObjectRef<Environment> environment, Declarations& declarations, std::stringstream& output);

    /** By default, emit the expression followed by a semicolon */
    virtual void emit_statement(ObjectRef<Object> self, const std::vector<ObjectRef<Object>>& arg_list,
        ObjectRef<Environment> environment, Declarations& declarations, std::stringstream& output);

    virtual std::unique_ptr<Type> emit_expression(ObjectRef<Object> self, const std::vector<ObjectRef<Object>>& arg_list,
        ObjectRef<Environment> environment, Declarations& declarations, std::stringstream& output);

    virtual std::string to_string(ObjectRef<Object> self) override;
    virtual ObjectRef<Object> type(ObjectRef<Object> self) const override;
};

/** (declare printf (int ((char *) &rest))) -> int printf(char *, ...);
    (declare counter int) -> extern int counter; */
class EmitterDeclare : public Emitter {
public:
    EmitterDeclare();

    void emit_top_level(ObjectRef<Object> self, const std::vector<ObjectRef<Object>>& arg_list,
        ObjectRef<Environment> environment, Declarations& declarations, std::stringstream& output) override;
};

/** (function main int ((argc int) (argv (char * *))) body...) */
class EmitterFunction : public Emitter {
public:
    EmitterFunction();

    void emit_top_level(ObjectRef<Object> self, const std::vector<ObjectRef<Object>>& arg_list,
        ObjectRef<Environment> environment, Declarations& declarations, std::stringstream& output) override;
};

/** (return) or (return expression) */
class EmitterReturn : public Emitter {
public:
    EmitterReturn();

    void emit_statement(ObjectRef<Object> self, const std::vector<ObjectRef<Object>>& arg_list,
        ObjectRef<Environment> environment, Declarations& declarations, std::stringstream& output) override;
};

/** Associate each emitter to its symbol */
void intern_emitters();
