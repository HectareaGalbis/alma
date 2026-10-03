
#include "macro.hpp"
#include "package.hpp"

#define intern_macro(name, sym_name)                                                       \
    std::shared_ptr<Symbol>& name##_macro = Package::almaPackage->intern_symbol(sym_name); \
    name##_macro->function = std::make_shared<name>(sym_name);

void intern_macros()
{
    intern_macro(defun, "defun");
    intern_macro(defmacro, "defmacro");
}

// --------------------------------------------------------------------------------

static std::shared_ptr<Object> define_operation(const std::string& gen,
    const std::vector<std::shared_ptr<Object>>& args)
{
    if (args.size() < 2)
        throw std::runtime_error("Expected at least the name and list of arguments");

    std::shared_ptr<Symbol> funcname = std::dynamic_pointer_cast<Symbol>(args[0]);
    if (!funcname)
        throw std::runtime_error("Expected a symbol as first argument");

    std::shared_ptr<Object> funcargs = std::dynamic_pointer_cast<Nil>(args[1]);
    if (!funcargs) {
        funcargs = std::dynamic_pointer_cast<Cons>(args[1]);
        if (!funcargs)
            throw std::runtime_error("Expected a list of symbols.");
    }

    // set-symbol-function
    std::shared_ptr<Object> set_symbol_function = *Package::almaPackage->find_symbol("set-symbol-function");

    // (quote funcname)
    std::shared_ptr<Object> quote = *Package::almaPackage->find_symbol("quote");
    std::shared_ptr<Object> quote_funcname = std::make_shared<Cons>(std::vector<std::shared_ptr<Object>> { quote, funcname });

    // (lambda (...) body ...)
    std::shared_ptr<Object> lambda = *Package::almaPackage->find_symbol(gen);
    std::vector<std::shared_ptr<Object>> lambdacall = { lambda, funcargs };
    for (size_t i = 2; i < args.size(); i++) {
        lambdacall.push_back(args[i]);
    }
    std::shared_ptr<Object> lambda_object = std::make_shared<Cons>(lambdacall);

    // (set-symbol-function (quote funcname) (lambda (...) body ...))
    std::vector<std::shared_ptr<Object>> result_list = { set_symbol_function, quote_funcname, lambda_object };
    std::shared_ptr<Object> result = std::make_shared<Cons>(result_list);

    return result;
}

std::shared_ptr<Object> defun::eval_body(
    const std::vector<std::shared_ptr<Object>>& args, Environment& lex_env [[maybe_unused]])
{
    return define_operation("lambda", args);
}

std::shared_ptr<Object> defmacro::eval_body(
    const std::vector<std::shared_ptr<Object>>& args, Environment& lex_env [[maybe_unused]])
{
    return define_operation("gamma", args);
}
