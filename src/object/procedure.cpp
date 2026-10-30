
#include "procedure.hpp"
#include "alma.hpp"
#include "cons.hpp"
#include "debug.hpp"
#include "environment.hpp"
#include "hash_table.hpp"
#include "package.hpp"
#include "symbol.hpp"
#include <algorithm>
#include <stdexcept>

static bool is_type(ObjectRef<Object> obj, const std::string& type)
{
    return Alma::alma.typep(obj, Alma::alma.intern_alma_symbol(type));
}

void Procedure::check_types(
    const std::vector<ObjectRef<Object>>& arg_list,
    const std::vector<std::string>& alma_types)
{
    if (arg_list.size() != alma_types.size())
        athrow("Expected " << alma_types.size() << " arguments but received " << arg_list.size());

    for (size_t i = 0; i < arg_list.size(); i++) {
        if (!is_type(arg_list[i], alma_types[i]))
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
        if (!is_type(arg_list[i], alma_types[i]))
            athrow("The argument number " << i << " must be a " << alma_types[i]);
    }
    if (alma_rest_type != "t") {
        for (size_t i = alma_types.size(); i < arg_list.size(); i++) {
            if (!is_type(arg_list[i], alma_rest_type))
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

ObjectRef<Object> Procedure::type(ObjectRef<Object> self [[maybe_unused]]) const
{
    return Alma::alma.intern_alma_symbol("procedure");
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

ObjectRef<Object> Function::type(ObjectRef<Object> self [[maybe_unused]]) const
{
    return Alma::alma.intern_alma_symbol("function");
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

ObjectRef<Object> Macro::type(ObjectRef<Object> self [[maybe_unused]]) const
{
    return Alma::alma.intern_alma_symbol("macro");
}

ObjectRef<Object> Macro::transform(
    ObjectRef<Object> self [[maybe_unused]],
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> enviroment)
{
    return Alma::alma.expand(this->eval_body(arg_list, enviroment));
}

ObjectRef<Object> Macro::apply(
    ObjectRef<Object> self,
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> enviroment)
{
    return Alma::alma.eval(this->transform(self, arg_list, enviroment));
}

// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// LambdaList

static bool is_named(ObjectRef<Object> obj, const char* name)
{
    return Alma::alma.symbolp(obj) && obj.as<Symbol>()->get_name() == name;
}

LambdaList::LambdaList(Object& _owner, ObjectRef<Object> spec)
    : owner(_owner)
    , rest(nullptr)
    , rest_keys(nullptr)
{
    if (Alma::alma.null(spec))
        return;
    aassert(Alma::alma.consp(spec), "Expected a list of parameters but found " << Alma::alma.to_string(spec));
    auto [elements, tail] = spec.as<Cons>()->to_list();
    aassert(Alma::alma.null(tail), "Expected a proper list of parameters but found " << Alma::alma.to_string(spec));

    enum class Stage { Required,
        Optional,
        Keyword };
    Stage stage = Stage::Required;
    std::vector<Symbol*> names;
    std::vector<Symbol*> keywords;
    for (size_t i = 0; i < elements.size(); i++) {
        ObjectRef<Object> element = elements[i];
        if (is_named(element, "&rest") || is_named(element, "&rest-keys")) {
            bool is_rest = is_named(element, "&rest");
            const char* name = is_rest ? "&rest" : "&rest-keys";
            aassert(!(is_rest ? this->rest : this->rest_keys), name << " is repeated");
            aassert(i + 1 < elements.size(), name << " must be followed by a symbol");
            Symbol* var = this->keep_variable(elements[++i], names);
            (is_rest ? this->rest : this->rest_keys) = var;
        } else if (Alma::alma.keywordp(element)) {
            Symbol* keyword = static_cast<Symbol*>(element.get());
            aassert(std::find(keywords.begin(), keywords.end(), keyword) == keywords.end(),
                "The keyword " << Alma::alma.to_string(element) << " is repeated");
            aassert(i + 1 < elements.size(),
                "The keyword " << Alma::alma.to_string(element) << " must be followed by a parameter");
            keywords.push_back(keyword);
            this->objects.emplace_back(this->owner, element);
            Parameter parameter = this->parse_parameter(elements[++i], true, names);
            parameter.keyword = keyword;
            this->keys.push_back(parameter);
            stage = Stage::Keyword;
        } else if (Alma::alma.consp(element)) {
            aassert(stage != Stage::Keyword,
                "The optional parameter " << Alma::alma.to_string(element) << " must be before the keyword parameters");
            this->optional.push_back(this->parse_parameter(element, false, names));
            stage = Stage::Optional;
        } else {
            aassert(stage == Stage::Required,
                "The required parameter " << Alma::alma.to_string(element)
                                          << " must be before the optional and keyword parameters");
            this->required.push_back({ this->keep_variable(element, names), nullptr, nullptr, nullptr });
        }
    }
}

Symbol* LambdaList::keep_variable(ObjectRef<Object> obj, std::vector<Symbol*>& names)
{
    aassert(Alma::alma.symbolp(obj) && !Alma::alma.keywordp(obj),
        "Expected a symbol as parameter but found " << Alma::alma.to_string(obj));
    Symbol* var = static_cast<Symbol*>(obj.get());
    aassert(std::find(names.begin(), names.end(), var) == names.end(),
        "The parameter " << Alma::alma.to_string(obj) << " is repeated");
    names.push_back(var);
    this->objects.emplace_back(this->owner, obj);
    return var;
}

LambdaList::Parameter LambdaList::parse_parameter(ObjectRef<Object> obj, bool allow_symbol, std::vector<Symbol*>& names)
{
    if (allow_symbol && !Alma::alma.consp(obj))
        return { this->keep_variable(obj, names), nullptr, nullptr, nullptr };
    aassert(Alma::alma.consp(obj), "Expected a parameter but found " << Alma::alma.to_string(obj));
    auto [spec, tail] = obj.as<Cons>()->to_list();
    aassert(Alma::alma.null(tail) && spec.size() <= 3,
        "Expected (name [default [received]]) as parameter but found " << Alma::alma.to_string(obj));
    Parameter parameter { this->keep_variable(spec[0], names), nullptr, nullptr, nullptr };
    if (spec.size() >= 2) {
        parameter.default_form = spec[1].get();
        this->objects.emplace_back(this->owner, spec[1]);
    }
    if (spec.size() == 3)
        parameter.supplied = this->keep_variable(spec[2], names);
    return parameter;
}

void LambdaList::bind(const std::vector<ObjectRef<Object>>& arg_list, ObjectRef<Environment> environment) const
{
    Alma& alma = Alma::alma;
    ObjectRef<Symbol> value = alma.intern_alma_symbol("value");
    auto set = [&](Symbol* var, ObjectRef<Object> obj) {
        environment->insert_or_set_value(ObjectRef<Symbol>(var), value, obj);
    };
    auto set_default = [&](const Parameter& parameter) {
        set(parameter.var,
            parameter.default_form ? alma.eval(ObjectRef<Object>(parameter.default_form), environment)
                                   : alma.intern_alma_symbol("nil"));
        if (parameter.supplied)
            set(parameter.supplied, alma.boolean(false));
    };
    auto set_received = [&](const Parameter& parameter, ObjectRef<Object> obj) {
        set(parameter.var, obj);
        if (parameter.supplied)
            set(parameter.supplied, alma.boolean(true));
    };

    size_t positional = this->required.size() + this->optional.size();
    aassert(arg_list.size() >= this->required.size(),
        "Expected at least " << this->required.size() << " arguments but received " << arg_list.size());

    size_t i = 0;
    for (const Parameter& parameter : this->required)
        set(parameter.var, arg_list[i++]);
    for (const Parameter& parameter : this->optional) {
        if (i < arg_list.size())
            set_received(parameter, arg_list[i++]);
        else
            set_default(parameter);
    }

    // The rest of arguments: keyword arguments mixed with the ones of &rest
    bool uses_keys = !this->keys.empty() || this->rest_keys;
    std::vector<ObjectRef<Object>> rest_values;
    std::vector<std::optional<ObjectRef<Object>>> key_values(this->keys.size());
    std::vector<Object*> received_keywords;
    std::optional<ObjectRef<HashTable>> other_keys;
    if (this->rest_keys)
        other_keys = alma.make<HashTable>(HashTable::Test::Eql, std::nullopt, std::nullopt, 0);
    for (; i < arg_list.size(); i++) {
        ObjectRef<Object> arg = arg_list[i];
        if (uses_keys && alma.keywordp(arg)) {
            aassert(i + 1 < arg_list.size(), "The keyword " << alma.to_string(arg) << " needs a value");
            aassert(std::find(received_keywords.begin(), received_keywords.end(), arg.get()) == received_keywords.end(),
                "The keyword " << alma.to_string(arg) << " is repeated");
            received_keywords.push_back(arg.get());
            ObjectRef<Object> key_value = arg_list[++i];
            auto it = std::find_if(this->keys.begin(), this->keys.end(),
                [&](const Parameter& parameter) { return parameter.keyword == arg.get(); });
            if (it != this->keys.end())
                key_values[it - this->keys.begin()] = key_value;
            else {
                aassert(other_keys, "Unknown keyword " << alma.to_string(arg));
                (*other_keys)->put(arg, key_value);
            }
        } else {
            aassert(this->rest, "Expected at most " << positional << " arguments but received " << arg_list.size());
            rest_values.push_back(arg);
        }
    }

    if (this->rest)
        set(this->rest, rest_values.empty() ? alma.intern_alma_symbol("nil") : ObjectRef<Object>(alma.make<Cons>(rest_values)));
    if (this->rest_keys)
        set(this->rest_keys, *other_keys);
    for (size_t k = 0; k < this->keys.size(); k++) {
        if (key_values[k])
            set_received(this->keys[k], *key_values[k]);
        else
            set_default(this->keys[k]);
    }
}

// -----------------------------------------------------------------------------

static ObjectRef<Object> run_user_procedure(const LambdaList& lambda_list, ObjectRef<Environment> closure,
    const std::vector<ObjectTrackedKeyRef<Object>>& body, const std::vector<ObjectRef<Object>>& arg_list)
{
    Environment::WithLayer layer(*closure);
    lambda_list.bind(arg_list, closure);
    ObjectRef<Object> result = Alma::alma.intern_alma_symbol("nil");
    for (const ObjectTrackedKeyRef<Object>& expr : body)
        result = Alma::alma.eval(expr, closure);
    return result;
}

// -----------------------------------------------------------------------------

FunctionUser::FunctionUser(
    ObjectRef<Object> _lambda_list,
    ObjectRef<Environment> _closure,
    const std::vector<ObjectRef<Object>>& _body)
    : Function()
    , lambda_list(*this, _lambda_list)
    , closure(*this, _closure)
{
    for (ObjectRef<Object> expr : _body)
        this->body.emplace_back(*this, expr);
}

ObjectRef<Object> FunctionUser::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> enviroment [[maybe_unused]])
{
    return run_user_procedure(this->lambda_list, this->closure, this->body, arg_list);
}

std::string FunctionUser::to_string(ObjectRef<Object> self [[maybe_unused]])
{
    return "<function-user>";
}

ObjectRef<Object> FunctionUser::type(ObjectRef<Object> self [[maybe_unused]]) const
{
    return Alma::alma.intern_alma_symbol("function-user");
}

// -----------------------------------------------------------------------------

MacroUser::MacroUser(
    ObjectRef<Object> _lambda_list,
    ObjectRef<Environment> _closure,
    const std::vector<ObjectRef<Object>>& _body)
    : Macro()
    , lambda_list(*this, _lambda_list)
    , closure(*this, _closure)
{
    for (ObjectRef<Object> expr : _body)
        this->body.emplace_back(*this, expr);
}

ObjectRef<Object> MacroUser::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> enviroment [[maybe_unused]])
{
    return run_user_procedure(this->lambda_list, this->closure, this->body, arg_list);
}

std::string MacroUser::to_string(ObjectRef<Object> self [[maybe_unused]])
{
    return "<macro-user>";
}

ObjectRef<Object> MacroUser::type(ObjectRef<Object> self [[maybe_unused]]) const
{
    return Alma::alma.intern_alma_symbol("macro-user");
}

// -----------------------------------------------------------------------------

SpecialOperatorUser::SpecialOperatorUser(
    ObjectRef<Object> _lambda_list,
    ObjectRef<Environment> _closure,
    const std::vector<ObjectRef<Object>>& _body)
    : Procedure()
    , lambda_list(*this, _lambda_list)
    , closure(*this, _closure)
{
    for (ObjectRef<Object> expr : _body)
        this->body.emplace_back(*this, expr);
}

ObjectRef<Object> SpecialOperatorUser::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> enviroment [[maybe_unused]])
{
    return run_user_procedure(this->lambda_list, this->closure, this->body, arg_list);
}

std::string SpecialOperatorUser::to_string(ObjectRef<Object> self [[maybe_unused]])
{
    return "<special-operator-user>";
}

ObjectRef<Object> SpecialOperatorUser::type(ObjectRef<Object> self [[maybe_unused]]) const
{
    return Alma::alma.intern_alma_symbol("special-operator-user");
}

// -----------------------------------------------------------------------------
