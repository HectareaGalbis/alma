
#include "symbol.hpp"
#include "alma.hpp"
#include "interpiler/declaration.hpp"
#include "debug.hpp"
#include "environment.hpp"
#include "object.hpp"
#include "package.hpp"
#include "interpiler/emitter.hpp"

// void intern_symbols()
// {
//     Package::almaPackage->intern_symbol("unquote");
//     Package::almaPackage->intern_symbol("slice-unquote");

//     std::shared_ptr<Symbol> current_package_sym = Package::almaPackage->intern_symbol("*current-package*");
//     current_package_sym->values = { Package::currentPackage };

//     std::shared_ptr<Symbol> t_sym = Package::almaPackage->intern_symbol("t");
//     t_sym->values = { t_sym };

//     std::shared_ptr<Symbol> nil_sym = Package::almaPackage->intern_symbol("nil");
//     nil_sym->values = { std::make_shared<Nil>() };
// }

Symbol::Symbol(const std::string& _name)
    : name(_name)
    , home_package(*this, static_cast<Package*>(nullptr))
{
}

ObjectRef<Object> Symbol::eval(ObjectRef<Object> self, ObjectRef<Environment> environment)
{
    std::optional<ObjectRef<Object>> lex_value
        = environment->get_value(self, Alma::alma.intern_alma_symbol("value"));
    if (lex_value) {
        return *lex_value;
    } else {
        return this->get_value();
    }
}

std::string Symbol::to_string(ObjectRef<Object> self)
{
    std::optional<ObjectRef<Package>> home = this->get_home_package();
    if (!home)
        return "::" + this->name;
    if ((*home)->is_keyword())
        return ":" + this->name;
    std::optional<ObjectRef<Package>> current = Alma::alma.find_current_package();
    if (current) {
        std::optional<Package::Lookup> found = (*current)->find_accessible_symbol(this->name);
        if (found && found->symbol.get() == self.get())
            return this->name;
    }
    return (*home)->get_name() + ((*home)->is_external(this->name) ? ":" : "::") + this->name;
}

ObjectRef<Object> Symbol::type(ObjectRef<Object> self [[maybe_unused]]) const
{
    return Alma::alma.intern_alma_symbol("symbol");
}

std::string& Symbol::get_name()
{
    return this->name;
}

const std::string& Symbol::get_name() const
{
    return this->name;
}

bool Symbol::has_property(ObjectRef<Symbol> property)
{
    return (this->properties.contains(property) && this->properties.find(property)->second.size() > 0);
}

ObjectRef<Object> Symbol::get_property(ObjectRef<Symbol> property)
{
    auto it = this->properties.find(property);
    if (it != this->properties.end() && it->second.size() > 0) {
        return it->second.back();
    }
    athrow("No property " << property->get_name() << " found for the symbol " << this->name);
}

void Symbol::set_property(ObjectRef<Symbol> property, ObjectRef<Object> value)
{
    auto it = this->properties.find(property);
    if (it != this->properties.end() && it->second.size() > 0) {
        it->second.back() = value;
        return;
    }
    mthrow("No property " << property->get_name() << " found for the symbol " << this->name);
}

void Symbol::push_property(ObjectRef<Symbol> property, ObjectRef<Object> value)
{
    auto it = this->properties.find(property);
    if (it == this->properties.end())
        this->properties.try_emplace(ObjectTrackedKeyRef<Symbol>(*this, property));
    this->properties.find(property)->second.emplace_back(*this, value);
}

void Symbol::pop_property(ObjectRef<Symbol> property)
{
    auto it = this->properties.find(property);
    if (it != this->properties.end() && it->second.size() > 0) {
        it->second.pop_back();
        return;
    }
    mthrow("No property " << property->get_name() << " found for the symbol " << this->name);
}

std::optional<ObjectRef<Package>> Symbol::get_home_package()
{
    if (this->home_package.get() == nullptr)
        return std::nullopt;
    return ObjectRef<Package>(this->home_package);
}

void Symbol::set_home_package(ObjectRef<Package> package)
{
    this->home_package = package;
}

void Symbol::clear_home_package()
{
    this->home_package = static_cast<Package*>(nullptr);
}

bool Symbol::has_value()
{
    return this->has_property(Alma::alma.intern_alma_symbol("value"));
}

ObjectRef<Object> Symbol::get_value()
{
    return this->get_property(Alma::alma.intern_alma_symbol("value"));
}

void Symbol::set_value(ObjectRef<Object> value)
{
    this->set_property(Alma::alma.intern_alma_symbol("value"), value);
}

void Symbol::push_value(ObjectRef<Object> value)
{
    this->push_property(Alma::alma.intern_alma_symbol("value"), value);
}

void Symbol::pop_value()
{
    this->pop_property(Alma::alma.intern_alma_symbol("value"));
}

bool Symbol::has_emitter()
{
    return this->has_property(Alma::alma.intern_alma_symbol("emitter"));
}

ObjectRef<Emitter> Symbol::get_emitter()
{
    return this->get_property(Alma::alma.intern_alma_symbol("emitter"));
}

void Symbol::set_emitter(ObjectRef<Emitter> emitter)
{
    this->set_property(Alma::alma.intern_alma_symbol("emitter"), emitter);
}

void Symbol::push_emitter(ObjectRef<Emitter> emitter)
{
    this->push_property(Alma::alma.intern_alma_symbol("emitter"), emitter);
}

void Symbol::pop_emitter()
{
    this->pop_property(Alma::alma.intern_alma_symbol("emitter"));
}

void Symbol::emit_top_level(ObjectRef<Object> self, ObjectRef<Environment> environment [[maybe_unused]],
    Declarations& declarations [[maybe_unused]], std::stringstream& output [[maybe_unused]])
{
    athrow("The symbol " << this->to_string(self) << " cannot be emitted at the top level");
}

void Symbol::emit_statement(ObjectRef<Object> self, ObjectRef<Environment> environment,
    Declarations& declarations, std::stringstream& output)
{
    Alma::alma.emit_expression(self, environment, declarations, output);
    output << ";\n";
}

std::unique_ptr<Type> Symbol::emit_expression(ObjectRef<Object> self [[maybe_unused]],
    ObjectRef<Environment> environment [[maybe_unused]],
    Declarations& declarations [[maybe_unused]], std::stringstream& output)
{
    aassert(declarations.contains(this->name), "'" << this->name << "' is not declared");
    output << this->name;
    return declarations.use(this->name).getType()->clone();
}
