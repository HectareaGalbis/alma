
#include "emitter.hpp"
#include "../alma.hpp"
#include "../debug.hpp"
#include "../object/cons.hpp"
#include "../object/environment.hpp"
#include "../object/string.hpp"
#include "../object/symbol.hpp"

static std::string type_to_string(const Type& type)
{
    return type.to_string("");
}

static std::string symbol_name(const ObjectRef<Object>& obj, const std::string& what)
{
    aassert(Alma::alma.symbolp(obj), "Expected a symbol as " << what << " but found " << Alma::alma.to_string(obj));
    return obj.as<Symbol>()->get_name();
}

static std::vector<ObjectRef<Object>> proper_list(const ObjectRef<Object>& obj, const std::string& what)
{
    if (Alma::alma.null(obj))
        return {};
    aassert(Alma::alma.consp(obj), "Expected a list as " << what << " but found " << Alma::alma.to_string(obj));
    auto [list, tail] = obj.as<Cons>()->to_list();
    aassert(Alma::alma.null(tail), "Expected a proper list as " << what << " but found " << Alma::alma.to_string(obj));
    return list;
}

// -----------------------------------------------------------------------------

Emitter::Emitter(const std::string& _name)
    : name(_name)
{
}

const std::string& Emitter::get_name() const
{
    return this->name;
}

void Emitter::emit_top_level(ObjectRef<Object> self [[maybe_unused]],
    const std::vector<ObjectRef<Object>>& arg_list [[maybe_unused]],
    ObjectRef<Environment> environment [[maybe_unused]],
    Declarations& declarations [[maybe_unused]],
    std::stringstream& output [[maybe_unused]])
{
    athrow("'" << this->name << "' cannot be used at the top level");
}

void Emitter::emit_statement(ObjectRef<Object> self, const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment, Declarations& declarations, std::stringstream& output)
{
    this->emit_expression(self, arg_list, environment, declarations, output);
    output << ";\n";
}

std::unique_ptr<Type> Emitter::emit_expression(ObjectRef<Object> self [[maybe_unused]],
    const std::vector<ObjectRef<Object>>& arg_list [[maybe_unused]],
    ObjectRef<Environment> environment [[maybe_unused]],
    Declarations& declarations [[maybe_unused]],
    std::stringstream& output [[maybe_unused]])
{
    athrow("'" << this->name << "' cannot be used as an expression");
}

std::string Emitter::to_string(ObjectRef<Object> self [[maybe_unused]])
{
    return "<emitter " + this->name + ">";
}

ObjectRef<Object> Emitter::type(ObjectRef<Object> self [[maybe_unused]]) const
{
    return Alma::alma.intern_alma_symbol("emitter");
}

// -----------------------------------------------------------------------------

EmitterInclude::EmitterInclude()
    : Emitter("include")
{
}

void EmitterInclude::emit_top_level(ObjectRef<Object> self [[maybe_unused]],
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]],
    Declarations& declarations [[maybe_unused]],
    std::stringstream& output)
{
    aassert(arg_list.size() == 1, "include expects one argument but received " << arg_list.size());
    const String* header = dynamic_cast<const String*>(arg_list[0].get());
    aassert(header, "include expects a string but found " << Alma::alma.to_string(arg_list[0]));
    output << "#include <" << header->get_content() << ">\n";
}

// -----------------------------------------------------------------------------

EmitterDeclare::EmitterDeclare()
    : Emitter("declare")
{
}

void EmitterDeclare::emit_top_level(ObjectRef<Object> self [[maybe_unused]],
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]],
    Declarations& declarations,
    std::stringstream& output [[maybe_unused]])
{
    aassert(arg_list.size() == 2, "declare expects a name and a type but received " << arg_list.size() << " arguments");
    std::string declaration_name = symbol_name(arg_list[0], "the name of a declaration");
    std::unique_ptr<Type> type = Type::parse_object(arg_list[1]);
    declarations.add(Declaration(declaration_name, type));
}

// -----------------------------------------------------------------------------

EmitterFunction::EmitterFunction()
    : Emitter("function")
{
}

void EmitterFunction::emit_top_level(ObjectRef<Object> self [[maybe_unused]],
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment,
    Declarations& declarations,
    std::stringstream& output)
{
    aassert(arg_list.size() >= 3,
        "function expects a name, a result type and a list of parameters but received "
            << arg_list.size() << " arguments");

    std::string function_name = symbol_name(arg_list[0], "the name of a function");

    std::unique_ptr<Type> result = Type::parse_object(arg_list[1]);
    aassert(!result->is_function(), "The function " << function_name << " cannot return a function");
    aassert(!result->is_array(), "The function " << function_name << " cannot return an array");

    std::vector<std::string> parameter_names;
    std::vector<std::unique_ptr<Type>> parameter_types;
    for (const ObjectRef<Object>& parameter : proper_list(arg_list[2], "the parameters of " + function_name)) {
        std::vector<ObjectRef<Object>> pair = proper_list(parameter, "a parameter of " + function_name);
        aassert(pair.size() == 2,
            "Expected (name type) as parameter of " << function_name << " but found " << Alma::alma.to_string(parameter));
        std::string parameter_name = symbol_name(pair[0], "the name of a parameter of " + function_name);
        std::unique_ptr<Type> parameter_type = Type::parse_object(pair[1]);
        aassert(!parameter_type->is_function(),
            "The parameter " << parameter_name << " of " << function_name << " cannot be a function");
        parameter_names.push_back(parameter_name);
        parameter_types.push_back(std::move(parameter_type));
    }

    // The function is declared before checking its body to allow recursive calls
    std::unique_ptr<Type> function_type = std::make_unique<FunctionType>(
        result, parameter_types, false, std::vector<Specifier> {});
    Declaration function(function_name, function_type, true);
    declarations.add(function);

    Declarations scope = declarations.function_scope(function);
    for (size_t i = 0; i < parameter_names.size(); i++)
        scope.add_local(Declaration(parameter_names[i], parameter_types[i]));

    output << static_cast<const FunctionType&>(*function_type).to_string(function_name, parameter_names) << "\n{\n";
    for (size_t i = 3; i < arg_list.size(); i++) {
        output << "    ";
        Alma::alma.emit_statement(arg_list[i], environment, scope, output);
    }
    output << "}\n";
}

// -----------------------------------------------------------------------------

EmitterReturn::EmitterReturn()
    : Emitter("return")
{
}

void EmitterReturn::emit_statement(ObjectRef<Object> self [[maybe_unused]],
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment,
    Declarations& declarations,
    std::stringstream& output)
{
    const Declaration* function = declarations.get_function();
    aassert(function, "return can only be used inside a function");
    aassert(arg_list.size() <= 1, "return expects at most one argument but received " << arg_list.size());

    const Type& result = static_cast<const FunctionType*>(function->getType())->get_result();
    const ValueType* value_result = dynamic_cast<const ValueType*>(&result);
    bool returns_void = value_result && value_result->is_void();

    if (arg_list.empty()) {
        aassert(returns_void, "The function " << function->getName() << " must return a value of type "
                                              << type_to_string(result));
        output << "return;\n";
        return;
    }

    aassert(!returns_void, "The function " << function->getName() << " cannot return a value");
    output << "return ";
    std::unique_ptr<Type> type = Alma::alma.emit_expression(arg_list[0], environment, declarations, output);
    aassert(is_assignable(result, *type),
        "The function " << function->getName() << " must return a value of type " << type_to_string(result)
                        << " but " << Alma::alma.to_string(arg_list[0]) << " is of type " << type_to_string(*type));
    output << ";\n";
}

// -----------------------------------------------------------------------------

template <typename T>
static void intern_emitter(const std::string& name)
{
    ObjectRef<Symbol> sym = Alma::alma.intern_alma_symbol(name);
    sym->push_emitter(Alma::alma.make<T>());
}

void intern_emitters()
{
    intern_emitter<EmitterInclude>("include");
    intern_emitter<EmitterDeclare>("declare");
    intern_emitter<EmitterFunction>("function");
    intern_emitter<EmitterReturn>("return");
}
