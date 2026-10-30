
#pragma once

#include "procedure.hpp"

// void intern_functions();

#define declare_function(name)                              \
    class name : public Function {                          \
    public:                                                 \
        name()                                              \
            : Function()                                    \
        {                                                   \
        }                                                   \
                                                            \
    protected:                                              \
        virtual ObjectRef<Object> eval_body(                \
            const std::vector<ObjectRef<Object>>& arg_list, \
            ObjectRef<Environment> enviroment) override;    \
    };

// -----------------------------------------------------------------------------
// Alma

declare_function(Sum);
declare_function(Print);
// declare_function(Type);
declare_function(Set_symbol_value);
declare_function(Setq);
declare_function(Car);
declare_function(Cdr);
declare_function(Eq);
declare_function(Eval);
declare_function(List);
declare_function(Load);
declare_function(Import);
declare_function(Exit);

struct ExitRequest {
    bool generate;
    std::optional<std::string> name;
};

// -----------------------------------------------------------------------------
// Alma: vectors
//
//   (vector 1 2 3)                 -> [1 2 3]
//   (make-vector 3 0)              -> [0 0 0]
//   (v 0)                          -> the element 0, applying the vector to the index
//   (vector-set v 0 x)
//   (vector-push-back v x) (vector-pop-front v) ...
//
// The functions that modify a vector return the vector, except the ones that remove an element,
// that return the element.

declare_function(Vector_create);
declare_function(Make_vector);
declare_function(Vectorp);
declare_function(Vector_length);
declare_function(Vector_empty_p);
declare_function(Vector_set);
declare_function(Vector_front);
declare_function(Vector_back);
declare_function(Vector_push_front);
declare_function(Vector_push_back);
declare_function(Vector_pop_front);
declare_function(Vector_pop_back);
declare_function(Vector_insert);
declare_function(Vector_erase);
declare_function(Vector_clear);
declare_function(Vector_to_list);
declare_function(List_to_vector);

// -----------------------------------------------------------------------------
// C: emission

declare_function(Emit);

// -----------------------------------------------------------------------------
// C: introspection
//
// They inspect what emit knows: the functions, variables, structs, unions and macros that have
// been declared, including the C standard library. They receive names as symbols and return types
// with the syntax of alma, so they can be used to generate code:
//
//   (c-type 'printf)                -> (int ((const char *) &rest))
//   (c-record-members 'Point)       -> ((x int) (y int))
//
// Only the top level declarations of the forms already emitted are visible. Macros are expanded
// before emitting their form, so they cannot see the local variables of a function or the form
// that is being emitted.

// Kinds of declarations
declare_function(C_declared_p);
declare_function(C_defined_p);
declare_function(C_function_p);
declare_function(C_variable_p);
declare_function(C_constant_p);
declare_function(C_standard_p);
declare_function(C_struct_p);
declare_function(C_union_p);
declare_function(C_macro_p);

// Functions and variables
declare_function(C_type);
declare_function(C_header);
declare_function(C_function_result);
declare_function(C_function_parameters);
declare_function(C_function_variadic_p);

// Structs and unions
declare_function(C_record_defined_p);
declare_function(C_record_members);
declare_function(C_member_type);

// Enums
declare_function(C_enum_p);
declare_function(C_enum_constants);

// Types
declare_function(C_type_string);
declare_function(C_pointer_p);
declare_function(C_array_p);
declare_function(C_const_p);

// Lists of names
declare_function(C_functions);
declare_function(C_variables);
declare_function(C_records);
declare_function(C_enums);
