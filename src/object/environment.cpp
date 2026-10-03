
#include "environment.hpp"
#include "debug.hpp"
#include <stdexcept>

Environment::Layer::Property::Property(Environment& _owner)
    : owner(_owner)
{
}

Environment::Layer::Property::Property(
    Environment& _owner, const Property& other)
    : Property(_owner)
{
    for (auto& [key, value] : other.values)
        this->values.try_emplace(
            ObjectTrackedKeyRef<Symbol>(this->owner, key),
            this->owner, value);
}

bool Environment::Layer::Property::has_symbol(ObjectRef<Symbol> symbol) const
{
    return this->values.contains(symbol);
}

void Environment::Layer::Property::insert(
    ObjectRef<Symbol> symbol, ObjectRef<Object> value)
{
    if (this->has_symbol(symbol))
        mthrow("The symbol " << symbol->get_name() << " is already in the environment property");
    this->values.try_emplace(ObjectTrackedKeyRef<Symbol>(this->owner, symbol), this->owner, value);
}

void Environment::Layer::Property::set_value(
    ObjectRef<Symbol> symbol, ObjectRef<Object> value)
{
    if (!this->has_symbol(symbol))
        mthrow("The symbol " << symbol->get_name() << " is not in the environment property");
    this->values.find(symbol)->second = value;
}

void Environment::Layer::Property::insert_or_set_value(
    ObjectRef<Symbol> symbol, ObjectRef<Object> value)
{
    if (!this->has_symbol(symbol))
        this->values.try_emplace(ObjectTrackedKeyRef<Symbol>(this->owner, symbol), this->owner, value);
    else
        this->values.find(symbol)->second = value;
}

std::optional<ObjectRef<Object>> Environment::Layer::Property::get_value(
    ObjectRef<Symbol> symbol) const
{
    if (!this->has_symbol(symbol))
        return std::nullopt;
    return this->values.find(symbol)->second;
}

// --------------------------------------------------------------------------------

Environment::Layer::Layer(Environment& _owner)
    : owner(_owner)
{
}

Environment::Layer::Layer(Environment& _owner, const Layer& other)
    : Layer(_owner)
{
    for (auto& [key, value] : other.properties)
        this->properties.emplace(std::piecewise_construct,
            std::forward_as_tuple(this->owner, key),
            std::forward_as_tuple(this->owner, value));
}

bool Environment::Layer::has_property(ObjectRef<Symbol> property) const
{
    return this->properties.contains(property);
}

bool Environment::Layer::has_symbol_property(
    ObjectRef<Symbol> symbol, ObjectRef<Symbol> property) const
{
    return this->has_property(property) && this->properties.find(property)->second.has_symbol(symbol);
}

void Environment::Layer::insert(
    ObjectRef<Symbol> symbol, ObjectRef<Symbol> property, ObjectRef<Object> value)
{
    if (!this->has_property(property)) {
        this->properties.emplace(std::piecewise_construct,
            std::forward_as_tuple(this->owner, property),
            std::forward_as_tuple(this->owner));
    }
    Property& env_property = this->properties.find(property)->second;
    if (env_property.has_symbol(symbol))
        mthrow("The symbol " + symbol->get_name() + " already has the property " << property->get_name());
    env_property.insert(symbol, value);
}

void Environment::Layer::set_value(
    ObjectRef<Symbol> symbol, ObjectRef<Symbol> property, ObjectRef<Object> value)
{
    if (!this->has_symbol_property(symbol, property))
        mthrow("The symbol " << symbol->get_name() << " has not the property " << property->get_name());
    this->properties.find(symbol)->second.set_value(symbol, value);
}

void Environment::Layer::insert_or_set_value(
    ObjectRef<Symbol> symbol, ObjectRef<Symbol> property, ObjectRef<Object> value)
{
    if (!this->has_property(property)) {
        this->properties.emplace(std::piecewise_construct,
            std::forward_as_tuple(this->owner, property),
            std::forward_as_tuple(this->owner));
    }
    this->properties.find(property)->second.insert_or_set_value(symbol, value);
}

std::optional<ObjectRef<Object>> Environment::Layer::get_value(
    ObjectRef<Symbol> symbol, ObjectRef<Symbol> property) const
{
    if (!this->has_symbol_property(symbol, property))
        return std::nullopt;
    return this->properties.find(property)->second.get_value(symbol);
}

// --------------------------------------------------------------------------------

Environment::WithLayer::WithLayer(Environment& _environment)
    : environment(_environment)
{
    this->environment.layers.emplace_back(this->environment);
}

Environment::WithLayer::~WithLayer()
{
    this->environment.layers.pop_back();
}

// --------------------------------------------------------------------------------

Environment::Environment(Alma& _alma)
    : Object(_alma)
{
    this->layers.emplace_back(*this);
}

Environment::Environment(const Environment& other)
    : Object(other)
{
    for (const Layer& layer : other.layers)
        this->layers.emplace_back(*this, layer);
}

bool Environment::has_symbol_property(ObjectRef<Symbol> symbol, ObjectRef<Symbol> property) const
{
    size_t len = this->layers.size();
    for (size_t i = 0; i < len; i++) {
        if (this->layers[len - i - 1].has_symbol_property(symbol, property))
            return true;
    }
    return false;
}

void Environment::insert_or_set_value(
    ObjectRef<Symbol> symbol, ObjectRef<Symbol> property, ObjectRef<Object> value)
{
    if (this->layers.empty())
        mthrow("The lexical environment is empty.");
    this->layers.back().insert_or_set_value(symbol, property, value);
}

void Environment::set_value(
    ObjectRef<Symbol> symbol, ObjectRef<Symbol> property, ObjectRef<Object> value)
{
    size_t len = this->layers.size();
    for (size_t i = 0; i < len; i++) {
        if (this->layers[len - i - 1].has_symbol_property(symbol, property)) {
            this->layers[len - i - 1].set_value(symbol, property, value);
            return;
        }
    }
    mthrow("The symbol " << symbol->get_name() << " has not the property " << property->get_name() << " in the lexcal environment");
}

std::optional<ObjectRef<Object>> Environment::get_value(ObjectRef<Symbol> symbol, ObjectRef<Symbol> property) const
{
    size_t len = this->layers.size();
    for (size_t i = 0; i < len; i++) {
        if (this->layers[len - i - 1].has_symbol_property(symbol, property))
            return this->layers[len - i - 1].get_value(symbol, property);
    }
    return std::nullopt;
}
