#pragma once

#include "../object/object.hpp"
#include "../object/procedure.hpp"
#include "emitter.hpp"
#include <string>
#include <vector>

/*
Preprocessor of emit. A macro is defined with alma code:

  (emit '(macro square (x) `(* ,x ,x)))
  (emit '(macro unless (condition &rest body) `(if (not ,condition) (block ,@body))))

emit is a function, so the code it receives must be quoted. The top level emitters (function,
variable, struct, union and macro) are also alma macros, defined in object/macro.hpp, that expand
to a call to emit, so (macro square (x) ...) is the same as (emit '(macro square (x) ...)).

Before emitting an object, every list that starts with the name of a macro is replaced by the
result of calling the macro with the elements of the list as arguments, without evaluating them.
As in the C preprocessor:
  - The arguments are expanded before calling the macro.
  - The result is expanded again, but a macro is not expanded inside its own expansion, so
    (macro printf (x) `(printf "%d\n" ,x)) calls the C function printf.
  - A macro has preference over a function or an emitter with the same name.
  - The definitions of macros are not expanded.
*/

class Environment;
class FunctionUser;

/** A macro of the preprocessor. It is stored in the property 'macro' of its symbol. */
class PreprocessorMacro : public Object {
private:
    ObjectTrackedRef<FunctionUser> function;
    ObjectTrackedRef<Environment> environment;

public:
    PreprocessorMacro(ObjectRef<FunctionUser> function, ObjectRef<Environment> environment);

    /** Call the macro with the arguments without evaluating them */
    ObjectRef<Object> expand(const std::vector<ObjectRef<Object>>& arg_list);

    virtual std::string to_string(ObjectRef<Object> self) override;
    virtual ObjectRef<Object> type(ObjectRef<Object> self) const override;
};

/** (macro name (parameter... [&rest parameter]) body...) defines a macro of the preprocessor.
    It writes nothing. */
class EmitterMacro : public Emitter {
public:
    EmitterMacro();

    void emit_top_level(ObjectRef<Object> self, const std::vector<ObjectRef<Object>>& arg_list,
        ObjectRef<Environment> environment, Declarations& declarations, std::stringstream& output) override;
};

/** Expand every macro of an object */
ObjectRef<Object> expand_macros(ObjectRef<Object> obj);

/** Associate the emitter macro to its symbol */
void intern_preprocessor();
