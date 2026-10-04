

#include "alma.hpp"
#include "debug.hpp"
#include "object/cons.hpp"
#include "object/environment.hpp"
#include "object/function.hpp"
#include "object/object.hpp"
#include "object/package.hpp"
#include "reader.hpp"
#include <fstream>

#define intern_function(name, sym_name)                                 \
    ObjectRef<Symbol> name##_func = this->intern_alma_symbol(sym_name); \
    name##_func->push_value(this->make<name>());

void Alma::intern_functions()
{
    intern_function(Sum, "+");
    intern_function(Print, "print");
    intern_function(Type, "type");
    intern_function(Set_symbol_value, "set-symbol-value");
    intern_function(Car, "car");
    intern_function(Cdr, "cdr");
    intern_function(Setq, "setq");
    intern_function(Eq, "eq");
    intern_function(Eval, "eval");
}

static void intern_selfevaluating_symbol(Alma& alma, const std::string& name)
{
    ObjectRef<Symbol> sym = alma.intern_alma_symbol(name);
    sym->push_value(sym);
}

void Alma::intern_symbols()
{
    intern_selfevaluating_symbol(*this, "t");
    intern_selfevaluating_symbol(*this, "nil");
}

Alma::Alma()
    : gc()
    , environment(gc.make_object<Environment>())
    , alma_package(gc.make_object<Package>())
    , current_package(alma_package)
    , nil(gc.make_object<Nil>())
    , type_system(gc.make_object<TypeSystem>())
{
    this->intern_symbols();
    this->intern_functions();
}

ObjectRef<Object> Alma::expand(ObjectRef<Object> obj, ObjectRef<Environment> _environment)
{
    try {
        return obj->expand(obj, _environment);
    } catch (const std::runtime_error& e) {
        athrow("Error expanding " << this->to_string(obj) << "\n"
                                  << e.what());
    }
}

ObjectRef<Object> Alma::expand(ObjectRef<Object> obj)
{
    return this->expand(obj, this->environment);
}

ObjectRef<Object> Alma::eval(ObjectRef<Object> obj, ObjectRef<Environment> lex_environment)
{
    try {
        return obj->eval(obj, lex_environment);
    } catch (const std::runtime_error& e) {
        athrow("Error evaluating " << this->to_string(obj) << "\n"
                                   << e.what());
    }
}

ObjectRef<Object> Alma::eval(ObjectRef<Object> obj)
{
    return this->eval(obj, this->environment);
}

void Alma::load(const std::filesystem::path& path)
{
    std::ifstream input(path);
    Reader reader(*this, path.native(), input);
    for (std::optional<ObjectRef<Object>> obj = reader.read(); obj; obj = reader.read())
        this->eval(*obj);
}

ObjectRef<Object> Alma::transform(
    ObjectRef<Object> obj,
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> _environment)
{
    return obj->transform(obj, arg_list, _environment);
}

ObjectRef<Object> Alma::transform(
    ObjectRef<Object> obj,
    ObjectRef<Cons> args,
    ObjectRef<Environment> _environment)
{
    auto [arg_list, arg_rest] = args->to_list();
    aassert(!arg_rest, "Cannot transform a non proper list of arguments " << args);
    return this->transform(obj, arg_list, _environment);
}

ObjectRef<Object> Alma::apply(
    ObjectRef<Object> obj,
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> _environment)
{
    return obj->apply(obj, arg_list, _environment);
}

ObjectRef<Object> Alma::apply(
    ObjectRef<Object> obj,
    ObjectRef<Cons> args,
    ObjectRef<Environment> _environment)
{
    auto [arg_list, arg_rest] = args->to_list();
    aassert(!arg_rest, "Cannot apply a non proper list of arguments " << args);
    return this->apply(obj, arg_list, _environment);
}

ObjectRef<Package> Alma::get_current_package()
{
    return this->current_package;
}

std::string Alma::to_string(ObjectRef<Object> obj)
{
    return obj->to_string(obj);
}

ObjectRef<Object> Alma::type(ObjectRef<Object> obj)
{
    return obj->type(obj);
}

bool Alma::typep(ObjectRef<Object> obj, ObjectRef<Object> type)
{
    return this->type_system->subtypep(this->type(obj), type);
}

bool Alma::symbolp(ObjectRef<Object> obj)
{
    return this->type(obj) == this->intern_alma_symbol("symbol");
}

ObjectRef<Object> Alma::symbol_value(ObjectRef<Symbol> symbol)
{
    return symbol->get_value();
}

ObjectRef<Object> Alma::set_symbol_value(ObjectRef<Symbol> symbol, ObjectRef<Object> value)
{
    symbol->set_value(value);
    return value;
}

bool Alma::consp(ObjectRef<Object> obj)
{
    return this->type(obj) == this->intern_alma_symbol("cons");
}

bool Alma::truep(ObjectRef<Object> obj)
{
    return obj != this->intern_alma_symbol("nil");
}

bool Alma::null(ObjectRef<Object> obj)
{
    return obj == this->intern_alma_symbol("nil");
}

ObjectRef<Object> Alma::boolean(bool v)
{
    return this->intern_alma_symbol(v ? "t" : "nil");
}

bool Alma::eq(ObjectRef<Object> obj1, ObjectRef<Object> obj2)
{
    return obj1 == obj2;
}

ObjectRef<Object> Alma::setq(ObjectRef<Symbol> symbol, ObjectRef<Object> value,
    ObjectRef<Environment> _environment)
{
    ObjectRef<Symbol> sym_value = this->intern_alma_symbol("value");
    if (_environment->has_symbol_property(symbol, sym_value)) {
        _environment->set_value(symbol, sym_value, value);
    } else {
        symbol->set_value(value);
    }
    return value;
}

ObjectRef<Object> Alma::setq(ObjectRef<Symbol> symbol, ObjectRef<Object> value)
{
    return this->setq(symbol, value, this->environment);
}

ObjectRef<Object> Alma::find_symbol(const std::string& name, ObjectRef<Package> package)
{
    return package->find_symbol(name);
}

ObjectRef<Object> Alma::find_symbol(const std::string& name)
{
    return this->current_package->find_symbol(name);
}

ObjectRef<Object> Alma::find_alma_symbol(const std::string& name)
{
    return this->alma_package->find_symbol(name);
}

ObjectRef<Object> Alma::intern_symbol(const std::string& name, ObjectRef<Package> package)
{
    return package->intern_symbol(name);
}

ObjectRef<Object> Alma::intern_symbol(const std::string& name)
{
    return this->current_package->intern_symbol(name);
}

ObjectRef<Object> Alma::intern_alma_symbol(const std::string& name)
{
    return this->alma_package->intern_symbol(name);
}

ObjectRef<Object> Alma::find_character_macro(char c, ObjectRef<Package> package)
{
    std::optional<ObjectRef<Procedure>> proc = package->find_character_macro(c);
    if (proc.has_value())
        return *proc;
    else
        return this->boolean(false);
}

ObjectRef<Object> Alma::find_character_macro(char c)
{
    return this->find_character_macro(c, this->current_package);
}

ObjectRef<Object> Alma::find_alma_character_macro(char c)
{
    return this->find_character_macro(c, this->alma_package);
}

ObjectRef<Object> Alma::intern_character_macro(char c, ObjectRef<Procedure> proc, ObjectRef<Package> package)
{
    return package->intern_character_macro(c, proc);
}

ObjectRef<Object> Alma::intern_character_macro(char c, ObjectRef<Procedure> proc)
{
    return this->intern_character_macro(c, proc, this->current_package);
}

ObjectRef<Object> Alma::intern_alma_character_macro(char c, ObjectRef<Procedure> proc)
{
    return this->intern_character_macro(c, proc, this->alma_package);
}

ObjectRef<Object> Alma::car(ObjectRef<Cons> c)
{
    return c->get_car();
}

ObjectRef<Object> Alma::cdr(ObjectRef<Cons> c)
{
    return c->get_cdr();
}
