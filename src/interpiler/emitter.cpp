
#include "emitter.hpp"
#include "../alma.hpp"
#include "../debug.hpp"
#include "../object/cons.hpp"
#include "../object/environment.hpp"
#include "../object/symbol.hpp"
#include <unordered_set>

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

/** Declaring again a function or variable of the standard library with the same type only checks it */
static bool is_standard_redeclaration(const Declarations& declarations, const Declaration& declaration)
{
    const std::string& name = declaration.getName();
    return !declaration.isDefined() && declarations.contains(name) && declarations.get(name).getHeader()
        && declarations.get(name) == declaration;
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
    bool variadic = false;
    std::vector<ObjectRef<Object>> parameters = proper_list(arg_list[2], "the parameters of " + function_name);
    for (size_t i = 0; i < parameters.size(); i++) {
        const ObjectRef<Object>& parameter = parameters[i];
        if (Alma::alma.symbolp(parameter) && parameter.as<Symbol>()->get_name() == "&rest") {
            aassert(i == parameters.size() - 1, "&rest must be the last parameter of " << function_name);
            aassert(i > 0, "The variadic function " << function_name << " needs at least one parameter");
            variadic = true;
            break;
        }
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
    bool has_body = arg_list.size() > 3;
    std::unique_ptr<Type> function_type = std::make_unique<FunctionType>(
        result, parameter_types, variadic, std::vector<Specifier> {});
    const FunctionType& type = static_cast<const FunctionType&>(*function_type);
    Declaration function(function_name, function_type, has_body);
    bool standard = is_standard_redeclaration(declarations, function);
    declarations.add(function);
    if (standard)
        return;
    declarations.use_type(*function_type);

    if (!has_body) {
        output << type.to_string(function_name, parameter_names) << ";\n";
        return;
    }

    Declarations scope = declarations.function_scope(function);
    for (size_t i = 0; i < parameter_names.size(); i++)
        scope.add_local(Declaration(parameter_names[i], parameter_types[i]));

    output << type.to_string(function_name, parameter_names) << "\n{\n";
    bool empty_body = arg_list.size() == 4 && Alma::alma.null(arg_list[3]);
    for (size_t i = 3; i < arg_list.size() && !empty_body; i++) {
        output << "    ";
        Alma::alma.emit_statement(arg_list[i], environment, scope, output);
    }
    output << "}\n";
}

// -----------------------------------------------------------------------------

EmitterVariable::EmitterVariable()
    : Emitter("variable")
{
}

/** Parse (name type [value]) of a variable */
static std::pair<std::string, std::unique_ptr<Type>> parse_variable(const std::vector<ObjectRef<Object>>& arg_list)
{
    aassert(arg_list.size() == 2 || arg_list.size() == 3,
        "variable expects a name, a type and optionally a value but received " << arg_list.size() << " arguments");
    std::string variable_name = symbol_name(arg_list[0], "the name of a variable");
    std::unique_ptr<Type> type = Type::parse_object(arg_list[1]);
    aassert(!type->is_function(), "The variable " << variable_name << " cannot be a function");
    aassert(!is_void(*type), "The variable " << variable_name << " cannot be void");
    return { variable_name, std::move(type) };
}

/** Emit the initial value of a variable checking its type */
static void emit_initial_value(const std::string& variable_name, const Type& type, const ObjectRef<Object>& value,
    ObjectRef<Environment> environment, Declarations& declarations, std::stringstream& output)
{
    std::unique_ptr<Type> value_type = Alma::alma.emit_expression(value, environment, declarations, output);
    aassert(!type.is_array() && is_assignable(type, *value_type),
        "Cannot initialize the variable " << variable_name << " of type " << type_to_string(type)
                                          << " with " << Alma::alma.to_string(value) << " of type "
                                          << type_to_string(*value_type));
}

static bool is_operator(const std::string& name)
{
    static const std::unordered_set<std::string> operators = {
        "+", "-", "*", "/", "%", "bit-and", "bit-or", "bit-xor", "bit-not", "<<", ">>",
        "==", "!=", "<", ">", "<=", ">=", "and", "or", "not", "?", "cast"
    };
    return operators.contains(name);
}

/** Check if an object can initialize a global variable: arithmetic constants, string literals,
    addresses of global objects and functions, and operations with them.
    Global variables are not constants, except arrays and functions that are used as addresses. */
static bool is_constant(const ObjectRef<Object>& obj, const Declarations& declarations)
{
    Alma& alma = Alma::alma;
    if (alma.symbolp(obj)) {
        const std::string& name = obj.as<Symbol>()->get_name();
        if (!declarations.contains(name))
            return false;
        const Declaration& declaration = declarations.get(name);
        return declaration.isConstant() || declaration.getType()->is_function() || declaration.getType()->is_array();
    }
    if (!alma.consp(obj))
        return true;

    auto [list, tail] = obj.as<Cons>()->to_list();
    if (!alma.null(tail) || !alma.symbolp(list[0]))
        return false;
    ObjectRef<Symbol> symbol(list[0]);
    const std::string& name = symbol->get_name();
    if (!symbol->has_emitter())
        return false;

    if (name == "sizeof")
        return true;
    if (name == "&" && list.size() == 2) {
        if (alma.symbolp(list[1]))
            return declarations.contains(list[1].as<Symbol>()->get_name());
        return is_constant(list[1], declarations);
    }
    if (name == "aref" && list.size() == 3)
        return is_constant(list[1], declarations) && is_constant(list[2], declarations);
    if (name == "*" && list.size() == 2)
        return false;
    if (!is_operator(name))
        return false;
    // The type of a cast is not an expression
    for (size_t i = name == "cast" ? 2 : 1; i < list.size(); i++)
        if (!is_constant(list[i], declarations))
            return false;
    return true;
}

void EmitterVariable::emit_top_level(ObjectRef<Object> self [[maybe_unused]],
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment,
    Declarations& declarations,
    std::stringstream& output)
{
    auto [variable_name, type] = parse_variable(arg_list);

    bool has_value = arg_list.size() == 3;
    Declaration variable(variable_name, type, has_value);
    bool standard = is_standard_redeclaration(declarations, variable);
    declarations.add(variable);
    if (standard)
        return;
    declarations.use_type(*type);

    if (!has_value) {
        output << "extern " << type->to_string(variable_name) << ";\n";
        return;
    }

    aassert(is_constant(arg_list[2], declarations),
        "The global variable " << variable_name << " must be initialized with a constant but found "
                               << Alma::alma.to_string(arg_list[2]));
    output << type->to_string(variable_name) << " = ";
    emit_initial_value(variable_name, *type, arg_list[2], environment, declarations, output);
    output << ";\n";
}

void EmitterVariable::emit_statement(ObjectRef<Object> self [[maybe_unused]],
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment,
    Declarations& declarations,
    std::stringstream& output)
{
    aassert(declarations.get_function(), "A local variable can only be defined inside a function");
    auto [variable_name, type] = parse_variable(arg_list);

    // As in C, the variable is visible in its own initial value
    declarations.add_local(Declaration(variable_name, type, true));
    declarations.use_type(*type);

    output << type->to_string(variable_name);
    if (arg_list.size() == 3) {
        output << " = ";
        emit_initial_value(variable_name, *type, arg_list[2], environment, declarations, output);
    }
    output << ";\n";
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
    intern_emitter<EmitterFunction>("function");
    intern_emitter<EmitterVariable>("variable");
    intern_emitter<EmitterReturn>("return");
}
