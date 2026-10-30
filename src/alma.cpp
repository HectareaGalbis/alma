

#include "alma.hpp"
#include <algorithm>
#include <sys/resource.h>
#include "debug.hpp"
#include "interfaces/applicable.hpp"
#include "interfaces/emittable.hpp"
#include "interfaces/expandable.hpp"
#include "interfaces/transformable.hpp"
#include "interpiler/operators.hpp"
#include "interpiler/preprocessor.hpp"
#include "interpiler/standard.hpp"
#include "object/cons.hpp"
#include "object/environment.hpp"
#include "object/function.hpp"
#include "object/macro.hpp"
#include "object/object.hpp"
#include "object/package.hpp"
#include "object/special_operator.hpp"
#include "object/string.hpp"
#include "object/symbol.hpp"
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
    intern_function(Load, "load");
    intern_function(Import, "import-project");
    intern_function(Exit, "exit");

    // Alma: lists
    intern_function(Cons_create, "cons");
    intern_function(Consp, "consp");
    intern_function(Listp, "listp");
    intern_function(Null, "null");
    intern_function(Endp, "endp");
    intern_function(Atom, "atom");
    intern_function(First, "first");
    intern_function(Second, "second");
    intern_function(Third, "third");
    intern_function(Rest, "rest");
    intern_function(Nth, "nth");
    intern_function(Nthcdr, "nthcdr");
    intern_function(Last, "last");
    intern_function(Length, "length");
    intern_function(Append, "append");
    intern_function(Reverse, "reverse");
    intern_function(Member, "member");
    intern_function(Rplaca, "rplaca");
    intern_function(Rplacd, "rplacd");

    // Alma: packages
    intern_function(Make_package, "make-package");
    intern_function(Find_package, "find-package");
    intern_function(Delete_package, "delete-package");
    intern_function(Rename_package, "rename-package");
    intern_function(Package_name, "package-name");
    intern_function(List_all_packages, "list-all-packages");
    intern_function(In_package, "in-package");
    intern_function(Symbol_package, "symbol-package");
    intern_function(Intern, "intern");
    intern_function(Find_symbol, "find-symbol");
    intern_function(Unintern, "unintern");
    intern_function(Make_symbol, "make-symbol");
    intern_function(Gensym, "gensym");
    intern_function(Export, "export");
    intern_function(Unexport, "unexport");
    intern_function(Import_symbol, "import");
    intern_function(Shadow, "shadow");
    intern_function(Shadowing_import, "shadowing-import");
    intern_function(Use_package, "use-package");
    intern_function(Unuse_package, "unuse-package");
    intern_function(Package_use_list, "package-use-list");
    intern_function(Package_used_by_list, "package-used-by-list");
    intern_function(Package_shadowing_symbols, "package-shadowing-symbols");
    intern_function(Add_package_local_name, "add-package-local-name");
    intern_function(Remove_package_local_name, "remove-package-local-name");
    intern_function(Package_local_names, "package-local-names");

    // Alma: equality and hash tables
    intern_function(Eql, "eql");
    intern_function(Equal, "equal");
    intern_function(Equalp, "equalp");
    intern_function(Make_hash_table, "make-hash-table");
    intern_function(Hash_table_p, "hash-table-p");
    intern_function(Gethash, "gethash");
    intern_function(Puthash, "puthash");
    intern_function(Remhash, "remhash");
    intern_function(Clrhash, "clrhash");
    intern_function(Maphash, "maphash");
    intern_function(Hash_table_count, "hash-table-count");
    intern_function(Hash_table_test, "hash-table-test");
    intern_function(Hash_table_contains_p, "hash-table-contains-p");

    // Alma: character macros
    intern_function(Set_character_macro, "set-character-macro");
    intern_function(Find_character_macro, "find-character-macro");
    intern_function(Export_character_macro, "export-character-macro");
    intern_function(Unexport_character_macro, "unexport-character-macro");
    intern_function(Import_character_macro, "import-character-macro");
    this->intern_alma_character_macro('(', this->make<Read_vector>());
    this->intern_alma_character_macro('{', this->make<Read_hash_table>());

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
    for (const char* name : { "&rest", "quote", "quasiquote", "unquote", "slice-unquote", "const", "*" })
        this->intern_alma_symbol(name);
    ObjectRef<Symbol> current_file = this->intern_alma_symbol("*current-file*");
    current_file->push_value(this->intern_alma_symbol("nil"));
    ObjectRef<Symbol> source_distribution = this->intern_alma_symbol("*source-distribution*");
    source_distribution->push_value(this->make<String>("https://github.com/HectareaGalbis/alma-projects"));
}

Alma::Alma()
    : gc()
    , environment(gc.make_object<Environment>())
    , alma_package(gc.make_object<Package>("alma"))
    , keyword_package(gc.make_object<Package>("keyword", true))
    , nil(gc.make_object<Nil>())
    , type_system(gc.make_object<TypeSystem>())
{
    // The stack grows down from here. Leave a margin to report the error
    char marker;
    this->stack_base = reinterpret_cast<std::uintptr_t>(&marker);
    std::size_t stack_size = 8 * 1024 * 1024;
    struct rlimit limit;
    if (getrlimit(RLIMIT_STACK, &limit) == 0 && limit.rlim_cur != RLIM_INFINITY)
        stack_size = limit.rlim_cur;
    std::size_t margin = std::min<std::size_t>(stack_size / 4, 256 * 1024);
    this->stack_limit = stack_size - margin;

    this->packages.emplace("alma", this->alma_package);
    this->packages.emplace("keyword", this->keyword_package);
    ObjectRef<Package> user_package = this->make_package("alma-user");
    user_package->use_package(this->alma_package);
    ObjectRef<Symbol> package_symbol = this->intern_alma_symbol("*package*");
    package_symbol->push_value(user_package);
    this->intern_symbols();
    this->intern_functions();
    intern_special_operators();
    intern_preprocessor();
    intern_macros();
    intern_emitters();
    intern_operators();
    declare_standard(this->declarations);
}

void Alma::check_stack()
{
    char marker;
    std::uintptr_t used = this->stack_base - reinterpret_cast<std::uintptr_t>(&marker);
    if (used > this->stack_limit)
        throw StackOverflow("The stack is exhausted, maybe because of an infinite recursion\n");
}

ObjectRef<Object> Alma::expand(ObjectRef<Object> obj, ObjectRef<Environment> _environment)
{
    this->check_stack();
    try {
        Expandable* expandable = dynamic_cast<Expandable*>(obj.get());
        if (!expandable)
            return obj;
        return expandable->expand(obj, _environment);
    } catch (const StackOverflow&) {
        throw;
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
    this->check_stack();
    try {
        return obj->eval(obj, lex_environment);
    } catch (const StackOverflow&) {
        throw;
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
    aassert(input.is_open(), "Cannot open the file " << path.native());
    Reader reader(*this, path.native(), input);
    ObjectRef<Symbol> current_file = this->intern_alma_symbol("*current-file*");
    current_file->push_value(this->make<String>(std::filesystem::absolute(path).lexically_normal().native()));
    ObjectRef<Symbol> package_symbol = this->intern_alma_symbol("*package*");
    package_symbol->push_value(this->get_current_package());
    try {
        for (std::optional<ObjectRef<Object>> obj = reader.read(); obj; obj = reader.read())
            this->eval(*obj);
    } catch (...) {
        current_file->pop_value();
        package_symbol->pop_value();
        throw;
    }
    current_file->pop_value();
    package_symbol->pop_value();
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
    aassert(this->consp(args), "Cannot transform a non proper list of arguments " << args);
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
    aassert(this->consp(args), "Cannot apply a non proper list of arguments " << args);
    auto [arg_list, arg_rest] = args->to_list();
    aassert(!arg_rest, "Cannot apply a non proper list of arguments " << args);
    return this->apply(obj, arg_list, _environment);
}

ObjectRef<Object> Alma::funcall(ObjectRef<Object> function, const std::vector<ObjectRef<Object>>& arg_list)
{
    aassert(this->typep(function, this->intern_alma_symbol("function")),
        "The object " << this->to_string(function) << " is not a function");
    std::vector<ObjectRef<Object>> quoted_args;
    for (const ObjectRef<Object>& arg : arg_list)
        quoted_args.push_back(this->make<Cons>(std::vector<ObjectRef<Object>> { this->intern_alma_symbol("quote"), arg }));
    return this->apply(function, quoted_args, this->environment);
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
    } catch (const StackOverflow&) {
        throw;
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
    this->check_stack();
    as_emittable(*this, obj).emit_statement(obj, _environment, _declarations, _output);
}

std::unique_ptr<Type> Alma::emit_expression(ObjectRef<Object> obj, ObjectRef<Environment> _environment,
    Declarations& _declarations, std::stringstream& _output)
{
    this->check_stack();
    return as_emittable(*this, obj).emit_expression(obj, _environment, _declarations, _output);
}

ObjectRef<Package> Alma::get_current_package()
{
    std::optional<ObjectRef<Package>> package = this->find_current_package();
    aassert(package, "The value of *package* is not a package");
    return *package;
}

std::optional<ObjectRef<Package>> Alma::find_current_package()
{
    ObjectRef<Symbol> package_symbol = this->intern_alma_symbol("*package*");
    if (!package_symbol->has_value())
        return std::nullopt;
    ObjectRef<Object> value = package_symbol->get_value();
    if (this->type(value) != this->intern_alma_symbol("package") || value.as<Package>()->is_deleted())
        return std::nullopt;
    return ObjectRef<Package>(value);
}

void Alma::set_current_package(ObjectRef<Package> package)
{
    aassert(!package->is_deleted(), "Cannot use a deleted package");
    this->set_symbol_value(this->intern_alma_symbol("*package*"), package);
}

std::optional<ObjectRef<Package>> Alma::find_package(const std::string& name)
{
    std::optional<ObjectRef<Package>> current = this->find_current_package();
    if (current) {
        std::optional<ObjectRef<Package>> local = (*current)->find_local_name(name);
        if (local)
            return local;
    }
    auto it = this->packages.find(name);
    if (it == this->packages.end())
        return std::nullopt;
    return it->second;
}

ObjectRef<Package> Alma::make_package(const std::string& name)
{
    aassert(!name.empty(), "The name of a package cannot be empty");
    aassert(!this->packages.contains(name), "The package " << name << " already exists");
    ObjectRef<Package> package = this->make<Package>(name);
    this->packages.emplace(name, package);
    return package;
}

void Alma::delete_package(ObjectRef<Package> package)
{
    aassert(package != this->alma_package && package != this->keyword_package,
        "Cannot delete the package " << package->get_name());
    std::optional<ObjectRef<Package>> current = this->find_current_package();
    aassert(!current || *current != package, "Cannot delete the current package " << package->get_name());
    for (auto& [name, other] : this->packages)
        for (auto& [local_name, local_package] : other->get_local_names())
            if (local_package == package)
                other->remove_local_name(local_name);
    this->packages.erase(package->get_name());
    package->destroy();
}

void Alma::rename_package(ObjectRef<Package> package, const std::string& name)
{
    aassert(!package->is_deleted(), "Cannot rename a deleted package");
    aassert(!name.empty(), "The name of a package cannot be empty");
    if (name == package->get_name())
        return;
    aassert(!this->packages.contains(name), "The package " << name << " already exists");
    this->packages.erase(package->get_name());
    package->set_name(name);
    this->packages.emplace(name, package);
}

std::vector<ObjectRef<Package>> Alma::list_all_packages()
{
    std::vector<ObjectRef<Package>> result;
    for (auto& [name, package] : this->packages)
        result.push_back(package);
    return result;
}

ObjectRef<Symbol> Alma::intern_keyword(const std::string& name)
{
    return this->keyword_package->intern_symbol(name);
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
    this->check_stack();
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

bool Alma::keywordp(ObjectRef<Object> obj)
{
    if (!this->symbolp(obj))
        return false;
    std::optional<ObjectRef<Package>> home = obj.as<Symbol>()->get_home_package();
    return home && *home == this->keyword_package;
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
        this->set_symbol_value(symbol, value);
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
    return this->find_symbol(name, this->get_current_package());
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
    return this->get_current_package()->intern_symbol(name);
}

ObjectRef<Object> Alma::intern_alma_symbol(const std::string& name)
{
    return this->alma_package->intern_external_symbol(name);
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
    return this->find_character_macro(c, this->get_current_package());
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
    return this->intern_character_macro(c, proc, this->get_current_package());
}

ObjectRef<Object> Alma::intern_alma_character_macro(char c, ObjectRef<Procedure> proc)
{
    ObjectRef<Object> result = this->intern_character_macro(c, proc, this->alma_package);
    this->alma_package->export_character_macro(c);
    return result;
}

ObjectRef<Object> Alma::car(ObjectRef<Cons> c)
{
    return c->get_car();
}

ObjectRef<Object> Alma::cdr(ObjectRef<Cons> c)
{
    return c->get_cdr();
}
