
#pragma once

#include "procedure.hpp"

void intern_macros();

#define declare_macro(name)                                                   \
    class name : public Macro {                                               \
    public:                                                                   \
        name()                                                                \
            : Macro()                                                         \
        {                                                                     \
        }                                                                     \
                                                                              \
    protected:                                                                \
        virtual ObjectRef<Object> eval_body(                                  \
            const std::vector<ObjectRef<Object>>& args, Alma& alma) override; \
    }

declare_macro(defun);
declare_macro(defmacro);
