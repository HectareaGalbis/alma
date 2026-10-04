
#pragma once

#include "procedure.hpp"

// void intern_functions();

#define declare_function(name)                              \
    class name : public Function {                          \
    public:                                                 \
        name()                                              \
            : Function()                                    \
        {                                                   \
        }                                                   \
                                                            \
    protected:                                              \
        virtual ObjectRef<Object> eval_body(                \
            const std::vector<ObjectRef<Object>>& arg_list, \
            ObjectRef<Environment> enviroment) override;    \
    };

declare_function(Sum);
declare_function(Print);
declare_function(Type);
declare_function(Set_symbol_value);
declare_function(Setq);
declare_function(Car);
declare_function(Cdr);
declare_function(Eq);
declare_function(Eval);
