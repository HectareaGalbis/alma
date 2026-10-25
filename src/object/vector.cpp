#include "vector.hpp"
#include "alma.hpp"
#include "debug.hpp"
#include "integer.hpp"
#include <sstream>

Vector::Vector(const std::vector<ObjectRef<Object>>& _elements)
{
    for (const ObjectRef<Object>& element : _elements)
        this->elements.emplace_back(*this, element);
}

void Vector::check_index(size_t index, size_t size) const
{
    aassert(index < size, "The index " << index << " is out of the vector of size " << this->elements.size());
}

size_t Vector::size() const
{
    return this->elements.size();
}

ObjectRef<Object> Vector::get(size_t index)
{
    this->check_index(index, this->elements.size());
    return this->elements[index];
}

void Vector::set(size_t index, ObjectRef<Object> value)
{
    this->check_index(index, this->elements.size());
    this->elements[index] = value;
}

ObjectRef<Object> Vector::front()
{
    aassert(!this->elements.empty(), "The vector is empty");
    return this->elements.front();
}

ObjectRef<Object> Vector::back()
{
    aassert(!this->elements.empty(), "The vector is empty");
    return this->elements.back();
}

void Vector::push_front(ObjectRef<Object> value)
{
    this->elements.emplace_front(*this, value);
}

void Vector::push_back(ObjectRef<Object> value)
{
    this->elements.emplace_back(*this, value);
}

ObjectRef<Object> Vector::pop_front()
{
    ObjectRef<Object> value = this->front();
    this->elements.pop_front();
    return value;
}

ObjectRef<Object> Vector::pop_back()
{
    ObjectRef<Object> value = this->back();
    this->elements.pop_back();
    return value;
}

void Vector::insert(size_t index, ObjectRef<Object> value)
{
    this->check_index(index, this->elements.size() + 1);
    this->elements.emplace(this->elements.begin() + static_cast<std::ptrdiff_t>(index), *this, value);
}

ObjectRef<Object> Vector::erase(size_t index)
{
    ObjectRef<Object> value = this->get(index);
    this->elements.erase(this->elements.begin() + static_cast<std::ptrdiff_t>(index));
    return value;
}

void Vector::clear()
{
    this->elements.clear();
}

std::vector<ObjectRef<Object>> Vector::to_vector()
{
    return std::vector<ObjectRef<Object>>(this->elements.begin(), this->elements.end());
}

ObjectRef<Object> Vector::apply(ObjectRef<Object> self [[maybe_unused]],
    const std::vector<ObjectRef<Object>>& arg_list, ObjectRef<Environment> environment)
{
    aassert(arg_list.size() == 1, "A vector expects an index but received " << arg_list.size() << " arguments");
    // As with functions, the argument is evaluated
    ObjectRef<Object> index = Alma::alma.eval(arg_list[0], environment);
    aassert(Alma::alma.typep(index, Alma::alma.intern_alma_symbol("integer")),
        "The index of a vector must be an integer but found " << Alma::alma.to_string(index));
    int64_t value = index.as<Integer>()->get_value();
    aassert(value >= 0, "The index of a vector cannot be negative: " << value);
    return this->get(static_cast<size_t>(value));
}

std::string Vector::to_string(ObjectRef<Object> self [[maybe_unused]])
{
    std::stringstream s;
    s << "[";
    for (size_t i = 0; i < this->elements.size(); i++) {
        if (i > 0)
            s << " ";
        s << Alma::alma.to_string(this->elements[i]);
    }
    s << "]";
    return s.str();
}

ObjectRef<Object> Vector::type(ObjectRef<Object> self [[maybe_unused]]) const
{
    return Alma::alma.intern_alma_symbol("vector");
}
