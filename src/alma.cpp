

#include "alma.hpp"
#include "debug.hpp"
#include "interpiler/operators.hpp"
#include "interpiler/preprocessor.hpp"
#include "interpiler/standard.hpp"
#include "interfaces/applicable.hpp"
#include "interfaces/emittable.hpp"
#include "interfaces/expandable.hpp"
#include "interfaces/transformable.hpp"
#include "object/cons.hpp"
#include "object/environment.hpp"
#include "object/function.hpp"
#include "object/macro.hpp"
#include "object/object.hpp"
#include "object/package.hpp"
#include "object/special_operator.hpp"
#include "reader.hpp"
#include <fstream>

Alma Alma::alma;

#define intern_function(name, sym_name)                                 \
    ObjectRef<Symbol> name##_func = this->intern_alma_symbol(sym_name); \
    name##_func->push_value(this->make<name>());

void Alma::intern_functions()
{
    // Alma
    intern_function(Sum, "+");
    intern_function(Print, "print");
    // intern_function(Type, "type");
    intern_function(Set_symbol_value, "set-symbol-value");
    intern_function(Car, "car");
    intern_function(Cdr, "cdr");
    intern_function(Setq, "setq");
    intern_function(Eq, "eq");
    intern_function(Eval, "eval");
    intern_function(List, "list");

    // Alma: vectors
    intern_function(Vector_create, "vector");
    intern_function(Make_vector, "make-vector");
    intern_function(Vectorp, "vectorp");
    intern_function(Vector_length, "vector-length");
    intern_function(Vector_empty_p, "vector-empty-p");
    intern_function(Vector_set, "vector-set");
    intern_function(Vector_front, "vector-front");
    intern_function(Vector_back, "vector-back");
    intern_function(Vector_push_front, "vector-push-front");
    intern_function(Vector_push_back, "vector-push-back");
    intern_function(Vector_pop_front, "vector-pop-front");
    intern_function(Vector_pop_back, "vector-pop-back");
    intern_function(Vector_insert, "vector-insert");
    intern_function(Vector_erase, "vector-erase");
    intern_function(Vector_clear, "vector-clear");
    intern_function(Vector_to_list, "vector-to-list");
    intern_function(List_to_vector, "list-to-vector");

    // C: emission
    intern_function(Emit, "emit");

    // C: introspection of kinds of declarations
    intern_function(C_declared_p, "c-declared-p");
    intern_function(C_defined_p, "c-defined-p");
    intern_function(C_function_p, "c-function-p");
    intern_function(C_variable_p, "c-variable-p");
    intern_function(C_constant_p, "c-constant-p");
    intern_function(C_standard_p, "c-standard-p");
    intern_function(C_struct_p, "c-struct-p");
    intern_function(C_union_p, "c-union-p");
    intern_function(C_macro_p, "c-macro-p");

    // C: introspection of functions and variables
    intern_function(C_type, "c-type");
    intern_function(C_header, "c-header");
    intern_function(C_function_result, "c-function-result");
    intern_function(C_function_parameters, "c-function-parameters");
    intern_function(C_function_variadic_p, "c-function-variadic-p");

    // C: introspection of structs and unions
    intern_function(C_record_defined_p, "c-record-defined-p");
    intern_function(C_record_members, "c-record-members");
    intern_function(C_member_type, "c-member-type");

    // C: introspection of enums
    intern_function(C_enum_p, "c-enum-p");
    intern_function(C_enum_constants, "c-enum-constants");

    // C: introspection of types
    intern_function(C_type_string, "c-type-string");
    intern_function(C_pointer_p, "c-pointer-p");
    intern_function(C_array_p, "c-array-p");
    intern_function(C_const_p, "c-const-p");

    // C: introspection with lists of names
    intern_function(C_functions, "c-functions");
    intern_function(C_variables, "c-variables");
    intern_function(C_records, "c-records");
    intern_function(C_enums, "c-enums");
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
    intern_special_operators();
    intern_preprocessor();
    intern_macros();
    intern_emitters();
    intern_operators();
    declare_standard(this->declarations);
}

ObjectRef<Object> Alma::expand(ObjectRef<Object> obj, ObjectRef<Environment> _environment)
{
    try {
        Expandable* expandable = dynamic_cast<Expandable*>(obj.get());
        if (!expandable)
            return obj;
        return expandable->expand(obj, _environment);
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
    Transformable* transformable = dynamic_cast<Transformable*>(obj.get());
    if (!transformable)
        athrow("The object " << this->to_string(obj) << " is not transformable");
    return transformable->transform(obj, arg_list, _environment);
}

ObjectRef<Object> Alma::transform(
    ObjectRef<Object> obj,
    ObjectRef<Cons> args,
    ObjectRef<Environment> _environment)
{
    // A call without arguments receives nil instead of a list
    if (this->null(args))
        return this->transform(obj, std::vector<ObjectRef<Object>> {}, _environment);
    auto [arg_list, arg_rest] = args->to_list();
    aassert(!arg_rest, "Cannot transform a non proper list of arguments " << args);
    return this->transform(obj, arg_list, _environment);
}

ObjectRef<Object> Alma::apply(
    ObjectRef<Object> obj,
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> _environment)
{
    Applicable* applicable = dynamic_cast<Applicable*>(obj.get());
    if (!applicable)
        athrow("The object " << this->to_string(obj) << " is not applicable");
    return applicable->apply(obj, arg_list, _environment);
}

ObjectRef<Object> Alma::apply(
    ObjectRef<Object> obj,
    ObjectRef<Cons> args,
    ObjectRef<Environment> _environment)
{
    // A call without arguments receives nil instead of a list
    if (this->null(args))
        return this->apply(obj, std::vector<ObjectRef<Object>> {}, _environment);
    auto [arg_list, arg_rest] = args->to_list();
    aassert(!arg_rest, "Cannot apply a non proper list of arguments " << args);
    return this->apply(obj, arg_list, _environment);
}

static Emittable& as_emittable(Alma& alma, ObjectRef<Object> obj)
{
    Emittable* emittable = dynamic_cast<Emittable*>(obj.get());
    if (!emittable)
        athrow("The object " << alma.to_string(obj) << " is not emittable");
    return *emittable;
}

void Alma::emit(ObjectRef<Object> obj)
{
    // Emit on copies so that a failed emission leaves no partial output or declarations
    Declarations _declarations = this->declarations.deep_copy();
    std::stringstream _output;
    try {
        ObjectRef<Object> expanded = expand_macros(obj);
        this->emit_top_level(expanded, this->environment, _declarations, _output);
    } catch (const std::runtime_error& e) {
        athrow("Error emitting " << this->to_string(obj) << "\n"
                                 << e.what());
    }
    this->declarations = std::move(_declarations);
    this->output << _output.str();
}

void Alma::emit_top_level(ObjectRef<Object> obj, ObjectRef<Environment> _environment,
    Declarations& _declarations, std::stringstream& _output)
{
    as_emittable(*this, obj).emit_top_level(obj, _environment, _declarations, _output);
}

void Alma::emit_statement(ObjectRef<Object> obj, ObjectRef<Environment> _environment,
    Declarations& _declarations, std::stringstream& _output)
{
    as_emittable(*this, obj).emit_statement(obj, _environment, _declarations, _output);
}

std::unique_ptr<Type> Alma::emit_expression(ObjectRef<Object> obj, ObjectRef<Environment> _environment,
    Declarations& _declarations, std::stringstream& _output)
{
    return as_emittable(*this, obj).emit_expression(obj, _environment, _declarations, _output);
}

ObjectRef<Package> Alma::get_current_package()
{
    return this->current_package;
}

std::string Alma::get_c_code()
{
    std::stringstream code;
    for (const std::string& header : this->declarations.get_headers())
        code << "#include <" << header << ">\n";
    if (!this->declarations.get_headers().empty())
        code << "\n";
    code << this->output.str();
    return code.str();
}

std::stringstream& Alma::get_output()
{
    return this->output;
}

Declarations& Alma::get_declarations()
{
    return this->declarations;
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
    if (symbol->has_value())
        symbol->set_value(value);
    else
        symbol->push_value(value);
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
    std::optional<ObjectRef<Object>> sym = package->find_symbol(name);
    if (sym.has_value())
        return *sym;
    else
        return this->boolean(false);
}

ObjectRef<Object> Alma::find_symbol(const std::string& name)
{
    return this->find_symbol(name, this->current_package);
}

ObjectRef<Object> Alma::find_alma_symbol(const std::string& name)
{
    return this->find_symbol(name, this->alma_package);
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
