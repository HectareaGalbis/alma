
#include "preprocessor.hpp"
#include "../alma.hpp"
#include "../debug.hpp"
#include "../object/cons.hpp"
#include "../object/environment.hpp"
#include "../object/symbol.hpp"
#include <optional>
#include <set>

PreprocessorMacro::PreprocessorMacro(ObjectRef<FunctionUser> _function, ObjectRef<Environment> _environment)
    : function(*this, _function)
    , environment(*this, _environment)
{
}

ObjectRef<Object> PreprocessorMacro::expand(const std::vector<ObjectRef<Object>>& arg_list)
{
    // Procedure::apply calls the body without evaluating the arguments, unlike Function::apply
    ObjectRef<FunctionUser> body = this->function;
    return body->Procedure::apply(body, arg_list, this->environment);
}

std::string PreprocessorMacro::to_string(ObjectRef<Object> self [[maybe_unused]])
{
    return "<preprocessor-macro>";
}

ObjectRef<Object> PreprocessorMacro::type(ObjectRef<Object> self [[maybe_unused]]) const
{
    return Alma::alma.intern_alma_symbol("preprocessor-macro");
}

// -----------------------------------------------------------------------------

static ObjectRef<Symbol> macro_property()
{
    return Alma::alma.intern_alma_symbol("macro");
}

EmitterMacro::EmitterMacro()
    : Emitter("macro")
{
}

void EmitterMacro::emit_top_level(ObjectRef<Object> self [[maybe_unused]],
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment,
    Declarations& declarations [[maybe_unused]],
    std::stringstream& output [[maybe_unused]])
{
    Alma& alma = Alma::alma;

    aassert(arg_list.size() >= 3, "macro expects a name, a list of parameters and a body");
    aassert(alma.symbolp(arg_list[0]), "The name of a macro must be a symbol but found " << alma.to_string(arg_list[0]));
    ObjectRef<Symbol> macro_name(arg_list[0]);

    std::vector<ObjectRef<Object>> parameters;
    std::optional<ObjectRef<Object>> rest;
    if (!alma.null(arg_list[1])) {
        aassert(alma.consp(arg_list[1]), "Expected a list of parameters but found " << alma.to_string(arg_list[1]));
        auto [list, tail] = arg_list[1].as<Cons>()->to_list();
        aassert(alma.null(tail), "Expected a proper list of parameters but found " << alma.to_string(arg_list[1]));
        for (size_t i = 0; i < list.size(); i++) {
            aassert(alma.symbolp(list[i]), "Expected a symbol as parameter but found " << alma.to_string(list[i]));
            if (list[i].as<Symbol>()->get_name() == "&rest") {
                aassert(i + 2 == list.size() && alma.symbolp(list[i + 1]),
                    "&rest must be followed by the last parameter in " << alma.to_string(arg_list[1]));
                rest = list[i + 1];
                break;
            }
            parameters.push_back(list[i]);
        }
    }

    std::vector<ObjectRef<Object>> body(arg_list.begin() + 2, arg_list.end());
    ObjectRef<FunctionUser> function = alma.make<FunctionUser>(parameters, rest, environment, body);
    ObjectRef<PreprocessorMacro> macro = alma.make<PreprocessorMacro>(function, environment);

    if (macro_name->has_property(macro_property()))
        macro_name->set_property(macro_property(), macro);
    else
        macro_name->push_property(macro_property(), macro);
}

// -----------------------------------------------------------------------------

/** Macros that cannot be expanded because the object is part of their own expansion */
using Blocked = std::set<const Object*>;

static std::optional<ObjectRef<PreprocessorMacro>> find_macro(ObjectRef<Object> obj, const Blocked& blocked)
{
    if (!Alma::alma.symbolp(obj))
        return std::nullopt;
    ObjectRef<Symbol> symbol(obj);
    if (blocked.contains(symbol.get()) || !symbol->has_property(macro_property()))
        return std::nullopt;
    return symbol->get_property(macro_property());
}

static ObjectRef<Object> expand(ObjectRef<Object> obj, const Blocked& blocked)
{
    Alma& alma = Alma::alma;
    if (!alma.consp(obj))
        return obj;

    auto [list, tail] = obj.as<Cons>()->to_list();

    std::optional<ObjectRef<PreprocessorMacro>> macro = alma.null(tail) ? find_macro(list[0], blocked) : std::nullopt;

    // The body of a macro definition is alma code that is not emitted
    if (!macro && list[0] == alma.intern_alma_symbol("macro"))
        return obj;

    if (macro) {
        std::vector<ObjectRef<Object>> arg_list;
        for (size_t i = 1; i < list.size(); i++)
            arg_list.push_back(expand(list[i], blocked));

        ObjectRef<Object> result = (*macro)->expand(arg_list);

        Blocked inner = blocked;
        inner.insert(list[0].get());
        return expand(result, inner);
    }

    std::vector<ObjectRef<Object>> elements;
    for (const ObjectRef<Object>& element : list)
        elements.push_back(expand(element, blocked));
    return alma.make<Cons>(elements, tail);
}

ObjectRef<Object> expand_macros(ObjectRef<Object> obj)
{
    return expand(obj, {});
}

void intern_preprocessor()
{
    ObjectRef<Symbol> macro_symbol = Alma::alma.intern_alma_symbol("macro");
    macro_symbol->push_emitter(Alma::alma.make<EmitterMacro>());
}
