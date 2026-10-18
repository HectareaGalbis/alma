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

/** (function main int ((argc int) (argv (char * *))) body...) defines a function.
    (function f void () ()) defines a function with an empty body.
    Without body, it only declares it: (function f int ((format (char *)) &rest)) -> int f(char *format, ...);
    Redeclaring a function of the C standard library writes nothing. */
class EmitterFunction : public Emitter {
public:
    EmitterFunction();

    void emit_top_level(ObjectRef<Object> self, const std::vector<ObjectRef<Object>>& arg_list,
        ObjectRef<Environment> environment, Declarations& declarations, std::stringstream& output) override;
};

/** At the top level, (variable counter int 0) defines a global variable: int counter = 0;
    The value must be a constant. Without value, it only declares it: (variable counter int) -> extern int counter;
    Redeclaring a variable of the C standard library writes nothing.
    Inside a function it defines a local variable, with or without value: int counter = 0; or int counter; */
class EmitterVariable : public Emitter {
public:
    EmitterVariable();

    void emit_top_level(ObjectRef<Object> self, const std::vector<ObjectRef<Object>>& arg_list,
        ObjectRef<Environment> environment, Declarations& declarations, std::stringstream& output) override;
    void emit_statement(ObjectRef<Object> self, const std::vector<ObjectRef<Object>>& arg_list,
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
