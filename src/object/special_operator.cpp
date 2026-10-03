
#include "special_operator.hpp"
#include "alma.hpp"
#include "cons.hpp"
#include "debug.hpp"
#include "object.hpp"
#include "package.hpp"
#include "symbol.hpp"
#include <memory>

template <typename T>
void intern_special_operator(const std::string& name, Alma& alma)
{
    ObjectRef<Symbol> sym = alma.alma_package->intern_symbol(name, alma);
    sym->set_function(alma.gc.make_object<T>());
}

void intern_special_operators(Alma& alma)
{
    intern_special_operator<progn>("progn", alma);
    intern_special_operator<let>("let", alma);
    intern_special_operator<quote>("quote", alma);
    intern_special_operator<lambda>("lambda", alma);
    intern_special_operator<gamma>("gamma", alma);
    intern_special_operator<branch>("if", alma);
    intern_special_operator<quasiquote>("quasiquote", alma);
}

// --------------------------------------------------------------------------------

ObjectRef<Object> progn::apply(ObjectRef<Cons> arguments, Alma& alma)
{
    if (arguments.empty()) {
        return alma.boolean(false);
    }
    for (size_t i = 0; i < arguments.size() - 1; i++) {
        alma.eval(arguments[i]);
    }
    return alma.eval(arguments.back());
}

// --------------------------------------------------------------------------------

static std::vector<std::pair<ObjectRef<Symbol>, ObjectRef<Object>>> parseBindings(
    ObjectRef<Cons> bindings, Alma& alma)
{
    std::vector<std::pair<ObjectRef<Symbol>, ObjectRef<Object>>> parsedBindings;

    for (ObjectRef<Object> element : bindings->toList()) {
        if (!alma.consp(element))
            throw std::runtime_error("Expected a binding clause (a list).");
        std::vector<ObjectRef<Object>> bindingList = element.as<Cons>()->toList();
        if (bindingList.size() != 2)
            throw std::runtime_error("The binding clause must have 2 elements.");
        if (!alma.symbolp(bindingList[0]))
            throw std::runtime_error("The first element of the binding clause must be a symbol");
        parsedBindings.emplace_back(bindingList[0], bindingList[1]);
    }

    return parsedBindings;
}

static std::vector<std::pair<ObjectRef<Symbol>, ObjectRef<Object>>> evaluateBindings(
    const std::vector<std::pair<ObjectRef<Symbol>, ObjectRef<Object>>>& bindings, Alma& alma)
{
    std::vector<std::pair<ObjectRef<Symbol>, ObjectRef<Object>>> evaluatedBindings;

    for (const auto& [var, value] : bindings) {
        evaluatedBindings.emplace_back(var, alma.eval(value));
    }

    return evaluatedBindings;
}

ObjectRef<Object> let::apply(const std::vector<ObjectRef<Object>>& arguments, Alma& alma)
{
    massert(!arguments.empty(), "let needs at least a list");

    massert(alma.consp(arguments.front()), "Expected a list");

    auto parsedBindings = parseBindings(bindings, alma);
    auto evaluatedBindings = evaluateBindings(lex_env, parsedBindings);

    if (arguments.size() == 1)
        return std::make_shared<Nil>();

    lex_env.pushValues(evaluatedBindings.begin(), evaluatedBindings.end());

    for (size_t i = 1; i < arguments.size() - 1; i++)
        Object::eval(arguments[i], lex_env);

    std::shared_ptr<Object> result = Object::eval(arguments.back(), lex_env);

    lex_env.popValues();

    return result;
}

// --------------------------------------------------------------------------------

std::shared_ptr<Object> quote::apply(
    Environment& lex_env [[maybe_unused]],
    const std::vector<std::shared_ptr<Object>>& arguments)
{
    if (arguments.size() != 1)
        throw std::runtime_error("Expected only one argument.");
    return arguments[0];
}

// --------------------------------------------------------------------------------

static std::vector<std::shared_ptr<Object>> expand_quotation(const std::shared_ptr<Symbol>& sym,
    const std::vector<std::shared_ptr<Object>>& elements)
{
    std::vector<std::shared_ptr<Object>> new_elements;
    for (const std::shared_ptr<Object>& element : elements) {
        new_elements.push_back(std::make_shared<Cons>(std::vector<std::shared_ptr<Object>> { sym, element }));
    }
    return new_elements;
}

static std::vector<std::shared_ptr<Object>> eval_quasiquote(const std::shared_ptr<Object>& obj,
    size_t quasi_level, Environment& lex_env)
{
    std::shared_ptr<Cons> cons = std::dynamic_pointer_cast<Cons>(obj);
    if (!cons)
        return { obj };
    std::vector<std::shared_ptr<Object>> list = cons->toList();
    std::shared_ptr<Symbol> sym = std::dynamic_pointer_cast<Symbol>(list[0]);
    if (sym && sym->name == "quote") {
        return expand_quotation(sym, eval_quasiquote(list[1], quasi_level, lex_env));
    } else if (sym && sym->name == "quasiquote") {
        return expand_quotation(sym, eval_quasiquote(list[1], quasi_level + 1, lex_env));
    } else if (sym && sym->name == "unquote") {
        if (quasi_level == 1)
            return { Object::eval(list[1], lex_env) };
        else {
            return expand_quotation(sym, eval_quasiquote(list[1], quasi_level - 1, lex_env));
        }
    } else if (sym && sym->name == "slice-unquote") {
        if (quasi_level == 1) {
            std::shared_ptr<Object> eval_obj = Object::eval(list[1], lex_env);
            std::shared_ptr<Cons> eval_cons = std::dynamic_pointer_cast<Cons>(eval_obj);
            if (!eval_cons) {
                std::shared_ptr<Nil> nil_obj = std::dynamic_pointer_cast<Nil>(eval_obj);
                if (!nil_obj)
                    throw std::runtime_error("The result of slice-unquote must be a list.");
                return {};
            }
            return eval_cons->toList();
        } else {
            return expand_quotation(sym, eval_quasiquote(list[1], quasi_level - 1, lex_env));
        }
    } else {
        std::vector<std::shared_ptr<Object>> result_list;
        for (std::shared_ptr<Object>& elem : list) {
            std::vector<std::shared_ptr<Object>> result_elem = eval_quasiquote(elem, quasi_level, lex_env);
            result_list.insert(result_list.end(), result_elem.begin(), result_elem.end());
        }
        if (result_list.empty())
            return { std::make_shared<Nil>() };
        else
            return { std::make_shared<Cons>(result_list) };
    }
}

std::shared_ptr<Object> quasiquote::apply(
    Environment& lex_env,
    const std::vector<std::shared_ptr<Object>>& arguments)
{
    if (arguments.size() != 1)
        throw std::runtime_error("Expected only one argument.");

    std::shared_ptr<Cons> list = std::dynamic_pointer_cast<Cons>(arguments[0]);

    if (!list)
        return arguments[0];
    else {
        std::vector<std::shared_ptr<Object>> res = eval_quasiquote(arguments[0], 1, lex_env);
        if (res.size() > 1)
            throw std::runtime_error("Used slice-unquote at the top of quasiquote");
        return res[0];
    }
}

// --------------------------------------------------------------------------------

std::shared_ptr<Object> lambda::apply(
    Environment& lex_env,
    const std::vector<std::shared_ptr<Object>>& arguments)
{
    if (arguments.size() < 1)
        throw std::runtime_error("Expected at least one argument.");

    std::vector<std::shared_ptr<Symbol>> func_arg_symbols;
    std::shared_ptr<Nil> no_args = std::dynamic_pointer_cast<Nil>(arguments[0]);
    if (!no_args) {
        std::shared_ptr<Cons> func_args = std::dynamic_pointer_cast<Cons>(arguments[0]);
        if (!func_args)
            throw std::runtime_error("Expected a list of symbols.");
        for (std::shared_ptr<Object>& func_arg : func_args->toList()) {
            std::shared_ptr<Symbol> func_arg_symbol = std::dynamic_pointer_cast<Symbol>(func_arg);
            if (!func_arg_symbol)
                throw std::runtime_error("Expected a symbol as an argument.");
            func_arg_symbols.push_back(func_arg_symbol);
        }
    }

    std::vector<std::shared_ptr<Object>> body;
    for (size_t i = 1; i < arguments.size(); i++)
        body.push_back(arguments[i]);

    return std::make_shared<FunctionUser>("<lambda>", lex_env, func_arg_symbols, body);
}

// --------------------------------------------------------------------------------

std::shared_ptr<Object> gamma::apply(
    Environment& lex_env,
    const std::vector<std::shared_ptr<Object>>& arguments)
{
    if (arguments.size() < 1)
        throw std::runtime_error("Expected at least one argument.");

    std::shared_ptr<Cons> macro_args = std::dynamic_pointer_cast<Cons>(arguments[0]);
    if (!macro_args)
        throw std::runtime_error("Expected a list of symbols.");

    std::vector<std::shared_ptr<Symbol>> macro_arg_symbols;
    for (std::shared_ptr<Object>& macro_arg : macro_args->toList()) {
        std::shared_ptr<Symbol> macro_arg_symbol = std::dynamic_pointer_cast<Symbol>(macro_arg);
        if (!macro_arg_symbol)
            throw std::runtime_error("Expected a symbol as an argument.");
        macro_arg_symbols.push_back(macro_arg_symbol);
    }

    std::vector<std::shared_ptr<Object>> body;
    for (size_t i = 1; i < arguments.size(); i++)
        body.push_back(arguments[i]);

    return std::make_shared<MacroUser>("<lambda>", lex_env, macro_arg_symbols, body);
}

// --------------------------------------------------------------------------------

std::shared_ptr<Object> branch::apply(
    Environment& lex_env,
    const std::vector<std::shared_ptr<Object>>& arguments)
{
    if (arguments.size() != 2 && arguments.size() != 3)
        throw std::runtime_error("Expected at two or three arguments.");

    if (Object::is_true(Object::eval(arguments[0], lex_env))) {
        return Object::eval(arguments[1], lex_env);
    } else {
        if (arguments.size() == 3)
            return Object::eval(arguments[2], lex_env);
        else
            return std::make_shared<Nil>();
    }
}
