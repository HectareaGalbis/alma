
#pragma once

#include "procedure.hpp"

void intern_macros();

#define declare_macro(name)                                                   \
    class name : public Macro {                                               \
    public:                                                                   \
        template <typename Name>                                              \
        name(Name&& _n)                                                       \
            : Macro(std::forward<Name>(_n))                                   \
        {                                                                     \
        }                                                                     \
                                                                              \
    protected:                                                                \
        virtual ObjectRef<Object> eval_body(                                  \
            const std::vector<ObjectRef<Object>>& args, Alma& alma) override; \
    }

declare_macro(defun);
declare_macro(defmacro);
