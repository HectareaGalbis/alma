
#pragma once

#include "gc/gc.hpp"
#include "object/object.hpp"
#include <filesystem>

class Environment;
class Package;
class Procedure;
class Symbol;
class Object;

class Alma {
    friend class Object;

private:
    GarbageCollector gc;
    ObjectRef<Environment> environment;
    ObjectRef<Package> alma_package;
    ObjectRef<Package> current_package;

private:
    static Alma* global_alma;

public:
    class WithAlma {
    public:
        WithAlma(Alma& alma);
        ~WithAlma();
    };

private:
    void intern_functions();
    void intern_symbols();

public:
    Alma();

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

    /** Return the current package */
    ObjectRef<Package> get_current_package();

    /** Return a string representation of an object */
    std::string to_string(ObjectRef<Object> obj);

    /** Check the type of an object */
    bool typep(ObjectRef<Object> obj, ObjectRef<Object> sym);
    bool alma_typep(ObjectRef<Object> obj, const std::string& type);

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
    ObjectRef<T> obj(*this, this->gc.make_object<T>(*this, std::forward<AS>(as)...));
    ObjectRef<T> result(obj);
    this->gc.try_collect();
    return result;
}
