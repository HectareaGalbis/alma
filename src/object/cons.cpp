
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

void Cons::emit(ObjectRef<Object> self, ObjectRef<Environment> environment,
    std::stringstream& output)
{
    Alma& alma = Alma::alma;

    if (!alma.symbolp(this->car))
        athrow("Cannot emit " << this->to_string(self) << ": the first element must be a symbol");

    ObjectRef<Object> car_ref = this->car;
    ObjectRef<Symbol> symbol = car_ref.as<Symbol>();
    if (!symbol->has_emitter())
        athrow("Cannot emit " << this->to_string(self) << ": the symbol "
                              << symbol->get_name() << " has no emitter");

    auto [list, rest] = this->to_list();
    aassert(!rest, "Cannot emit a non proper list " << this->to_string(self));
    std::vector<ObjectRef<Object>> arg_list(list.begin() + 1, list.end());

    ObjectRef<Emitter> emitter = symbol->get_emitter();
    emitter->handle(emitter, arg_list, environment, output);
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
