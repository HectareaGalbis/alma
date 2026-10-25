
#include "function.hpp"
#include "alma.hpp"
#include "cons.hpp"
#include "debug.hpp"
#include "integer.hpp"
#include "object.hpp"
#include "package.hpp"
#include "string.hpp"
#include "symbol.hpp"
#include "vector.hpp"
#include "interpiler/declaration.hpp"
#include <filesystem>
#include <iostream>

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
    this->check_types(arg_list, { "cons" });
    return Alma::alma.car(arg_list[0]);
}

// --------------------------------------------------------------------------------

ObjectRef<Object> Cdr::eval_body(
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]])
{
    this->check_types(arg_list, { "cons" });
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
    return Alma::alma.intern_alma_symbol(name);
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
