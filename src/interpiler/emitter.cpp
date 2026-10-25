
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

/** Indent each line of a piece of code */
static std::string indent(const std::string& code)
{
    std::string result;
    bool line_start = true;
    for (char c : code) {
        if (line_start && c != '\n')
            result += "    ";
        result.push_back(c);
        line_start = c == '\n';
    }
    return result;
}

/** Emit statements between braces, in the given scope */
static void emit_block(const std::vector<ObjectRef<Object>>& statements, ObjectRef<Environment> environment,
    Declarations& scope, std::stringstream& output)
{
    std::stringstream inner;
    for (const ObjectRef<Object>& statement : statements)
        Alma::alma.emit_statement(statement, environment, scope, inner);
    output << "{\n"
           << indent(inner.str()) << "}\n";
}

/** A list whose first element is a symbol with this name */
static bool is_clause(const ObjectRef<Object>& obj, const std::string& name)
{
    if (!Alma::alma.consp(obj))
        return false;
    ObjectRef<Object> car = obj.as<Cons>()->get_car();
    return Alma::alma.symbolp(car) && car.as<Symbol>()->get_name() == name;
}

/** A list that uses the emitter of the symbol with this name */
static bool is_form(const ObjectRef<Object>& obj, const std::string& name)
{
    return is_clause(obj, name) && ObjectRef<Symbol>(obj.as<Cons>()->get_car())->has_emitter();
}

/** Emit the body of an if: a (block ...) is emitted as the block, any other statement inside a new block */
static void emit_branch(const ObjectRef<Object>& statement, ObjectRef<Environment> environment,
    Declarations& declarations, std::stringstream& output)
{
    Declarations scope = declarations.block_scope();
    if (is_form(statement, "block"))
        emit_block(proper_list(statement.as<Cons>()->get_cdr(), "a block"), environment, scope, output);
    else
        emit_block({ statement }, environment, scope, output);
}

/** Emit the condition of a control statement, that must be a scalar */
static void emit_condition(const std::string& name, const ObjectRef<Object>& condition,
    ObjectRef<Environment> environment, Declarations& declarations, std::stringstream& output)
{
    std::unique_ptr<Type> type = Alma::alma.emit_expression(condition, environment, declarations, output);
    aassert(is_scalar(*type), "The condition of " << name << " must be a number or a pointer but "
                                                  << Alma::alma.to_string(condition) << " is of type " << type->to_string(""));
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
    if (has_body) {
        aassert(is_void(*result) || declarations.is_complete(*result),
            "The function " << function_name << " returns the incomplete type " << type_to_string(*result));
        for (size_t i = 0; i < parameter_types.size(); i++)
            aassert(declarations.is_complete(*parameter_types[i]),
                "The parameter " << parameter_names[i] << " of " << function_name << " has the incomplete type "
                                 << type_to_string(*parameter_types[i]));
    }
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

    output << type.to_string(function_name, parameter_names) << "\n";
    bool empty_body = arg_list.size() == 4 && Alma::alma.null(arg_list[3]);
    std::vector<ObjectRef<Object>> body;
    if (!empty_body)
        body.assign(arg_list.begin() + 3, arg_list.end());
    emit_block(body, environment, scope, output);

    std::vector<std::string> undefined = scope.undefined_labels();
    aassert(undefined.empty(), "The label '" << undefined.front() << "' used by goto is not defined in " << function_name);
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

    aassert(declarations.is_complete(*type),
        "The variable " << variable_name << " has the incomplete type " << type_to_string(*type));
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
    aassert(declarations.is_complete(*type),
        "The variable " << variable_name << " has the incomplete type " << type_to_string(*type));

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

EmitterStruct::EmitterStruct(bool _union_record)
    : Emitter(_union_record ? "union" : "struct")
    , union_record(_union_record)
{
}

void EmitterStruct::emit_top_level(ObjectRef<Object> self [[maybe_unused]],
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]],
    Declarations& declarations,
    std::stringstream& output)
{
    const std::string& keyword = get_name();
    aassert(!arg_list.empty(), keyword << " expects a name and optionally its members");
    std::string struct_name = symbol_name(arg_list[0], "the name of a " + keyword);
    aassert(!declarations.contains(struct_name), "'" << struct_name << "' is already declared");
    aassert(!declarations.is_enum(struct_name), "'" << struct_name << "' is already declared as an enum");

    // The typedef lets use the name of the struct as a type
    Records& records = declarations.get_records();
    if (!records.contains(struct_name)) {
        records.add(Record(struct_name, union_record, {}));
        output << "typedef " << keyword << " " << struct_name << " " << struct_name << ";\n";
    }
    aassert(records.get(struct_name).isUnion() == union_record,
        "'" << struct_name << "' is already declared as a " << (union_record ? "struct" : "union"));

    if (arg_list.size() == 1)
        return;

    aassert(!records.get(struct_name).isDefined(), "Redefinition of " << keyword << " '" << struct_name << "'");
    Record::Members members;
    for (size_t i = 1; i < arg_list.size(); i++) {
        std::vector<ObjectRef<Object>> pair = proper_list(arg_list[i], "a member of " + struct_name);
        aassert(pair.size() == 2,
            "Expected (name type) as member of " << struct_name << " but found " << Alma::alma.to_string(arg_list[i]));
        std::string member_name = symbol_name(pair[0], "the name of a member of " + struct_name);
        for (const auto& [previous_name, previous_type] : members)
            aassert(previous_name != member_name, "Duplicate member '" << member_name << "' in " << keyword << " " << struct_name);
        std::unique_ptr<Type> member_type = Type::parse_object(pair[1]);
        aassert(declarations.is_complete(*member_type),
            "The member " << member_name << " of " << struct_name << " has the incomplete type "
                          << type_to_string(*member_type));
        declarations.use_type(*member_type);
        members.emplace_back(member_name, std::move(member_type));
    }

    output << keyword << " " << struct_name << "\n{\n";
    for (const auto& [member_name, member_type] : members)
        output << "    " << member_type->to_string(member_name) << ";\n";
    output << "};\n";
    records.add(Record(struct_name, union_record, members, true));
}

// -----------------------------------------------------------------------------

EmitterEnum::EmitterEnum()
    : Emitter("enum")
{
}

void EmitterEnum::emit_top_level(ObjectRef<Object> self [[maybe_unused]],
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment,
    Declarations& declarations,
    std::stringstream& output)
{
    aassert(arg_list.size() >= 2, "enum expects a name and at least one constant");
    std::string enum_name = symbol_name(arg_list[0], "the name of an enum");
    aassert(!declarations.contains(enum_name), "'" << enum_name << "' is already declared");
    aassert(!declarations.get_records().contains(enum_name), "'" << enum_name << "' is already declared as a struct or union");
    aassert(!declarations.is_enum(enum_name), "Redefinition of enum '" << enum_name << "'");
    declare_enum_type(enum_name);

    // The constants are int constants, visible from the next constant
    std::unique_ptr<Type> int_type = std::make_unique<ValueType>("int");
    std::vector<std::string> constants;
    std::stringstream body;
    for (size_t i = 1; i < arg_list.size(); i++) {
        const ObjectRef<Object>& item = arg_list[i];
        std::string constant;
        std::optional<ObjectRef<Object>> value;
        if (Alma::alma.symbolp(item)) {
            constant = symbol_name(item, "a constant of " + enum_name);
        } else {
            std::vector<ObjectRef<Object>> pair = proper_list(item, "a constant of " + enum_name);
            aassert(pair.size() == 2,
                "Expected a name or (name value) as constant of " << enum_name << " but found " << Alma::alma.to_string(item));
            constant = symbol_name(pair[0], "the name of a constant of " + enum_name);
            value = pair[1];
        }

        body << "    " << constant;
        if (value) {
            aassert(is_constant(*value, declarations),
                "The value of " << constant << " must be a constant but found " << Alma::alma.to_string(*value));
            body << " = ";
            std::unique_ptr<Type> type = Alma::alma.emit_expression(*value, environment, declarations, body);
            aassert(is_integer(*type), "The value of " << constant << " must be an integer but "
                                                       << Alma::alma.to_string(*value) << " is of type " << type_to_string(*type));
        }
        body << (i + 1 < arg_list.size() ? ",\n" : "\n");

        declarations.add(Declaration(constant, int_type, true, std::nullopt, true));
        constants.push_back(constant);
    }
    declarations.add_enum(enum_name, constants);

    output << "typedef enum " << enum_name << "\n{\n"
           << body.str() << "} " << enum_name << ";\n";
}

// -----------------------------------------------------------------------------

EmitterBlock::EmitterBlock()
    : Emitter("block")
{
}

void EmitterBlock::emit_statement(ObjectRef<Object> self [[maybe_unused]],
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment,
    Declarations& declarations,
    std::stringstream& output)
{
    Declarations scope = declarations.block_scope();
    emit_block(arg_list, environment, scope, output);
}

// -----------------------------------------------------------------------------

EmitterIf::EmitterIf()
    : Emitter("if")
{
}

void EmitterIf::emit_statement(ObjectRef<Object> self [[maybe_unused]],
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment,
    Declarations& declarations,
    std::stringstream& output)
{
    aassert(arg_list.size() == 2 || arg_list.size() == 3,
        "if expects a condition, a statement and optionally another statement but received "
            << arg_list.size() << " arguments");

    output << "if (";
    emit_condition("if", arg_list[0], environment, declarations, output);
    output << ")\n";
    emit_branch(arg_list[1], environment, declarations, output);

    if (arg_list.size() == 3) {
        output << "else";
        if (is_form(arg_list[2], "if")) {
            output << " ";
            Alma::alma.emit_statement(arg_list[2], environment, declarations, output);
        } else {
            output << "\n";
            emit_branch(arg_list[2], environment, declarations, output);
        }
    }
}

// -----------------------------------------------------------------------------

EmitterWhile::EmitterWhile()
    : Emitter("while")
{
}

void EmitterWhile::emit_statement(ObjectRef<Object> self [[maybe_unused]],
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment,
    Declarations& declarations,
    std::stringstream& output)
{
    aassert(!arg_list.empty(), "while expects a condition and its statements");

    output << "while (";
    emit_condition("while", arg_list[0], environment, declarations, output);
    output << ")\n";
    Declarations scope = declarations.loop_scope();
    emit_block(std::vector<ObjectRef<Object>>(arg_list.begin() + 1, arg_list.end()), environment, scope, output);
}

// -----------------------------------------------------------------------------

EmitterFor::EmitterFor()
    : Emitter("for")
{
}

void EmitterFor::emit_statement(ObjectRef<Object> self [[maybe_unused]],
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment,
    Declarations& declarations,
    std::stringstream& output)
{
    aassert(!arg_list.empty(), "for expects (init condition step) and its statements");
    std::vector<ObjectRef<Object>> header = proper_list(arg_list[0], "the header of for");
    aassert(header.size() == 3,
        "for expects (init condition step) but found " << Alma::alma.to_string(arg_list[0]));
    const ObjectRef<Object>& init = header[0];
    const ObjectRef<Object>& condition = header[1];
    const ObjectRef<Object>& step = header[2];

    // The variables of init only exist inside the for
    Declarations scope = declarations.block_scope();

    output << "for (";
    if (is_form(init, "variable")) {
        // A declaration is emitted as a statement, without its final ";\n"
        std::stringstream declaration;
        Alma::alma.emit_statement(init, environment, scope, declaration);
        std::string code = declaration.str();
        output << code.substr(0, code.size() - 2);
    } else if (!Alma::alma.null(init)) {
        Alma::alma.emit_expression(init, environment, scope, output);
    }
    output << ";";
    if (!Alma::alma.null(condition)) {
        output << " ";
        emit_condition("for", condition, environment, scope, output);
    }
    output << ";";
    if (!Alma::alma.null(step)) {
        output << " ";
        Alma::alma.emit_expression(step, environment, scope, output);
    }
    output << ")\n";

    Declarations body_scope = scope.loop_scope();
    emit_block(std::vector<ObjectRef<Object>>(arg_list.begin() + 1, arg_list.end()), environment, body_scope, output);
}

// -----------------------------------------------------------------------------

EmitterDoWhile::EmitterDoWhile()
    : Emitter("do-while")
{
}

void EmitterDoWhile::emit_statement(ObjectRef<Object> self [[maybe_unused]],
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment,
    Declarations& declarations,
    std::stringstream& output)
{
    aassert(!arg_list.empty(), "do-while expects a condition and its statements");

    output << "do\n";
    Declarations scope = declarations.loop_scope();
    emit_block(std::vector<ObjectRef<Object>>(arg_list.begin() + 1, arg_list.end()), environment, scope, output);
    output << "while (";
    emit_condition("do-while", arg_list[0], environment, declarations, output);
    output << ");\n";
}

// -----------------------------------------------------------------------------

EmitterSwitch::EmitterSwitch()
    : Emitter("switch")
{
}

void EmitterSwitch::emit_statement(ObjectRef<Object> self [[maybe_unused]],
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment,
    Declarations& declarations,
    std::stringstream& output)
{
    aassert(!arg_list.empty(), "switch expects an expression and its cases");

    output << "switch (";
    std::unique_ptr<Type> type = Alma::alma.emit_expression(arg_list[0], environment, declarations, output);
    aassert(is_integer(*type), "The expression of switch must be an integer but "
                                   << Alma::alma.to_string(arg_list[0]) << " is of type " << type_to_string(*type));
    output << ")\n";

    Declarations scope = declarations.switch_scope();
    std::stringstream cases;
    bool has_default = false;
    for (size_t i = 1; i < arg_list.size(); i++) {
        const ObjectRef<Object>& clause = arg_list[i];
        std::vector<ObjectRef<Object>> statements;
        if (is_clause(clause, "case")) {
            std::vector<ObjectRef<Object>> list = proper_list(clause.as<Cons>()->get_cdr(), "a case");
            aassert(!list.empty(), "case expects a constant and its statements");
            aassert(is_constant(list[0], declarations),
                "The value of a case must be a constant but found " << Alma::alma.to_string(list[0]));
            cases << "case ";
            std::unique_ptr<Type> case_type = Alma::alma.emit_expression(list[0], environment, scope, cases);
            aassert(is_integer(*case_type), "The value of a case must be an integer but "
                                                << Alma::alma.to_string(list[0]) << " is of type " << type_to_string(*case_type));
            cases << ":\n";
            statements.assign(list.begin() + 1, list.end());
        } else if (is_clause(clause, "default")) {
            aassert(!has_default, "switch can only have one default");
            has_default = true;
            cases << "default:\n";
            statements = proper_list(clause.as<Cons>()->get_cdr(), "default");
        } else {
            athrow("Expected (case ...) or (default ...) in switch but found " << Alma::alma.to_string(clause));
        }

        // Each case has its own block, so it can declare variables
        if (!statements.empty()) {
            Declarations case_scope = scope.block_scope();
            emit_block(statements, environment, case_scope, cases);
        }
    }
    output << "{\n"
           << cases.str() << "}\n";
}

// -----------------------------------------------------------------------------

EmitterJump::EmitterJump(const std::string& _name)
    : Emitter(_name)
{
}

void EmitterJump::emit_statement(ObjectRef<Object> self [[maybe_unused]],
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]],
    Declarations& declarations,
    std::stringstream& output)
{
    aassert(arg_list.empty(), get_name() << " does not expect arguments");
    if (get_name() == "break") {
        aassert(declarations.can_break(), "break can only be used inside a loop or a switch");
    } else {
        aassert(declarations.can_continue(), "continue can only be used inside a loop");
    }
    output << get_name() << ";\n";
}

// -----------------------------------------------------------------------------

EmitterLabel::EmitterLabel(const std::string& _name)
    : Emitter(_name)
{
}

void EmitterLabel::emit_statement(ObjectRef<Object> self [[maybe_unused]],
    const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Environment> environment [[maybe_unused]],
    Declarations& declarations,
    std::stringstream& output)
{
    aassert(arg_list.size() == 1, get_name() << " expects the name of a label but received "
                                             << arg_list.size() << " arguments");
    std::string label = symbol_name(arg_list[0], "the name of a label");
    if (get_name() == "label") {
        declarations.define_label(label);
        // The empty statement allows a label at the end of a block or before a declaration
        output << label << ":;\n";
    } else {
        declarations.use_label(label);
        output << "goto " << label << ";\n";
    }
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

template <typename T, typename... AS>
static void intern_emitter(const std::string& name, AS&&... args)
{
    ObjectRef<Symbol> sym = Alma::alma.intern_alma_symbol(name);
    sym->push_emitter(Alma::alma.make<T>(std::forward<AS>(args)...));
}

void intern_emitters()
{
    intern_emitter<EmitterFunction>("function");
    intern_emitter<EmitterVariable>("variable");
    intern_emitter<EmitterStruct>("struct", false);
    intern_emitter<EmitterStruct>("union", true);
    intern_emitter<EmitterEnum>("enum");
    intern_emitter<EmitterReturn>("return");
    intern_emitter<EmitterBlock>("block");
    intern_emitter<EmitterIf>("if");
    intern_emitter<EmitterWhile>("while");
    intern_emitter<EmitterFor>("for");
    intern_emitter<EmitterDoWhile>("do-while");
    intern_emitter<EmitterSwitch>("switch");
    intern_emitter<EmitterJump>("break", "break");
    intern_emitter<EmitterJump>("continue", "continue");
    intern_emitter<EmitterLabel>("label", "label");
    intern_emitter<EmitterLabel>("goto", "goto");
}
