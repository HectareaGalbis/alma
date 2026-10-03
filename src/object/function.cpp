
#include "function.hpp"
#include "alma.hpp"
#include "cons.hpp"
#include "debug.hpp"
#include "integer.hpp"
#include "object.hpp"
#include "package.hpp"
#include <iostream>

// --------------------------------------------------------------------------------

ObjectRef<Object> Sum::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, {}, "integer");
    int64_t sum_value = 0;
    for (ObjectRef<Object> arg : arg_list) {
        sum_value += arg.as<Integer>()->get_value();
    }
    return this->alma.make<Integer>(sum_value);
}

// --------------------------------------------------------------------------------

ObjectRef<Object> Print::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "t" });
    std::cout << this->alma.to_string(arg_list.front()) << std::endl;
    return arg_list.front();
}

// --------------------------------------------------------------------------------

ObjectRef<Object> Typep::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "t", "symbol" });
    bool correct_type = this->alma.typep(arg_list[0], arg_list[1]);
    return this->alma.boolean(correct_type);
}

// --------------------------------------------------------------------------------

ObjectRef<Object> Set_symbol_value::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "symbol", "t" });
    return this->alma.set_symbol_value(arg_list[0], arg_list[1]);
}

// --------------------------------------------------------------------------------

ObjectRef<Object> Setq::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "symbol", "t" });
    return this->alma.setq(arg_list[0], arg_list[1]);
}

// --------------------------------------------------------------------------------

ObjectRef<Object> Car::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "cons" });
    return this->alma.car(arg_list[0]);
}

// --------------------------------------------------------------------------------

ObjectRef<Object> Cdr::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "cons" });
    return this->alma.cdr(arg_list[0]);
}

// --------------------------------------------------------------------------------

ObjectRef<Object> Eq::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "t", "t" });
    return this->alma.boolean(this->alma.eq(arg_list[0], arg_list[1]));
}

// --------------------------------------------------------------------------------

ObjectRef<Object> Eval::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment)
{
    this->check_types(arg_list, { "t" });

    return this->alma.eval(arg_list[0], environment);
}
