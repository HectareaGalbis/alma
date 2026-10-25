
#pragma once

#include "procedure.hpp"

void intern_macros();

#define declare_macro(name)                                     \
    class name : public Macro {                                 \
    public:                                                     \
        name()                                                  \
            : Macro()                                           \
        {                                                       \
        }                                                       \
                                                                \
    protected:                                                  \
        virtual ObjectRef<Object> eval_body(                    \
            const std::vector<ObjectRef<Object>>& arg_list,     \
            ObjectRef<class Environment> environment) override; \
    }

declare_macro(defun);
declare_macro(defmacro);
declare_macro(defspecial);

// (function ...), (macro ...), (variable ...), (struct ...), (union ...) and (enum ...) expand to (emit '(...))
declare_macro(emit_function);
declare_macro(emit_macro);
declare_macro(emit_variable);
declare_macro(emit_struct);
declare_macro(emit_union);
declare_macro(emit_enum);
