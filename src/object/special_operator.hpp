
#pragma once

#include "cons.hpp"
#include "procedure.hpp"

void intern_special_operators();

#define declare_special_operator(name)                          \
    class name : public Procedure {                             \
    public:                                                     \
        virtual ObjectRef<Object> eval_body(                    \
            const std::vector<ObjectRef<Object>>& arg_list,     \
            ObjectRef<class Environment> environment) override; \
    }

declare_special_operator(progn);
declare_special_operator(let);
declare_special_operator(quote);
declare_special_operator(quasiquote);
declare_special_operator(lambda);
declare_special_operator(gamma);
declare_special_operator(branch);
