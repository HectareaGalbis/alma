
#include "symbol.hpp"
#include "alma.hpp"
#include "debug.hpp"
#include "environment.hpp"
#include "object.hpp"
#include "package.hpp"

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

Symbol::Symbol(Alma& _alma, const std::string& _name)
    : Object(_alma)
    , name(_name)
{
}

ObjectRef<Object> Symbol::eval(ObjectRef<Object> self, ObjectRef<Environment> environment)
{
    std::optional<ObjectRef<Object>> lex_value
        = environment->get_value(self, this->alma.intern_alma_symbol("value"));
    if (lex_value) {
        return *lex_value;
    } else {
        return this->get_value();
    }
}

std::string Symbol::to_string()
{
    return this->name;
}

bool Symbol::typep(ObjectRef<Object> self, ObjectRef<Object> type)
{
    return type == this->alma.intern_alma_symbol("symbol") || this->Object::typep(self, type);
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
    }
    mthrow("No property " << property->get_name() << " found for the symbol " << this->name);
}

bool Symbol::has_value()
{
    return this->has_property(this->alma.intern_alma_symbol("value"));
}

ObjectRef<Object> Symbol::get_value()
{
    return this->get_property(this->alma.intern_alma_symbol("value"));
}

void Symbol::set_value(ObjectRef<Object> value)
{
    this->set_property(this->alma.intern_alma_symbol("value"), value);
}

void Symbol::push_value(ObjectRef<Object> value)
{
    this->push_property(this->alma.intern_alma_symbol("value"), value);
}

void Symbol::pop_value()
{
    this->pop_property(this->alma.intern_alma_symbol("value"));
}

bool Symbol::has_package()
{
    return this->has_property(this->alma.intern_alma_symbol("package"));
}

ObjectRef<Package> Symbol::get_package()
{
    return this->get_property(this->alma.intern_alma_symbol("package"));
}

void Symbol::set_package(ObjectRef<Package> package)
{
    this->set_property(this->alma.intern_alma_symbol("package"), package);
}

void Symbol::push_package(ObjectRef<Package> package)
{
    this->push_property(this->alma.intern_alma_symbol("package"), package);
}

void Symbol::pop_package()
{
    this->pop_property(this->alma.intern_alma_symbol("package"));
}
