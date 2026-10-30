
#pragma once

#include "gc/gc.hpp"
#include "interpiler/declaration.hpp"
#include "interpiler/emitter.hpp"
#include "object/cons.hpp"
#include "object/nil.hpp"
#include "object/object.hpp"
#include "object/type_system.hpp"
#include <filesystem>
#include <map>
#include <optional>
#include <sstream>
#include <vector>

class Environment;
class Package;
class Procedure;
class Symbol;
class Object;

class Alma {
    friend class Object;

public:
private:
    GarbageCollector gc;
    ObjectRef<Environment> environment;
    ObjectRef<Package> alma_package;
    ObjectRef<Package> keyword_package;
    std::map<std::string, ObjectRef<Package>> packages;
    ObjectRef<Nil> nil;
    ObjectRef<TypeSystem> type_system;
    std::stringstream output;
    Declarations declarations;

private:
    void intern_functions();
    void intern_symbols();

private:
    Alma();

public:
    static Alma alma;

public:
    /** Allocates an object */
    template <typename T, typename... AS>
    ObjectRef<T> make(AS&&... as);

    /** Expands an object */
    ObjectRef<Object> expand(ObjectRef<Object> obj, ObjectRef<Environment> environment);
    ObjectRef<Object> expand(ObjectRef<Object> obj);

    /** Evaluates an object */
    ObjectRef<Object> eval(ObjectRef<Object> obj, ObjectRef<Environment> environment);
    ObjectRef<Object> eval(ObjectRef<Object> obj);

    /** Loads a file */
    void load(const std::filesystem::path& path);

    /** Transform an object */
    ObjectRef<Object> transform(
        ObjectRef<Object> obj,
        const std::vector<ObjectRef<Object>>& arg_list,
        ObjectRef<Environment> environment);
    ObjectRef<Object> transform(
        ObjectRef<Object> obj,
        ObjectRef<Cons> args,
        ObjectRef<Environment> environment);

    /** Apply an object */
    ObjectRef<Object> apply(
        ObjectRef<Object> obj,
        const std::vector<ObjectRef<Object>>& arg_list,
        ObjectRef<Environment> environment);
    ObjectRef<Object> apply(
        ObjectRef<Object> obj,
        ObjectRef<Cons> args,
        ObjectRef<Environment> environment);

    /** Emit an object as top level C code, appending it to the output */
    void emit(ObjectRef<Object> obj);

    /** Emit an object as a top level C construction, a statement or an expression */
    void emit_top_level(ObjectRef<Object> obj, ObjectRef<Environment> environment,
        Declarations& declarations, std::stringstream& output);
    void emit_statement(ObjectRef<Object> obj, ObjectRef<Environment> environment,
        Declarations& declarations, std::stringstream& output);
    std::unique_ptr<Type> emit_expression(ObjectRef<Object> obj, ObjectRef<Environment> environment,
        Declarations& declarations, std::stringstream& output);

    /** Return the current package, the value of *package* */
    ObjectRef<Package> get_current_package();

    /** Return the current package, or nothing if *package* is not a package */
    std::optional<ObjectRef<Package>> find_current_package();

    /** Change the value of *package* */
    void set_current_package(ObjectRef<Package> package);

    /** Find a package by its local name in the current package or by its name */
    std::optional<ObjectRef<Package>> find_package(const std::string& name);

    /** Create, delete and rename packages */
    ObjectRef<Package> make_package(const std::string& name);
    void delete_package(ObjectRef<Package> package);
    void rename_package(ObjectRef<Package> package, const std::string& name);

    /** Return every package */
    std::vector<ObjectRef<Package>> list_all_packages();

    /** Intern a symbol in the keyword package */
    ObjectRef<Symbol> intern_keyword(const std::string& name);

    /** Return the output */
    std::stringstream& get_output();

    /** Return the emitted C code, starting with the includes of the standard library it uses */
    std::string get_c_code();

    /** Return the declarations */
    Declarations& get_declarations();

    /** Return a string representation of an object */
    std::string to_string(ObjectRef<Object> obj);

    /** Return a symbol representing the type of an object */
    ObjectRef<Object> type(ObjectRef<Object> obj);

    /** Check if an object is of a type */
    bool typep(ObjectRef<Object> obj, ObjectRef<Object> type);

    /** Check if an object is a symbol */
    bool symbolp(ObjectRef<Object> obj);

    /** Get the value of a symbol */
    ObjectRef<Object> symbol_value(ObjectRef<Symbol> symbol);

    /** Set the value of a symbol */
    ObjectRef<Object> set_symbol_value(ObjectRef<Symbol> symbol, ObjectRef<Object> value);

    /** Check if an object is a cons */
    bool consp(ObjectRef<Object> obj);

    /** Return false for the nil object and true otherwise */
    bool truep(ObjectRef<Object> obj);

    /** Return true for the nil object and false otherwise */
    bool null(ObjectRef<Object> obj);

    /** Return the alma's boolean objects */
    ObjectRef<Object> boolean(bool v);

    /** Check if two objects are the same */
    bool eq(ObjectRef<Object> obj1, ObjectRef<Object> obj2);

    /** Assign a value to a symbol */
    ObjectRef<Object> setq(ObjectRef<Symbol> symbol, ObjectRef<Object> value,
        ObjectRef<Environment> environment);
    ObjectRef<Object> setq(ObjectRef<Symbol> symbol, ObjectRef<Object> value);

    /** Return a symbol */
    ObjectRef<Object> find_symbol(const std::string& name, ObjectRef<Package> package);

    /** Return a symbol from the current packge */
    ObjectRef<Object> find_symbol(const std::string& name);

    /** Return a symbol from the alma package */
    ObjectRef<Object> find_alma_symbol(const std::string& name);

    /** Intern a symbol */
    ObjectRef<Object> intern_symbol(const std::string& name, ObjectRef<Package> package);

    /** Intern a symbol in the current package */
    ObjectRef<Object> intern_symbol(const std::string& name);

    /** Intern a symbol in the alma package */
    ObjectRef<Object> intern_alma_symbol(const std::string& name);

    /** Return the procedure associated to a character */
    ObjectRef<Object> find_character_macro(char c, ObjectRef<Package> package);

    /** Return the procedure associated to a character from the current package */
    ObjectRef<Object> find_character_macro(char c);

    /** Return the procedure associated to a character from the alma package */
    ObjectRef<Object> find_alma_character_macro(char c);

    /** Associate a procedure to a character */
    ObjectRef<Object> intern_character_macro(char c, ObjectRef<Procedure> proc, ObjectRef<Package> package);

    /** Associate a procedure to a character in the current package */
    ObjectRef<Object> intern_character_macro(char c, ObjectRef<Procedure> proc);

    /** Associate a procedure to a character in the alma package */
    ObjectRef<Object> intern_alma_character_macro(char c, ObjectRef<Procedure> proc);

    /** Return each element of a cons */
    ObjectRef<Object> car(ObjectRef<Cons> c);
    ObjectRef<Object> cdr(ObjectRef<Cons> c);
};

template <typename T, typename... AS>
ObjectRef<T> Alma::make(AS&&... as)
{
    ObjectRef<T> result(this->gc.make_object<T>(std::forward<AS>(as)...));
    this->gc.try_collect();
    return result;
}
