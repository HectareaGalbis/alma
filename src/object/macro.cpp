#include "macro.hpp"
#include "alma.hpp"
#include "cons.hpp"
#include "debug.hpp"
#include "symbol.hpp"

template <typename T>
static void intern_macro(const std::string& name)
{
    ObjectRef<Symbol> sym = Alma::alma.intern_alma_symbol(name);
    sym->push_value(Alma::alma.make<T>());
}

void intern_macros()
{
    intern_macro<defun>("defun");
    intern_macro<defmacro>("defmacro");
    intern_macro<emit_function>("function");
    intern_macro<emit_macro>("macro");
    intern_macro<emit_variable>("variable");
    intern_macro<emit_struct>("struct");
    intern_macro<emit_union>("union");
}

// --------------------------------------------------------------------------------

/** (defun name (params) body...) -> (set-symbol-value (quote name) (lambda (params) body...)) */
static ObjectRef<Object> define_operation(const std::string& gen, const std::vector<ObjectRef<Object>>& arg_list)
{
    Alma& alma = Alma::alma;

    aassert(arg_list.size() >= 2, "Expected at least the name and the list of arguments");
    aassert(alma.symbolp(arg_list[0]), "Expected a symbol as first argument but found " << alma.to_string(arg_list[0]));
    aassert(alma.null(arg_list[1]) || alma.consp(arg_list[1]),
        "Expected a list of symbols but found " << alma.to_string(arg_list[1]));

    std::vector<ObjectRef<Object>> lambda_call = { alma.intern_alma_symbol(gen) };
    lambda_call.insert(lambda_call.end(), arg_list.begin() + 1, arg_list.end());

    ObjectRef<Object> quoted_name = alma.make<Cons>(
        std::vector<ObjectRef<Object>> { alma.intern_alma_symbol("quote"), arg_list[0] });

    return alma.make<Cons>(std::vector<ObjectRef<Object>> {
        alma.intern_alma_symbol("set-symbol-value"), quoted_name, alma.make<Cons>(lambda_call) });
}

ObjectRef<Object> defun::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list, ObjectRef<Environment> environment [[maybe_unused]])
{
    return define_operation("lambda", arg_list);
}

ObjectRef<Object> defmacro::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list, ObjectRef<Environment> environment [[maybe_unused]])
{
    return define_operation("gamma", arg_list);
}

// --------------------------------------------------------------------------------

/** (name argument...) -> (emit (quote (name argument...))) */
static ObjectRef<Object> emit_form(const std::string& name, const std::vector<ObjectRef<Object>>& arg_list)
{
    Alma& alma = Alma::alma;
    std::vector<ObjectRef<Object>> form { alma.intern_alma_symbol(name) };
    form.insert(form.end(), arg_list.begin(), arg_list.end());
    ObjectRef<Object> quoted = alma.make<Cons>(
        std::vector<ObjectRef<Object>> { alma.intern_alma_symbol("quote"), alma.make<Cons>(form) });
    return alma.make<Cons>(std::vector<ObjectRef<Object>> { alma.intern_alma_symbol("emit"), quoted });
}

ObjectRef<Object> emit_function::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list, ObjectRef<Environment> environment [[maybe_unused]])
{
    return emit_form("function", arg_list);
}

ObjectRef<Object> emit_macro::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list, ObjectRef<Environment> environment [[maybe_unused]])
{
    return emit_form("macro", arg_list);
}

ObjectRef<Object> emit_variable::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list, ObjectRef<Environment> environment [[maybe_unused]])
{
    return emit_form("variable", arg_list);
}

ObjectRef<Object> emit_struct::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list, ObjectRef<Environment> environment [[maybe_unused]])
{
    return emit_form("struct", arg_list);
}

ObjectRef<Object> emit_union::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list, ObjectRef<Environment> environment [[maybe_unused]])
{
    return emit_form("union", arg_list);
}
