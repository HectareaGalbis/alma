
#include "alma.hpp"
#include "special_operator.hpp"
#include "cons.hpp"
#include "debug.hpp"
#include "environment.hpp"
#include "nil.hpp"
#include "object.hpp"
#include "package.hpp"
#include "symbol.hpp"
#include <optional>
#include <utility>

template <typename T>
void intern_special_operator(const std::string& name)
{
    ObjectRef<Symbol> sym = Alma::alma.intern_alma_symbol(name);
    sym->set_value(Alma::alma.make<T>());
}

void intern_special_operators()
{
    intern_special_operator<progn>("progn");
    intern_special_operator<let>("let");
    intern_special_operator<quote>("quote");
    intern_special_operator<lambda>("lambda");
    intern_special_operator<gamma>("gamma");
    intern_special_operator<branch>("if");
    intern_special_operator<quasiquote>("quasiquote");
}

// --------------------------------------------------------------------------------

ObjectRef<Object> progn::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list, ObjectRef<Environment> environment)
{
    if (arg_list.empty()) {
        return Alma::alma.boolean(false);
    }
    for (size_t i = 0; i < arg_list.size() - 1; i++) {
        Alma::alma.eval(arg_list[i], environment);
    }
    return Alma::alma.eval(arg_list.back(), environment);
}

// --------------------------------------------------------------------------------

static std::vector<std::pair<ObjectRef<Symbol>, ObjectRef<Object>>> parseBindings(
    ObjectRef<Object> bindings)
{
    std::vector<std::pair<ObjectRef<Symbol>, ObjectRef<Object>>> parsedBindings;

    for (ObjectRef<Object> element : bindings.as<Cons>()->to_list().first) {
        massert(Alma::alma.consp(element), "Expected a binding clause (a list).");
        std::vector<ObjectRef<Object>> bindingList = element.as<Cons>()->to_list().first;
        massert(bindingList.size() == 2, "The binding clause must have 2 elements.");
        massert(Alma::alma.symbolp(bindingList[0]),
            "The first element of the binding clause must be a symbol");
        parsedBindings.emplace_back(bindingList[0], bindingList[1]);
    }

    return parsedBindings;
}

static std::vector<std::pair<ObjectRef<Symbol>, ObjectRef<Object>>> evaluateBindings(
    const std::vector<std::pair<ObjectRef<Symbol>, ObjectRef<Object>>>& bindings,
    ObjectRef<Environment> environment)
{
    std::vector<std::pair<ObjectRef<Symbol>, ObjectRef<Object>>> evaluatedBindings;

    for (const auto& [var, value] : bindings) {
        evaluatedBindings.emplace_back(var, Alma::alma.eval(value, environment));
    }

    return evaluatedBindings;
}

ObjectRef<Object> let::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list, ObjectRef<Environment> environment)
{
    massert(!arg_list.empty(), "let needs at least a list");

    massert(Alma::alma.consp(arg_list.front()), "Expected a list");

    if (arg_list.size() == 1)
        return Alma::alma.make<Nil>();

    std::vector<std::pair<ObjectRef<Symbol>, ObjectRef<Object>>> evaluatedBindings
        = evaluateBindings(parseBindings(arg_list.front()), environment);

    ObjectRef<Symbol> value_property = Alma::alma.intern_alma_symbol("value");

    {
        Environment::WithLayer layer(*environment);

        for (const auto& [var, value] : evaluatedBindings)
            environment->insert_or_set_value(var, value_property, value);

        for (size_t i = 1; i < arg_list.size() - 1; i++)
            Alma::alma.eval(arg_list[i], environment);

        return Alma::alma.eval(arg_list.back(), environment);
    }
}

// --------------------------------------------------------------------------------

ObjectRef<Object> quote::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    massert(arg_list.size() == 1, "Expected only one argument.");
    return arg_list[0];
}

// --------------------------------------------------------------------------------

static std::vector<ObjectRef<Object>> expand_quotation(ObjectRef<Symbol> sym,
    const std::vector<ObjectRef<Object>>& elements)
{
    std::vector<ObjectRef<Object>> new_elements;
    for (const ObjectRef<Object>& element : elements) {
        new_elements.push_back(
            Alma::alma.make<Cons>(std::vector<ObjectRef<Object>> { sym, element }));
    }
    return new_elements;
}

static std::vector<ObjectRef<Object>> eval_quasiquote(ObjectRef<Object> obj,
    size_t quasi_level, ObjectRef<Environment> environment)
{
    if (!Alma::alma.consp(obj))
        return { obj };

    std::vector<ObjectRef<Object>> list = obj.as<Cons>()->to_list().first;
    massert(!list.empty(), "Expected a non empty list");

    std::optional<ObjectRef<Symbol>> sym;
    std::string name;
    if (Alma::alma.symbolp(list[0])) {
        sym = list[0];
        name = (*sym)->get_name();
    }

    if (sym && name == "quote") {
        return expand_quotation(*sym, eval_quasiquote(list[1], quasi_level, environment));
    } else if (sym && name == "quasiquote") {
        return expand_quotation(*sym, eval_quasiquote(list[1], quasi_level + 1, environment));
    } else if (sym && name == "unquote") {
        if (quasi_level == 1)
            return { Alma::alma.eval(list[1], environment) };
        else {
            return expand_quotation(*sym, eval_quasiquote(list[1], quasi_level - 1, environment));
        }
    } else if (sym && name == "slice-unquote") {
        if (quasi_level == 1) {
            ObjectRef<Object> eval_obj = Alma::alma.eval(list[1], environment);
            if (Alma::alma.null(eval_obj))
                return {};
            massert(Alma::alma.consp(eval_obj), "The result of slice-unquote must be a list.");
            return eval_obj.as<Cons>()->to_list().first;
        } else {
            return expand_quotation(*sym, eval_quasiquote(list[1], quasi_level - 1, environment));
        }
    } else {
        std::vector<ObjectRef<Object>> result_list;
        for (const ObjectRef<Object>& elem : list) {
            std::vector<ObjectRef<Object>> result_elem
                = eval_quasiquote(elem, quasi_level, environment);
            result_list.insert(result_list.end(), result_elem.begin(), result_elem.end());
        }
        if (result_list.empty())
            return { Alma::alma.make<Nil>() };
        else
            return { Alma::alma.make<Cons>(result_list) };
    }
}

ObjectRef<Object> quasiquote::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list, ObjectRef<Environment> environment)
{
    massert(arg_list.size() == 1, "Expected only one argument.");

    if (!Alma::alma.consp(arg_list[0]))
        return arg_list[0];

    std::vector<ObjectRef<Object>> res = eval_quasiquote(arg_list[0], 1, environment);
    massert(res.size() == 1, "Used slice-unquote at the top of quasiquote");
    return res.front();
}

// --------------------------------------------------------------------------------

ObjectRef<Object> lambda::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list, ObjectRef<Environment> environment)
{
    massert(arg_list.size() >= 1, "Expected at least one argument.");

    std::vector<ObjectRef<Object>> param_list;
    if (!Alma::alma.null(arg_list[0])) {
        massert(Alma::alma.consp(arg_list[0]), "Expected a list of symbols.");
        param_list = arg_list[0].as<Cons>()->to_list().first;
        for (const ObjectRef<Object>& param : param_list)
            massert(Alma::alma.symbolp(param), "Expected a symbol as an argument.");
    }

    std::vector<ObjectRef<Object>> body;
    for (size_t i = 1; i < arg_list.size(); i++)
        body.push_back(arg_list[i]);

    return Alma::alma.make<FunctionUser>(param_list, std::nullopt, environment, body);
}

// --------------------------------------------------------------------------------

ObjectRef<Object> gamma::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list, ObjectRef<Environment> environment)
{
    massert(arg_list.size() >= 1, "Expected at least one argument.");

    massert(Alma::alma.consp(arg_list[0]), "Expected a list of symbols.");

    std::vector<ObjectRef<Object>> param_list = arg_list[0].as<Cons>()->to_list().first;
    for (const ObjectRef<Object>& param : param_list)
        massert(Alma::alma.symbolp(param), "Expected a symbol as an argument.");

    std::vector<ObjectRef<Object>> body;
    for (size_t i = 1; i < arg_list.size(); i++)
        body.push_back(arg_list[i]);

    return Alma::alma.make<MacroUser>(param_list, std::nullopt, environment, body);
}

// --------------------------------------------------------------------------------

ObjectRef<Object> branch::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list, ObjectRef<Environment> environment)
{
    massert(arg_list.size() == 2 || arg_list.size() == 3,
        "Expected at two or three arguments.");

    if (Alma::alma.truep(Alma::alma.eval(arg_list[0], environment))) {
        return Alma::alma.eval(arg_list[1], environment);
    } else {
        if (arg_list.size() == 3)
            return Alma::alma.eval(arg_list[2], environment);
        else
            return Alma::alma.make<Nil>();
    }
}
