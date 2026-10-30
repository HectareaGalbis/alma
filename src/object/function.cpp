
#include "function.hpp"
#include "alma.hpp"
#include "hash_table.hpp"
#include "character.hpp"
#include "cons.hpp"
#include "debug.hpp"
#include "integer.hpp"
#include "object.hpp"
#include "package.hpp"
#include "string.hpp"
#include "symbol.hpp"
#include "vector.hpp"
#include "interpiler/declaration.hpp"
#include "reader.hpp"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

// ================================================================================
// Alma
// ================================================================================

// --------------------------------------------------------------------------------

ObjectRef<Object> Sum::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, {}, "integer");
    int64_t sum_value = 0;
    for (ObjectRef<Object> arg : arg_list) {
        sum_value += arg.as<Integer>()->get_value();
    }
    return Alma::alma.make<Integer>(sum_value);
}

// --------------------------------------------------------------------------------

ObjectRef<Object> Print::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "t" });
    std::cout << Alma::alma.to_string(arg_list.front()) << std::endl;
    return arg_list.front();
}

// --------------------------------------------------------------------------------

// ObjectRef<Object> Type::eval_body(
//     const std::vector<ObjectRef<Object>>& arg_list,
//     ObjectRef<Environment> environment [[maybe_unused]])
// {
//     this->check_types(arg_list, { "t" });
//     return Alma::alma.type(arg_list[0]);
// }

// --------------------------------------------------------------------------------

ObjectRef<Object> Set_symbol_value::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "symbol", "t" });
    return Alma::alma.set_symbol_value(arg_list[0], arg_list[1]);
}

// --------------------------------------------------------------------------------

ObjectRef<Object> Setq::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "symbol", "t" });
    return Alma::alma.setq(arg_list[0], arg_list[1]);
}

// --------------------------------------------------------------------------------

ObjectRef<Object> Car::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "t" });
    if (Alma::alma.null(arg_list[0]))
        return arg_list[0];
    aassert(Alma::alma.consp(arg_list[0]), "The argument number 0 must be a list");
    return Alma::alma.car(arg_list[0]);
}

// --------------------------------------------------------------------------------

ObjectRef<Object> Cdr::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "t" });
    if (Alma::alma.null(arg_list[0]))
        return arg_list[0];
    aassert(Alma::alma.consp(arg_list[0]), "The argument number 0 must be a list");
    return Alma::alma.cdr(arg_list[0]);
}

// --------------------------------------------------------------------------------

ObjectRef<Object> Eq::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "t", "t" });
    return Alma::alma.boolean(Alma::alma.eq(arg_list[0], arg_list[1]));
}

// --------------------------------------------------------------------------------

ObjectRef<Object> Eval::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment)
{
    this->check_types(arg_list, { "t" });

    return Alma::alma.eval(arg_list[0], environment);
}

// --------------------------------------------------------------------------------

ObjectRef<Object> List::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    if (arg_list.empty())
        return Alma::alma.intern_alma_symbol("nil");
    return Alma::alma.make<Cons>(arg_list);
}

// ================================================================================
// Alma: lists
// ================================================================================

static bool listp(ObjectRef<Object> obj)
{
    return Alma::alma.null(obj) || Alma::alma.consp(obj);
}

static void assert_list(ObjectRef<Object> obj)
{
    aassert(listp(obj), "Expected a list but found " << Alma::alma.to_string(obj));
}

static std::vector<ObjectRef<Object>> proper_list_elements(ObjectRef<Object> obj)
{
    if (Alma::alma.null(obj))
        return {};
    aassert(Alma::alma.consp(obj), "Expected a list but found " << Alma::alma.to_string(obj));
    auto [list, tail] = obj.as<Cons>()->to_list();
    aassert(Alma::alma.null(tail), "Expected a proper list but found " << Alma::alma.to_string(obj));
    return list;
}

static ObjectRef<Object> make_list(const std::vector<ObjectRef<Object>>& elements, ObjectRef<Object> tail)
{
    if (elements.empty())
        return tail;
    return Alma::alma.make<Cons>(elements, tail);
}

static int64_t as_list_index(ObjectRef<Object> obj)
{
    int64_t index = obj.as<Integer>()->get_value();
    aassert(index >= 0, "Expected a non negative index but found " << index);
    return index;
}

static ObjectRef<Object> list_nthcdr(int64_t n, ObjectRef<Object> list)
{
    assert_list(list);
    for (int64_t i = 0; i < n && Alma::alma.consp(list); ++i) {
        list = list.as<Cons>()->get_cdr();
        if (i + 1 < n)
            assert_list(list);
    }
    return list;
}

static ObjectRef<Object> list_nth(int64_t n, ObjectRef<Object> list)
{
    ObjectRef<Object> tail = list_nthcdr(n, list);
    if (Alma::alma.consp(tail))
        return tail.as<Cons>()->get_car();
    return Alma::alma.intern_alma_symbol("nil");
}

// --------------------------------------------------------------------------------

ObjectRef<Object> Cons_create::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "t", "t" });
    return Alma::alma.make<Cons>(arg_list[0], arg_list[1]);
}

ObjectRef<Object> Consp::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "t" });
    return Alma::alma.boolean(Alma::alma.consp(arg_list[0]));
}

ObjectRef<Object> Listp::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "t" });
    return Alma::alma.boolean(listp(arg_list[0]));
}

ObjectRef<Object> Null::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "t" });
    return Alma::alma.boolean(Alma::alma.null(arg_list[0]));
}

ObjectRef<Object> Endp::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "t" });
    assert_list(arg_list[0]);
    return Alma::alma.boolean(Alma::alma.null(arg_list[0]));
}

ObjectRef<Object> Atom::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "t" });
    return Alma::alma.boolean(!Alma::alma.consp(arg_list[0]));
}

ObjectRef<Object> First::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "t" });
    return list_nth(0, arg_list[0]);
}

ObjectRef<Object> Second::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "t" });
    return list_nth(1, arg_list[0]);
}

ObjectRef<Object> Third::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "t" });
    return list_nth(2, arg_list[0]);
}

ObjectRef<Object> Rest::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "t" });
    return list_nthcdr(1, arg_list[0]);
}

ObjectRef<Object> Nth::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "integer", "t" });
    return list_nth(as_list_index(arg_list[0]), arg_list[1]);
}

ObjectRef<Object> Nthcdr::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "integer", "t" });
    return list_nthcdr(as_list_index(arg_list[0]), arg_list[1]);
}

ObjectRef<Object> Last::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "t" });
    ObjectRef<Object> list = arg_list[0];
    assert_list(list);
    while (Alma::alma.consp(list) && Alma::alma.consp(list.as<Cons>()->get_cdr()))
        list = list.as<Cons>()->get_cdr();
    return list;
}

ObjectRef<Object> Length::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "t" });
    return Alma::alma.make<Integer>(static_cast<int64_t>(proper_list_elements(arg_list[0]).size()));
}

ObjectRef<Object> Append::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    if (arg_list.empty())
        return Alma::alma.intern_alma_symbol("nil");
    std::vector<ObjectRef<Object>> elements;
    for (size_t i = 0; i + 1 < arg_list.size(); ++i) {
        std::vector<ObjectRef<Object>> list = proper_list_elements(arg_list[i]);
        elements.insert(elements.end(), list.begin(), list.end());
    }
    return make_list(elements, arg_list.back());
}

ObjectRef<Object> Reverse::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "t" });
    std::vector<ObjectRef<Object>> elements = proper_list_elements(arg_list[0]);
    std::reverse(elements.begin(), elements.end());
    return make_list(elements, Alma::alma.intern_alma_symbol("nil"));
}

ObjectRef<Object> Member::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "t", "t" });
    ObjectRef<Object> list = arg_list[1];
    assert_list(list);
    while (Alma::alma.consp(list)) {
        if (Alma::alma.eq(list.as<Cons>()->get_car(), arg_list[0]))
            return list;
        list = list.as<Cons>()->get_cdr();
        assert_list(list);
    }
    return Alma::alma.intern_alma_symbol("nil");
}

ObjectRef<Object> Rplaca::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "cons", "t" });
    arg_list[0].as<Cons>()->set_car(arg_list[1]);
    return arg_list[0];
}

ObjectRef<Object> Rplacd::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "cons", "t" });
    arg_list[0].as<Cons>()->set_cdr(arg_list[1]);
    return arg_list[0];
}

// ================================================================================
// Alma: packages
// ================================================================================

static void check_arity(const std::vector<ObjectRef<Object>>& arg_list, size_t min, size_t max)
{
    aassert(arg_list.size() >= min && arg_list.size() <= max,
        "Expected between " << min << " and " << max << " arguments but received " << arg_list.size());
}

static bool packagep(ObjectRef<Object> obj)
{
    return Alma::alma.type(obj) == Alma::alma.intern_alma_symbol("package");
}

static std::string string_designator(ObjectRef<Object> obj)
{
    if (Alma::alma.typep(obj, Alma::alma.intern_alma_symbol("string")))
        return obj.as<String>()->get_content();
    if (Alma::alma.symbolp(obj))
        return obj.as<Symbol>()->get_name();
    athrow("Expected a string or a symbol but found " << Alma::alma.to_string(obj));
}

static ObjectRef<Package> package_designator(ObjectRef<Object> obj)
{
    if (packagep(obj)) {
        aassert(!obj.as<Package>()->is_deleted(), "The package has been deleted");
        return obj;
    }
    std::string name = string_designator(obj);
    std::optional<ObjectRef<Package>> package = Alma::alma.find_package(name);
    aassert(package, "The package " << name << " does not exist");
    return *package;
}

static ObjectRef<Package> optional_package(const std::vector<ObjectRef<Object>>& arg_list, size_t index)
{
    if (index < arg_list.size())
        return package_designator(arg_list[index]);
    return Alma::alma.get_current_package();
}

static std::vector<ObjectRef<Object>> designator_list(ObjectRef<Object> obj)
{
    if (Alma::alma.null(obj))
        return {};
    if (Alma::alma.consp(obj))
        return proper_list_elements(obj);
    return { obj };
}

static ObjectRef<Symbol> as_symbol(ObjectRef<Object> obj)
{
    aassert(Alma::alma.symbolp(obj), "Expected a symbol but found " << Alma::alma.to_string(obj));
    return obj;
}

static ObjectRef<String> as_string(ObjectRef<Object> obj)
{
    aassert(Alma::alma.typep(obj, Alma::alma.intern_alma_symbol("string")),
        "Expected a string but found " << Alma::alma.to_string(obj));
    return obj;
}

template <typename T>
static ObjectRef<Object> to_alma_list(const std::vector<ObjectRef<T>>& elements)
{
    std::vector<ObjectRef<Object>> list(elements.begin(), elements.end());
    return make_list(list, Alma::alma.intern_alma_symbol("nil"));
}

// --------------------------------------------------------------------------------

ObjectRef<Object> Make_package::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "t" }, "t");
    std::string name = string_designator(arg_list[0]);
    aassert(arg_list.size() % 2 == 1, "make-package expects a name followed by keyword arguments");
    std::vector<ObjectRef<Package>> use_list;
    for (size_t i = 1; i < arg_list.size(); i += 2) {
        aassert(arg_list[i].get() == Alma::alma.intern_keyword("use").get(),
            "Unknown keyword argument " << Alma::alma.to_string(arg_list[i]) << " for make-package");
        for (ObjectRef<Object> used : designator_list(arg_list[i + 1]))
            use_list.push_back(package_designator(used));
    }
    ObjectRef<Package> package = Alma::alma.make_package(name);
    for (ObjectRef<Package> used : use_list)
        package->use_package(used);
    return package;
}

ObjectRef<Object> Find_package::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "t" });
    if (packagep(arg_list[0]))
        return arg_list[0];
    std::optional<ObjectRef<Package>> package = Alma::alma.find_package(string_designator(arg_list[0]));
    if (!package)
        return Alma::alma.intern_alma_symbol("nil");
    return *package;
}

ObjectRef<Object> Delete_package::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "t" });
    if (packagep(arg_list[0]) && arg_list[0].as<Package>()->is_deleted())
        return Alma::alma.boolean(false);
    Alma::alma.delete_package(package_designator(arg_list[0]));
    return Alma::alma.boolean(true);
}

ObjectRef<Object> Rename_package::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "t", "t" });
    ObjectRef<Package> package = package_designator(arg_list[0]);
    Alma::alma.rename_package(package, string_designator(arg_list[1]));
    return package;
}

ObjectRef<Object> Package_name::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "t" });
    if (packagep(arg_list[0]) && arg_list[0].as<Package>()->is_deleted())
        return Alma::alma.intern_alma_symbol("nil");
    return Alma::alma.make<String>(package_designator(arg_list[0])->get_name());
}

ObjectRef<Object> List_all_packages::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, {});
    return to_alma_list(Alma::alma.list_all_packages());
}

ObjectRef<Object> In_package::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "t" });
    ObjectRef<Package> package = package_designator(arg_list[0]);
    Alma::alma.set_current_package(package);
    return package;
}

ObjectRef<Object> Symbol_package::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "symbol" });
    std::optional<ObjectRef<Package>> home = arg_list[0].as<Symbol>()->get_home_package();
    if (!home)
        return Alma::alma.intern_alma_symbol("nil");
    return *home;
}

ObjectRef<Object> Intern::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    check_arity(arg_list, 1, 2);
    std::string name = as_string(arg_list[0])->get_content();
    return optional_package(arg_list, 1)->intern_symbol(name);
}

ObjectRef<Object> Find_symbol::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    check_arity(arg_list, 1, 2);
    std::string name = as_string(arg_list[0])->get_content();
    return Alma::alma.find_symbol(name, optional_package(arg_list, 1));
}

ObjectRef<Object> Unintern::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    check_arity(arg_list, 1, 2);
    ObjectRef<Symbol> symbol = as_symbol(arg_list[0]);
    return Alma::alma.boolean(optional_package(arg_list, 1)->unintern_symbol(symbol));
}

ObjectRef<Object> Make_symbol::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    check_arity(arg_list, 1, 1);
    return Alma::alma.make<Symbol>(as_string(arg_list[0])->get_content());
}

ObjectRef<Object> Gensym::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    static uint64_t counter = 0;
    check_arity(arg_list, 0, 1);
    std::string prefix = arg_list.empty() ? "G" : as_string(arg_list[0])->get_content();
    return Alma::alma.make<Symbol>(prefix + std::to_string(counter++));
}

ObjectRef<Object> Export::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    check_arity(arg_list, 1, 2);
    ObjectRef<Package> package = optional_package(arg_list, 1);
    for (ObjectRef<Object> symbol : designator_list(arg_list[0]))
        package->export_symbol(as_symbol(symbol));
    return Alma::alma.boolean(true);
}

ObjectRef<Object> Unexport::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    check_arity(arg_list, 1, 2);
    ObjectRef<Package> package = optional_package(arg_list, 1);
    for (ObjectRef<Object> symbol : designator_list(arg_list[0]))
        package->unexport_symbol(as_symbol(symbol));
    return Alma::alma.boolean(true);
}

ObjectRef<Object> Import_symbol::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    check_arity(arg_list, 1, 2);
    ObjectRef<Package> package = optional_package(arg_list, 1);
    for (ObjectRef<Object> symbol : designator_list(arg_list[0]))
        package->import_symbol(as_symbol(symbol));
    return Alma::alma.boolean(true);
}

ObjectRef<Object> Shadow::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    check_arity(arg_list, 1, 2);
    ObjectRef<Package> package = optional_package(arg_list, 1);
    for (ObjectRef<Object> name : designator_list(arg_list[0]))
        package->shadow(string_designator(name));
    return Alma::alma.boolean(true);
}

ObjectRef<Object> Shadowing_import::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    check_arity(arg_list, 1, 2);
    ObjectRef<Package> package = optional_package(arg_list, 1);
    for (ObjectRef<Object> symbol : designator_list(arg_list[0]))
        package->shadowing_import_symbol(as_symbol(symbol));
    return Alma::alma.boolean(true);
}

ObjectRef<Object> Use_package::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    check_arity(arg_list, 1, 2);
    ObjectRef<Package> package = optional_package(arg_list, 1);
    for (ObjectRef<Object> used : designator_list(arg_list[0]))
        package->use_package(package_designator(used));
    return Alma::alma.boolean(true);
}

ObjectRef<Object> Unuse_package::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    check_arity(arg_list, 1, 2);
    ObjectRef<Package> package = optional_package(arg_list, 1);
    for (ObjectRef<Object> used : designator_list(arg_list[0]))
        package->unuse_package(package_designator(used));
    return Alma::alma.boolean(true);
}

ObjectRef<Object> Package_use_list::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "t" });
    return to_alma_list(package_designator(arg_list[0])->get_use_list());
}

ObjectRef<Object> Package_used_by_list::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "t" });
    return to_alma_list(package_designator(arg_list[0])->get_used_by_list());
}

ObjectRef<Object> Package_shadowing_symbols::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "t" });
    return to_alma_list(package_designator(arg_list[0])->get_shadowing_symbols());
}

ObjectRef<Object> Add_package_local_name::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    check_arity(arg_list, 2, 3);
    std::string local_name = string_designator(arg_list[0]);
    ObjectRef<Package> actual = package_designator(arg_list[1]);
    ObjectRef<Package> package = optional_package(arg_list, 2);
    package->add_local_name(local_name, actual);
    return package;
}

ObjectRef<Object> Remove_package_local_name::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    check_arity(arg_list, 1, 2);
    std::string local_name = string_designator(arg_list[0]);
    return Alma::alma.boolean(optional_package(arg_list, 1)->remove_local_name(local_name));
}

ObjectRef<Object> Package_local_names::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "t" });
    std::vector<ObjectRef<Object>> list;
    for (auto& [local_name, package] : package_designator(arg_list[0])->get_local_names())
        list.push_back(Alma::alma.make<Cons>(Alma::alma.make<String>(local_name), package));
    return make_list(list, Alma::alma.intern_alma_symbol("nil"));
}

// ================================================================================
// Alma: equality and hash tables
// ================================================================================

ObjectRef<Object> Eql::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "t", "t" });
    return Alma::alma.boolean(alma_eql(arg_list[0], arg_list[1]));
}

ObjectRef<Object> Equal::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "t", "t" });
    return Alma::alma.boolean(alma_equal(arg_list[0], arg_list[1]));
}

ObjectRef<Object> Equalp::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "t", "t" });
    return Alma::alma.boolean(alma_equalp(arg_list[0], arg_list[1]));
}

static const std::vector<std::pair<const char*, HashTable::Test>> standard_tests = {
    { "eq", HashTable::Test::Eq },
    { "eql", HashTable::Test::Eql },
    { "equal", HashTable::Test::Equal },
    { "equalp", HashTable::Test::Equalp },
};

static std::optional<HashTable::Test> standard_test(ObjectRef<Object> obj)
{
    for (auto& [name, test] : standard_tests) {
        ObjectRef<Symbol> symbol = Alma::alma.intern_alma_symbol(name);
        if (obj.get() == symbol.get() || (symbol->has_value() && obj == symbol->get_value()))
            return test;
    }
    return std::nullopt;
}

static ObjectRef<Object> as_function(ObjectRef<Object> obj)
{
    if (Alma::alma.symbolp(obj) && obj.as<Symbol>()->has_value())
        obj = obj.as<Symbol>()->get_value();
    aassert(Alma::alma.typep(obj, Alma::alma.intern_alma_symbol("function")),
        "Expected a function but found " << Alma::alma.to_string(obj));
    return obj;
}

ObjectRef<Object> Make_hash_table::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    aassert(arg_list.size() % 2 == 0, "make-hash-table expects keyword arguments");
    std::optional<ObjectRef<Object>> test_arg;
    std::optional<ObjectRef<Object>> hash_arg;
    std::optional<size_t> size_arg;
    for (size_t i = 0; i < arg_list.size(); i += 2) {
        ObjectRef<Object> key = arg_list[i];
        ObjectRef<Object> value = arg_list[i + 1];
        if (key.get() == Alma::alma.intern_keyword("test").get()) {
            aassert(!test_arg, "The keyword argument :test is repeated");
            test_arg = value;
        } else if (key.get() == Alma::alma.intern_keyword("hash-function").get()) {
            aassert(!hash_arg, "The keyword argument :hash-function is repeated");
            hash_arg = as_function(value);
        } else if (key.get() == Alma::alma.intern_keyword("size").get()) {
            aassert(!size_arg, "The keyword argument :size is repeated");
            aassert(Alma::alma.typep(value, Alma::alma.intern_alma_symbol("integer"))
                    && value.as<Integer>()->get_value() >= 0,
                "The size of a hash table must be a non negative integer but found " << Alma::alma.to_string(value));
            size_arg = static_cast<size_t>(value.as<Integer>()->get_value());
        } else {
            athrow("Unknown keyword argument " << Alma::alma.to_string(key) << " for make-hash-table");
        }
    }

    HashTable::Test test = HashTable::Test::Eql;
    std::optional<ObjectRef<Object>> test_function;
    std::optional<HashTable::Test> standard = test_arg ? standard_test(*test_arg) : std::nullopt;
    if (test_arg && standard && !hash_arg) {
        test = *standard;
    } else if (test_arg || hash_arg) {
        aassert(hash_arg, "A hash table with a test other than eq, eql, equal or equalp needs a :hash-function");
        test = HashTable::Test::Custom;
        test_function = as_function(test_arg ? *test_arg : Alma::alma.intern_alma_symbol("eql"));
    }
    return Alma::alma.make<HashTable>(test, test_function, hash_arg, size_arg.value_or(0));
}

ObjectRef<Object> Hash_table_p::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "t" });
    return Alma::alma.boolean(Alma::alma.typep(arg_list[0], Alma::alma.intern_alma_symbol("hash-table")));
}

ObjectRef<Object> Gethash::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "t", "hash-table" }, "t");
    aassert(arg_list.size() <= 3, "Expected at most 3 arguments but received " << arg_list.size());
    std::optional<ObjectRef<Object>> value = arg_list[1].as<HashTable>()->get(arg_list[0]);
    if (value)
        return *value;
    return arg_list.size() == 3 ? arg_list[2] : Alma::alma.intern_alma_symbol("nil");
}

ObjectRef<Object> Puthash::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "t", "t", "hash-table" });
    arg_list[2].as<HashTable>()->put(arg_list[0], arg_list[1]);
    return arg_list[1];
}

ObjectRef<Object> Remhash::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "t", "hash-table" });
    return Alma::alma.boolean(arg_list[1].as<HashTable>()->remove(arg_list[0]));
}

ObjectRef<Object> Clrhash::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "hash-table" });
    arg_list[0].as<HashTable>()->clear();
    return arg_list[0];
}

ObjectRef<Object> Maphash::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "t", "hash-table" });
    ObjectRef<Object> function = as_function(arg_list[0]);
    for (auto& [key, value] : arg_list[1].as<HashTable>()->to_vector())
        Alma::alma.funcall(function, { key, value });
    return Alma::alma.intern_alma_symbol("nil");
}

ObjectRef<Object> Hash_table_count::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "hash-table" });
    return Alma::alma.make<Integer>(static_cast<int64_t>(arg_list[0].as<HashTable>()->count()));
}

ObjectRef<Object> Hash_table_test::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "hash-table" });
    ObjectRef<HashTable> table = arg_list[0];
    for (auto& [name, test] : standard_tests)
        if (table->get_test() == test)
            return Alma::alma.intern_alma_symbol(name);
    return *table->get_test_function();
}

ObjectRef<Object> Hash_table_contains_p::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "t", "hash-table" });
    return Alma::alma.boolean(arg_list[1].as<HashTable>()->get(arg_list[0]).has_value());
}

// ================================================================================
// Alma: character macros
// ================================================================================

ObjectRef<Object> Read_vector::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "t" });
    return Alma::alma.make<Vector>(proper_list_elements(arg_list[0]));
}

ObjectRef<Object> Read_hash_table::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "t" });
    std::vector<ObjectRef<Object>> elements = proper_list_elements(arg_list[0]);
    aassert(elements.size() % 2 == 0, "A hash table needs a value for each key");
    ObjectRef<HashTable> table = Alma::alma.make<HashTable>(HashTable::Test::Equal, std::nullopt, std::nullopt, elements.size() / 2);
    for (size_t i = 0; i < elements.size(); i += 2)
        table->put(elements[i], elements[i + 1]);
    return table;
}

static char as_char(ObjectRef<Object> obj)
{
    aassert(Alma::alma.typep(obj, Alma::alma.intern_alma_symbol("character")),
        "Expected a character but found " << Alma::alma.to_string(obj));
    return obj.as<Character>()->get_char();
}

ObjectRef<Object> Set_character_macro::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    check_arity(arg_list, 2, 3);
    char c = as_char(arg_list[0]);
    aassert(Alma::alma.typep(arg_list[1], Alma::alma.intern_alma_symbol("procedure")),
        "Expected a procedure but found " << Alma::alma.to_string(arg_list[1]));
    optional_package(arg_list, 2)->set_character_macro(c, arg_list[1]);
    return arg_list[1];
}

ObjectRef<Object> Find_character_macro::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    check_arity(arg_list, 1, 2);
    std::optional<ObjectRef<Procedure>> proc = optional_package(arg_list, 1)->find_character_macro(as_char(arg_list[0]));
    if (!proc)
        return Alma::alma.intern_alma_symbol("nil");
    return *proc;
}

ObjectRef<Object> Export_character_macro::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    check_arity(arg_list, 1, 2);
    ObjectRef<Package> package = optional_package(arg_list, 1);
    for (ObjectRef<Object> c : designator_list(arg_list[0]))
        package->export_character_macro(as_char(c));
    return Alma::alma.boolean(true);
}

ObjectRef<Object> Unexport_character_macro::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    check_arity(arg_list, 1, 2);
    ObjectRef<Package> package = optional_package(arg_list, 1);
    for (ObjectRef<Object> c : designator_list(arg_list[0]))
        package->unexport_character_macro(as_char(c));
    return Alma::alma.boolean(true);
}

ObjectRef<Object> Import_character_macro::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    check_arity(arg_list, 2, 3);
    ObjectRef<Package> from = package_designator(arg_list[1]);
    ObjectRef<Package> package = optional_package(arg_list, 2);
    for (ObjectRef<Object> obj : designator_list(arg_list[0])) {
        char c = as_char(obj);
        std::optional<ObjectRef<Procedure>> proc = from->find_character_macro(c);
        aassert(proc, "The character macro #" << c << " is not defined in the package " << from->get_name());
        package->import_character_macro(c, *proc);
    }
    return Alma::alma.boolean(true);
}

// --------------------------------------------------------------------------------

ObjectRef<Object> Load::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "string" });
    std::filesystem::path path = arg_list[0].as<String>()->get_content();
    if (path.is_relative()) {
        ObjectRef<Symbol> current_file_sym = Alma::alma.intern_alma_symbol("*current-file*");
        ObjectRef<Object> current_file = current_file_sym->get_value();
        aassert(Alma::alma.typep(current_file, Alma::alma.intern_alma_symbol("string")),
            "load cannot resolve the relative path " << path.native() << " because *current-file* is not a string");
        path = std::filesystem::path(current_file.as<String>()->get_content()).parent_path() / path;
    }
    Alma::alma.load(path);
    return Alma::alma.boolean(true);
}

// --------------------------------------------------------------------------------

static std::filesystem::path import_cache_directory()
{
    const char* xdg_cache = std::getenv("XDG_CACHE_HOME");
    if (xdg_cache && *xdg_cache)
        return std::filesystem::path(xdg_cache) / "alma";
    const char* home = std::getenv("HOME");
    aassert(home && *home, "import cannot find the cache directory because HOME is not defined");
    return std::filesystem::path(home) / ".cache" / "alma";
}

static std::string import_sanitize(const std::string& text)
{
    std::string sanitized;
    for (char c : text)
        sanitized += std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' ? c : '_';
    return sanitized;
}

static std::filesystem::path import_clone(const std::string& url, const std::optional<std::string>& ref)
{
    std::string dir_name = import_sanitize(url);
    if (ref)
        dir_name += "__" + import_sanitize(*ref);
    std::filesystem::path cache = import_cache_directory();
    std::filesystem::path dir = cache / dir_name;
    if (std::filesystem::exists(dir))
        return dir;
    std::filesystem::create_directories(cache);
    std::vector<std::string> args = { "git", "-c", "advice.detachedHead=false", "clone", "--quiet", "--depth", "1" };
    if (ref) {
        args.push_back("--branch");
        args.push_back(*ref);
    }
    args.push_back("--");
    args.push_back(url);
    args.push_back(dir.native());
    std::vector<char*> argv;
    for (std::string& arg : args)
        argv.push_back(arg.data());
    argv.push_back(nullptr);
    pid_t pid;
    int error = posix_spawnp(&pid, "git", nullptr, nullptr, argv.data(), environ);
    aassert(error == 0, "import cannot run git: " << std::strerror(error));
    int status;
    aassert(waitpid(pid, &status, 0) == pid, "import cannot wait for git");
    aassert(WIFEXITED(status) && WEXITSTATUS(status) == 0,
        "import cannot clone " << url << (ref ? " at " + *ref : std::string()));
    return dir;
}

static std::pair<std::string, std::optional<std::string>> import_find(const std::string& distribution_url,
    const std::optional<std::string>& distribution_ref, const std::string& name, const std::string& kind)
{
    std::filesystem::path index = import_clone(distribution_url, distribution_ref) / "projects.alma";
    std::ifstream input(index);
    aassert(input.is_open(), "Cannot open the file " << index.native());
    Reader reader(Alma::alma, index.native(), input);
    ObjectRef<Object> string_type = Alma::alma.intern_alma_symbol("string");
    for (std::optional<ObjectRef<Object>> obj = reader.read(); obj; obj = reader.read()) {
        aassert(Alma::alma.consp(*obj), "Invalid entry " << Alma::alma.to_string(*obj) << " in " << index.native());
        auto [entry, entry_rest] = obj->as<Cons>()->to_list();
        aassert(!entry_rest && entry.size() == 4
                && Alma::alma.typep(entry[0], string_type)
                && Alma::alma.symbolp(entry[1])
                && Alma::alma.typep(entry[2], string_type)
                && Alma::alma.typep(entry[3], string_type),
            "Invalid entry " << Alma::alma.to_string(*obj) << " in " << index.native()
                             << ", expected (name project-or-distribution url branch-or-tag)");
        if (entry[0].as<String>()->get_content() != name)
            continue;
        aassert(entry[1] == Alma::alma.intern_symbol(kind),
            name << " in " << distribution_url << " is not a " << kind);
        return { entry[2].as<String>()->get_content(), entry[3].as<String>()->get_content() };
    }
    athrow("Cannot find " << name << " in " << distribution_url);
}

ObjectRef<Object> Import::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "string" });
    const std::string& project = arg_list[0].as<String>()->get_content();
    std::vector<std::string> names;
    size_t start = 0;
    while (true) {
        size_t end = project.find('/', start);
        names.push_back(project.substr(start, end == std::string::npos ? std::string::npos : end - start));
        aassert(!names.back().empty(), "Invalid project name " << project);
        if (end == std::string::npos)
            break;
        start = end + 1;
    }
    ObjectRef<Symbol> source_sym = Alma::alma.intern_alma_symbol("*source-distribution*");
    ObjectRef<Object> source = source_sym->get_value();
    aassert(Alma::alma.typep(source, Alma::alma.intern_alma_symbol("string")),
        "import cannot find " << project << " because *source-distribution* is not a string");
    std::pair<std::string, std::optional<std::string>> location = { source.as<String>()->get_content(), std::nullopt };
    for (size_t i = 0; i + 1 < names.size(); ++i)
        location = import_find(location.first, location.second, names[i], "distribution");
    location = import_find(location.first, location.second, names.back(), "project");
    Alma::alma.load(import_clone(location.first, location.second) / (names.back() + ".alma"));
    return Alma::alma.boolean(true);
}

// --------------------------------------------------------------------------------

ObjectRef<Object> Exit::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "t" }, "string");
    aassert(arg_list.size() <= 2, "exit expects a boolean and optionally the name of the executable");
    std::optional<std::string> name;
    if (arg_list.size() == 2)
        name = arg_list[1].as<String>()->get_content();
    throw ExitRequest { Alma::alma.truep(arg_list[0]), name };
}

// ================================================================================
// Alma: vectors
// ================================================================================

static ObjectRef<Vector> as_vector(const ObjectRef<Object>& obj)
{
    return ObjectRef<Vector>(obj);
}

static size_t as_index(const ObjectRef<Object>& obj)
{
    int64_t index = obj.as<Integer>()->get_value();
    aassert(index >= 0, "The index of a vector cannot be negative: " << index);
    return static_cast<size_t>(index);
}

ObjectRef<Object> Vector_create::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list, ObjectRef<Environment> environment [[maybe_unused]])
{
    return Alma::alma.make<Vector>(arg_list);
}

ObjectRef<Object> Make_vector::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list, ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "integer" }, "t");
    aassert(arg_list.size() <= 2, "make-vector expects a size and optionally an initial value");
    ObjectRef<Object> initial = arg_list.size() == 2 ? arg_list[1] : Alma::alma.intern_alma_symbol("nil");
    return Alma::alma.make<Vector>(std::vector<ObjectRef<Object>>(as_index(arg_list[0]), initial));
}

ObjectRef<Object> Vectorp::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list, ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "t" });
    return Alma::alma.boolean(Alma::alma.typep(arg_list[0], Alma::alma.intern_alma_symbol("vector")));
}

ObjectRef<Object> Vector_length::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list, ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "vector" });
    return Alma::alma.make<Integer>(static_cast<int64_t>(as_vector(arg_list[0])->size()));
}

ObjectRef<Object> Vector_empty_p::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list, ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "vector" });
    return Alma::alma.boolean(as_vector(arg_list[0])->size() == 0);
}

ObjectRef<Object> Vector_set::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list, ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "vector", "integer", "t" });
    as_vector(arg_list[0])->set(as_index(arg_list[1]), arg_list[2]);
    return arg_list[0];
}

ObjectRef<Object> Vector_front::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list, ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "vector" });
    return as_vector(arg_list[0])->front();
}

ObjectRef<Object> Vector_back::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list, ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "vector" });
    return as_vector(arg_list[0])->back();
}

ObjectRef<Object> Vector_push_front::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list, ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "vector", "t" });
    as_vector(arg_list[0])->push_front(arg_list[1]);
    return arg_list[0];
}

ObjectRef<Object> Vector_push_back::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list, ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "vector", "t" });
    as_vector(arg_list[0])->push_back(arg_list[1]);
    return arg_list[0];
}

ObjectRef<Object> Vector_pop_front::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list, ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "vector" });
    return as_vector(arg_list[0])->pop_front();
}

ObjectRef<Object> Vector_pop_back::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list, ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "vector" });
    return as_vector(arg_list[0])->pop_back();
}

ObjectRef<Object> Vector_insert::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list, ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "vector", "integer", "t" });
    as_vector(arg_list[0])->insert(as_index(arg_list[1]), arg_list[2]);
    return arg_list[0];
}

ObjectRef<Object> Vector_erase::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list, ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "vector", "integer" });
    return as_vector(arg_list[0])->erase(as_index(arg_list[1]));
}

ObjectRef<Object> Vector_clear::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list, ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "vector" });
    as_vector(arg_list[0])->clear();
    return arg_list[0];
}

ObjectRef<Object> Vector_to_list::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list, ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "vector" });
    std::vector<ObjectRef<Object>> elements = as_vector(arg_list[0])->to_vector();
    if (elements.empty())
        return Alma::alma.intern_alma_symbol("nil");
    return Alma::alma.make<Cons>(elements);
}

ObjectRef<Object> List_to_vector::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list, ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "t" });
    if (Alma::alma.null(arg_list[0]))
        return Alma::alma.make<Vector>(std::vector<ObjectRef<Object>> {});
    aassert(Alma::alma.consp(arg_list[0]), "Expected a list but found " << Alma::alma.to_string(arg_list[0]));
    auto [list, tail] = arg_list[0].as<Cons>()->to_list();
    aassert(Alma::alma.null(tail), "Expected a proper list but found " << Alma::alma.to_string(arg_list[0]));
    return Alma::alma.make<Vector>(list);
}

// ================================================================================
// C: emission
// ================================================================================

ObjectRef<Object> Emit::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    for (const ObjectRef<Object>& arg : arg_list)
        Alma::alma.emit(arg);
    return Alma::alma.boolean(false);
}

// ================================================================================
// C: introspection
// ================================================================================

// --------------------------------------------------------------------------------
// C: introspection helpers

using Arguments = std::vector<ObjectRef<Object>>;

static ObjectRef<Object> symbol(const std::string& name)
{
    return Alma::alma.intern_symbol(name);
}

static ObjectRef<Object> boolean(bool value)
{
    return Alma::alma.boolean(value);
}

static ObjectRef<Object> nil()
{
    return Alma::alma.intern_alma_symbol("nil");
}

static ObjectRef<Object> make_list(const std::vector<ObjectRef<Object>>& elements)
{
    if (elements.empty())
        return nil();
    return Alma::alma.make<Cons>(elements);
}

static std::string name_of(const ObjectRef<Object>& obj)
{
    return obj.as<Symbol>()->get_name();
}

static const Declaration* find_declaration(const std::string& name)
{
    Declarations& declarations = Alma::alma.get_declarations();
    return declarations.contains(name) ? &declarations.get(name) : nullptr;
}

static const Record* find_record(const std::string& name)
{
    const Records& records = Alma::alma.get_declarations().get_records();
    return records.contains(name) ? &records.get(name) : nullptr;
}

static const FunctionType* find_function(const std::string& name)
{
    const Declaration* declaration = find_declaration(name);
    if (!declaration || !declaration->getType()->is_function())
        return nullptr;
    return static_cast<const FunctionType*>(declaration->getType());
}

// --------------------------------------------------------------------------------
// C: introspection of kinds of declarations

ObjectRef<Object> C_declared_p::eval_body(const Arguments& arg_list, ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "symbol" });
    return boolean(find_declaration(name_of(arg_list[0])) != nullptr);
}

ObjectRef<Object> C_defined_p::eval_body(const Arguments& arg_list, ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "symbol" });
    const Declaration* declaration = find_declaration(name_of(arg_list[0]));
    return boolean(declaration && declaration->isDefined());
}

ObjectRef<Object> C_function_p::eval_body(const Arguments& arg_list, ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "symbol" });
    return boolean(find_function(name_of(arg_list[0])) != nullptr);
}

ObjectRef<Object> C_variable_p::eval_body(const Arguments& arg_list, ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "symbol" });
    const Declaration* declaration = find_declaration(name_of(arg_list[0]));
    return boolean(declaration && !declaration->getType()->is_function() && !declaration->isConstant());
}

ObjectRef<Object> C_constant_p::eval_body(const Arguments& arg_list, ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "symbol" });
    const Declaration* declaration = find_declaration(name_of(arg_list[0]));
    return boolean(declaration && declaration->isConstant());
}

ObjectRef<Object> C_standard_p::eval_body(const Arguments& arg_list, ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "symbol" });
    const Declaration* declaration = find_declaration(name_of(arg_list[0]));
    return boolean(declaration && declaration->getHeader());
}

ObjectRef<Object> C_struct_p::eval_body(const Arguments& arg_list, ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "symbol" });
    const Record* record = find_record(name_of(arg_list[0]));
    return boolean(record && !record->isUnion());
}

ObjectRef<Object> C_union_p::eval_body(const Arguments& arg_list, ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "symbol" });
    const Record* record = find_record(name_of(arg_list[0]));
    return boolean(record && record->isUnion());
}

ObjectRef<Object> C_macro_p::eval_body(const Arguments& arg_list, ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "symbol" });
    ObjectRef<Symbol> name(arg_list[0]);
    return boolean(name->has_property(Alma::alma.intern_alma_symbol("macro")));
}

// --------------------------------------------------------------------------------
// C: introspection of functions and variables

ObjectRef<Object> C_type::eval_body(const Arguments& arg_list, ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "symbol" });
    const Declaration* declaration = find_declaration(name_of(arg_list[0]));
    return declaration ? declaration->getType()->to_object() : nil();
}

ObjectRef<Object> C_header::eval_body(const Arguments& arg_list, ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "symbol" });
    const Declaration* declaration = find_declaration(name_of(arg_list[0]));
    if (!declaration || !declaration->getHeader())
        return nil();
    return Alma::alma.make<String>(*declaration->getHeader());
}

ObjectRef<Object> C_function_result::eval_body(const Arguments& arg_list, ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "symbol" });
    const FunctionType* function = find_function(name_of(arg_list[0]));
    return function ? function->get_result().to_object() : nil();
}

ObjectRef<Object> C_function_parameters::eval_body(const Arguments& arg_list, ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "symbol" });
    const FunctionType* function = find_function(name_of(arg_list[0]));
    if (!function)
        return nil();
    std::vector<ObjectRef<Object>> parameters;
    for (const std::unique_ptr<Type>& parameter : function->get_arguments())
        parameters.push_back(parameter->to_object());
    return make_list(parameters);
}

ObjectRef<Object> C_function_variadic_p::eval_body(const Arguments& arg_list, ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "symbol" });
    const FunctionType* function = find_function(name_of(arg_list[0]));
    return boolean(function && function->is_variadic());
}

// --------------------------------------------------------------------------------
// C: introspection of structs and unions

ObjectRef<Object> C_record_defined_p::eval_body(const Arguments& arg_list, ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "symbol" });
    const Record* record = find_record(name_of(arg_list[0]));
    return boolean(record && record->isDefined());
}

ObjectRef<Object> C_record_members::eval_body(const Arguments& arg_list, ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "symbol" });
    const Record* record = find_record(name_of(arg_list[0]));
    if (!record)
        return nil();
    std::vector<ObjectRef<Object>> members;
    for (const auto& [name, type] : record->getMembers())
        members.push_back(make_list({ symbol(name), type->to_object() }));
    return make_list(members);
}

ObjectRef<Object> C_member_type::eval_body(const Arguments& arg_list, ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "symbol", "symbol" });
    const Record* record = find_record(name_of(arg_list[0]));
    if (!record)
        return nil();
    std::string member = name_of(arg_list[1]);
    for (const auto& [name, type] : record->getMembers())
        if (name == member)
            return type->to_object();
    return nil();
}

// --------------------------------------------------------------------------------
// C: introspection of enums

ObjectRef<Object> C_enum_p::eval_body(const Arguments& arg_list, ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "symbol" });
    return boolean(Alma::alma.get_declarations().is_enum(name_of(arg_list[0])));
}

ObjectRef<Object> C_enum_constants::eval_body(const Arguments& arg_list, ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "symbol" });
    Declarations& declarations = Alma::alma.get_declarations();
    std::string name = name_of(arg_list[0]);
    if (!declarations.is_enum(name))
        return nil();
    std::vector<ObjectRef<Object>> constants;
    for (const std::string& constant : declarations.get_enum_constants(name))
        constants.push_back(symbol(constant));
    return make_list(constants);
}

// --------------------------------------------------------------------------------
// C: introspection of types, written with the syntax of alma

ObjectRef<Object> C_type_string::eval_body(const Arguments& arg_list, ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "t" });
    return Alma::alma.make<String>(Type::parse_object(arg_list[0])->to_string(""));
}

ObjectRef<Object> C_pointer_p::eval_body(const Arguments& arg_list, ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "t" });
    return boolean(Type::parse_object(arg_list[0])->is_pointer());
}

ObjectRef<Object> C_array_p::eval_body(const Arguments& arg_list, ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "t" });
    return boolean(Type::parse_object(arg_list[0])->is_array());
}

ObjectRef<Object> C_const_p::eval_body(const Arguments& arg_list, ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "t" });
    return boolean(is_const(*Type::parse_object(arg_list[0])));
}

// --------------------------------------------------------------------------------
// C: introspection with lists of names. They do not include the standard library.

static ObjectRef<Object> declaration_names(bool functions)
{
    Declarations& declarations = Alma::alma.get_declarations();
    std::vector<ObjectRef<Object>> names;
    for (const std::string& name : declarations.names()) {
        const Declaration& declaration = declarations.get(name);
        if (!declaration.getHeader() && declaration.getType()->is_function() == functions)
            names.push_back(symbol(name));
    }
    return make_list(names);
}

ObjectRef<Object> C_functions::eval_body(const Arguments& arg_list, ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, {});
    return declaration_names(true);
}

ObjectRef<Object> C_variables::eval_body(const Arguments& arg_list, ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, {});
    return declaration_names(false);
}

ObjectRef<Object> C_records::eval_body(const Arguments& arg_list, ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, {});
    std::vector<ObjectRef<Object>> names;
    for (const std::string& name : Alma::alma.get_declarations().get_records().names())
        names.push_back(symbol(name));
    return make_list(names);
}

ObjectRef<Object> C_enums::eval_body(const Arguments& arg_list, ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, {});
    std::vector<ObjectRef<Object>> names;
    for (const std::string& name : Alma::alma.get_declarations().enum_names())
        names.push_back(symbol(name));
    return make_list(names);
}
