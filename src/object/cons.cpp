
#include "cons.hpp"
#include "alma.hpp"
#include "debug.hpp"
#include "interpiler/emitter.hpp"
#include "package.hpp"
#include "procedure.hpp"
#include "symbol.hpp"
#include <sstream>

// -----------------------------------------------------------------------------

Cons::Cons(ObjectRef<Object> _car, ObjectRef<Object> _cdr)
    : car(*this, _car)
    , cdr(*this, _cdr)
{
}

static ObjectRef<Object> get_list_car(const std::vector<ObjectRef<Object>>& list)
{
    if (list.empty())
        mthrow("The list must not be empty");

    return list.front();
}

static ObjectRef<Object> get_list_cdr(const std::vector<ObjectRef<Object>>& list,
    size_t currentIndex, ObjectRef<Object> non_proper_element, Alma& alma)
{
    if (list.empty())
        mthrow("The list must not be empty");

    if (currentIndex >= list.size())
        return non_proper_element;
    else
        return alma.make<Cons>(
            list[currentIndex], get_list_cdr(list, currentIndex + 1, non_proper_element, alma));
}

Cons::Cons(const std::vector<ObjectRef<Object>>& list)
    : car(*this, get_list_car(list))
    , cdr(*this, get_list_cdr(list, 1, Alma::alma.intern_alma_symbol("nil"), Alma::alma))
{
}

Cons::Cons(const std::vector<ObjectRef<Object>>& list, ObjectRef<Object> non_proper_element)
    : car(*this, get_list_car(list))
    , cdr(*this, get_list_cdr(list, 1, non_proper_element, Alma::alma))
{
}

std::pair<std::vector<ObjectRef<Object>>, ObjectRef<Object>> Cons::to_list() const
{
    std::vector<ObjectRef<Object>> list;
    list.push_back(this->car);
    ObjectRef<Object> argIt = this->cdr;
    while (argIt) {
        if (!Alma::alma.consp(argIt)) {
            return { list, argIt };
        }
        list.push_back(argIt.as<Cons>()->car);
        argIt = argIt.as<Cons>()->cdr;
    }
    return { list, Alma::alma.intern_alma_symbol("nil") };
}

ObjectRef<Object> Cons::expand(ObjectRef<Object> self, ObjectRef<Environment> environment)
{
    ObjectRef<Object> callee = Alma::alma.eval(this->car, environment);
    if (!dynamic_cast<Transformable*>(callee.get()))
        return self;
    return Alma::alma.transform(callee, this->cdr, environment);
}

ObjectRef<Object> Cons::eval(ObjectRef<Object> self [[maybe_unused]], ObjectRef<Environment> environment)
{
    ObjectRef<Object> callee = Alma::alma.eval(this->car, environment);
    return Alma::alma.apply(callee, this->cdr, environment);
}

static std::optional<ObjectRef<Emitter>> find_emitter(ObjectRef<Object> obj)
{
    if (!Alma::alma.symbolp(obj))
        return std::nullopt;
    ObjectRef<Symbol> symbol(obj);
    if (!symbol->has_emitter())
        return std::nullopt;
    return symbol->get_emitter();
}

static std::vector<ObjectRef<Object>> emit_arguments(const Cons& cons, ObjectRef<Object> self)
{
    auto [list, rest] = cons.to_list();
    aassert(Alma::alma.null(rest), "Cannot emit a non proper list " << Alma::alma.to_string(self));
    return std::vector<ObjectRef<Object>>(list.begin() + 1, list.end());
}

/** Emit a call to a declared function */
static std::unique_ptr<Type> emit_call(ObjectRef<Object> callee, const std::vector<ObjectRef<Object>>& arg_list,
    ObjectRef<Object> self, ObjectRef<Environment> environment, Declarations& declarations, std::stringstream& output)
{
    Alma& alma = Alma::alma;

    aassert(alma.symbolp(callee),
        "Cannot emit " << alma.to_string(self) << ": the first element must be a symbol");
    const std::string& name = callee.as<Symbol>()->get_name();
    aassert(declarations.contains(name), "The function " << name << " is not declared");

    const FunctionType* type = dynamic_cast<const FunctionType*>(declarations.get(name).getType());
    aassert(type && (type->is_function() || (type->is_pointer() && type->get_specifiers().size() == 1)),
        name << " is not a function");

    const std::vector<std::unique_ptr<Type>>& parameters = type->get_arguments();
    if (type->is_variadic()) {
        aassert(arg_list.size() >= parameters.size(), "The function " << name << " expects at least "
                                                                      << parameters.size() << " arguments but received " << arg_list.size());
    } else {
        aassert(arg_list.size() == parameters.size(), "The function " << name << " expects "
                                                                      << parameters.size() << " arguments but received " << arg_list.size());
    }

    output << name << "(";
    for (size_t i = 0; i < arg_list.size(); i++) {
        if (i > 0)
            output << ", ";
        std::unique_ptr<Type> argument_type = alma.emit_expression(arg_list[i], environment, declarations, output);
        if (i < parameters.size()) {
            std::unique_ptr<Type> parameter_type = parameters[i]->decay();
            aassert(is_assignable(*parameter_type, *argument_type),
                "The argument " << i + 1 << " of " << name << " must be of type " << parameter_type->to_string("")
                                << " but " << alma.to_string(arg_list[i]) << " is of type " << argument_type->to_string(""));
        }
    }
    output << ")";

    return type->get_result().clone();
}

void Cons::emit_top_level(ObjectRef<Object> self, ObjectRef<Environment> environment,
    Declarations& declarations, std::stringstream& output)
{
    std::optional<ObjectRef<Emitter>> emitter = find_emitter(this->car);
    aassert(emitter, "Cannot emit " << this->to_string(self) << " at the top level");
    (*emitter)->emit_top_level(*emitter, emit_arguments(*this, self), environment, declarations, output);
}

void Cons::emit_statement(ObjectRef<Object> self, ObjectRef<Environment> environment,
    Declarations& declarations, std::stringstream& output)
{
    std::optional<ObjectRef<Emitter>> emitter = find_emitter(this->car);
    if (emitter) {
        (*emitter)->emit_statement(*emitter, emit_arguments(*this, self), environment, declarations, output);
    } else {
        emit_call(this->car, emit_arguments(*this, self), self, environment, declarations, output);
        output << ";\n";
    }
}

std::unique_ptr<Type> Cons::emit_expression(ObjectRef<Object> self, ObjectRef<Environment> environment,
    Declarations& declarations, std::stringstream& output)
{
    std::optional<ObjectRef<Emitter>> emitter = find_emitter(this->car);
    if (emitter)
        return (*emitter)->emit_expression(*emitter, emit_arguments(*this, self), environment, declarations, output);
    return emit_call(this->car, emit_arguments(*this, self), self, environment, declarations, output);
}

std::string Cons::to_string(ObjectRef<Object> self [[maybe_unused]])
{
    Alma& alma = Alma::alma;

    std::stringstream s;
    s << "(";
    s << alma.to_string(this->car);
    ObjectRef<Object> it = this->cdr;
    while (alma.truep(it)) {
        s << " ";
        if (alma.consp(it)) {
            s << alma.to_string(it.as<Cons>()->car);
        } else {
            s << ". ";
            s << alma.to_string(it);
            break;
        }
        it = it.as<Cons>()->cdr;
    }
    s << ")";

    return s.str();
}

ObjectRef<Object> Cons::type(ObjectRef<Object> self [[maybe_unused]]) const
{
    return Alma::alma.intern_alma_symbol("cons");
}

ObjectRef<Object> Cons::get_car()
{
    return this->car;
}

ObjectRef<Object> Cons::get_cdr()
{
    return this->cdr;
}
