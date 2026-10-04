
#include "procedure.hpp"
#include "alma.hpp"
#include "cons.hpp"
#include "debug.hpp"
#include "environment.hpp"
#include "package.hpp"
#include "symbol.hpp"
#include <stdexcept>

void Procedure::check_types(
    const std::vector<ObjectRef<Object>>& arg_list,
    const std::vector<std::string>& alma_types)
{
    if (arg_list.size() != alma_types.size())
        athrow("Expected " << alma_types.size() << " arguments but received " << arg_list.size());

    for (size_t i = 0; i < arg_list.size(); i++) {
        if (!Alma::alma.alma_typep(arg_list[i], alma_types[i]))
            athrow("The argument number " << i << " must be a " << alma_types[i]);
    }
}

void Procedure::check_types(
    const std::vector<ObjectRef<Object>>& arg_list,
    const std::vector<std::string>& alma_types,
    const std::string& alma_rest_type)
{
    if (arg_list.size() < alma_types.size())
        athrow("Expected at least " << arg_list.size() << " arguments but received " << alma_types.size());
    for (size_t i = 0; i < alma_types.size(); i++) {
        if (!Alma::alma.alma_typep(arg_list[i], alma_types[i]))
            athrow("The argument number " << i << " must be a " << alma_types[i]);
    }
    if (alma_rest_type != "t") {
        for (size_t i = alma_types.size(); i < arg_list.size(); i++) {
            if (!Alma::alma.alma_typep(arg_list[i], alma_rest_type))
                athrow("The argument number " << i << " must be a " << alma_rest_type);
        }
    }
}

Procedure::Procedure()
{
}

std::string Procedure::to_string(ObjectRef<Object> self [[maybe_unused]])
{
    return "<procedure>";
}

ObjectRef<Object> Procedure::apply(
    ObjectRef<Object> self [[maybe_unused]],
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> enviroment)
{
    return this->eval_body(arg_list, enviroment);
}

// -----------------------------------------------------------------------------

Function::Function()
{
}

std::string Function::to_string(ObjectRef<Object> self [[maybe_unused]])
{
    return "<function>";
}

ObjectRef<Object> Function::apply(
    ObjectRef<Object> self [[maybe_unused]],
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> enviroment)
{
    std::vector<ObjectRef<Object>> eval_arg_list;
    for (const ObjectRef<Object>& arg : arg_list)
        eval_arg_list.push_back(Alma::alma.eval(arg));

    return this->eval_body(eval_arg_list, enviroment);
}

// -----------------------------------------------------------------------------

Macro::Macro()
{
}

std::string Macro::to_string(ObjectRef<Object> self [[maybe_unused]])
{
    return "<macro>";
}

ObjectRef<Object> Macro::transform(
    ObjectRef<Object> self [[maybe_unused]],
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> enviroment)
{
    return this->eval_body(arg_list, enviroment);
}

ObjectRef<Object> Macro::apply(
    ObjectRef<Object> self,
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> enviroment)
{
    return Alma::alma.eval(this->transform(self, arg_list, enviroment));
}

// -----------------------------------------------------------------------------

FunctionUser::FunctionUser(
    const std::vector<ObjectRef<Object>>& _param_list,
    const std::optional<ObjectRef<Object>>& _param_rest,
    ObjectRef<Environment> _closure,
    const std::vector<ObjectRef<Object>>& _body)
    : Function()
    , closure(*this, _closure)
{
    for (ObjectRef<Object> param : _param_list)
        this->param_list.emplace_back(*this, param);
    if (_param_rest)
        this->param_rest.emplace(*this, *_param_rest);
    for (ObjectRef<Object> expr : _body)
        this->body.emplace_back(*this, expr);
}

ObjectRef<Object> FunctionUser::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> enviroment [[maybe_unused]])
{
    if (arg_list.size() < this->param_list.size()) {
        if (this->param_rest) {
            mthrow("Expected at least " << this->param_list.size() << " arguments");
        } else {
            mthrow("Expected " << this->param_list.size() << " arguments");
        }
    } else if (!this->param_rest && arg_list.size() > this->param_list.size()) {
        mthrow("Expected " << this->param_list.size() << " arguments");
    }

    std::vector<ObjectRef<Object>> arg_rest;
    if (this->param_rest) {
        for (size_t i = this->param_list.size(); i < arg_list.size(); i++)
            arg_rest.push_back(arg_list[i]);
    }
    {
        Environment::WithLayer layer(*this->closure);

        for (size_t i = 0; i < this->param_list.size(); i++)
            this->closure->insert_or_set_value(
                this->param_list[i], Alma::alma.intern_alma_symbol("value"), arg_list[i]);
        if (this->param_rest)
            this->closure->insert_or_set_value(
                *this->param_rest, Alma::alma.intern_alma_symbol("value"), Alma::alma.make<Cons>(arg_rest));

        for (size_t i = 0; i < this->body.size() - 1; i++)
            Alma::alma.eval(this->body[i], this->closure);
        return Alma::alma.eval(this->body.back(), this->closure);
    }
}

std::string FunctionUser::to_string(ObjectRef<Object> self [[maybe_unused]])
{
    return "<function-user>";
}

// -----------------------------------------------------------------------------

MacroUser::MacroUser(
    const std::vector<ObjectRef<Object>>& _param_list,
    const std::optional<ObjectRef<Object>>& _param_rest,
    ObjectRef<Environment> _closure,
    const std::vector<ObjectRef<Object>>& _body)
    : Macro()
    , closure(*this, _closure)
{
    for (ObjectRef<Object> param : _param_list)
        this->param_list.emplace_back(*this, param);
    if (_param_rest)
        this->param_rest.emplace(*this, *_param_rest);
    for (ObjectRef<Object> expr : _body)
        this->body.emplace_back(*this, expr);
}

ObjectRef<Object> MacroUser::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> enviroment [[maybe_unused]])
{
    if (arg_list.size() < this->param_list.size()) {
        if (this->param_rest) {
            mthrow("Expected at least " << this->param_list.size() << " arguments");
        } else {
            mthrow("Expected " << this->param_list.size() << " arguments");
        }
    } else if (!this->param_rest && arg_list.size() > this->param_list.size()) {
        mthrow("Expected " << this->param_list.size() << " arguments");
    }

    std::vector<ObjectRef<Object>> arg_rest;
    if (this->param_rest) {
        for (size_t i = this->param_list.size(); i < arg_list.size(); i++)
            arg_rest.push_back(arg_list[i]);
    }
    {
        Environment::WithLayer layer(*this->closure);

        for (size_t i = 0; i < this->param_list.size(); i++)
            this->closure->insert_or_set_value(
                this->param_list[i], Alma::alma.intern_alma_symbol("value"), arg_list[i]);
        if (this->param_rest)
            this->closure->insert_or_set_value(
                *this->param_rest, Alma::alma.intern_alma_symbol("value"), Alma::alma.make<Cons>(arg_rest));

        for (size_t i = 0; i < this->body.size() - 1; i++)
            Alma::alma.eval(this->body[i], this->closure);
        return Alma::alma.eval(this->body.back(), this->closure);
    }
}

std::string MacroUser::to_string(ObjectRef<Object> self [[maybe_unused]])
{
    return "<macro-user>";
}

// -----------------------------------------------------------------------------
