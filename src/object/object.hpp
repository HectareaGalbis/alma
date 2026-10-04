
#pragma once

#include "gc/gc.hpp"
#include <string>
#include <vector>

class Object;
template <typename T>
class ObjectTrackedRef;
template <typename T>
class ObjectTrackedKeyRef;
template <typename T>
class ObjectRef;

// -----------------------------------------------------------------------------

template <typename T, typename S>
concept Related = std::is_base_of_v<T, S> || std::is_base_of_v<S, T>;

// -----------------------------------------------------------------------------

template <typename T>
concept ObjectRefType
    = (std::is_same_v<typename std::remove_cvref_t<T>::template rebind<int>, ObjectTrackedRef<int>>
        || std::is_same_v<typename std::remove_cvref_t<T>::template rebind<int>, ObjectTrackedKeyRef<int>>
        || std::is_same_v<typename std::remove_cvref_t<T>::template rebind<int>, ObjectRef<int>>);

template <typename T, typename S>
concept ObjectRefRelatedType
    = ObjectRefType<T> && Related<S, typename std::remove_cvref_t<T>::value_type>;

// -----------------------------------------------------------------------------

// class Object : public GCObject {
//     template <typename S>
//     friend class ObjectRef;

// public:
//     Alma& alma;

// private:
//     static void protect_object(Alma& alma, GCObject** object);
//     static void unprotect_object(Alma& alma, GCObject** object);

// public:
//     Object(Alma& alma);
//     Object(const Object& other);
//     Object(const Object&& other);

//     virtual ObjectRef<Object> expand(ObjectRef<Object> self, ObjectRef<Environment> enviroment);
//     virtual ObjectRef<Object> transform(
//         ObjectRef<Object> self,
//         const std::vector<ObjectRef<Object>>& arg_list,
//         ObjectRef<Environment> enviroment);
//     virtual ObjectRef<Object> eval(ObjectRef<Object> self, ObjectRef<Environment> enviroment);
//     virtual ObjectRef<Object> apply(
//         ObjectRef<Object> self,
//         const std::vector<ObjectRef<Object>>& arg_list,
//         ObjectRef<Environment> enviroment);
//     virtual std::string to_string();
//     virtual bool typep(ObjectRef<Object> self, ObjectRef<Object> type);//     operator bool();
// };

class Object : public GCObject {
    template <typename S>
    friend class ObjectRef;

private:
    static void protect_object(GCObject** object);
    static void unprotect_object(GCObject** object);

public:
    virtual ObjectRef<Object> eval(ObjectRef<Object> self, ObjectRef<class Environment> environment);
    virtual ObjectRef<Object> expand(ObjectRef<Object> self, ObjectRef<class Environment> environment);
    virtual ObjectRef<Object> apply(ObjectRef<Object> self,
        const std::vector<ObjectRef<Object>>& arg_list, ObjectRef<class Environment> environment);
    virtual std::string to_string(ObjectRef<Object> self);
    virtual operator bool();
};

// -----------------------------------------------------------------------------

template <typename T>
class ObjectWeakRef {
    template <typename S>
    friend class ObjectTrackedRef;
    template <typename S>
    friend class ObjectWeakRef;
    template <typename S>
    friend class ObjectRef;
    template <typename S>
    friend class ObjectTrackedKeyRef;
    friend struct ObjectRefHash;
    friend struct ObjectRefEqual;

public:
    template <typename S>
    using rebind = ObjectWeakRef<S>;
    using value_type = T;

private:
    GCObject* obj;

protected:
    ObjectWeakRef(const ObjectWeakRef& other);
    ObjectWeakRef(ObjectWeakRef&& other);
    template <ObjectRefRelatedType<T> S>
    ObjectWeakRef(S&& other);
    template <Related<T> S>
    ObjectWeakRef(S* obj);
    ObjectWeakRef(std::nullptr_t) = delete;

    ObjectWeakRef& operator=(const ObjectWeakRef& other);
    ObjectWeakRef& operator=(ObjectWeakRef&& other);
    template <ObjectRefRelatedType<T> S>
    ObjectWeakRef& operator=(S&& other);
    template <Related<T> S>
    ObjectWeakRef& operator=(S* obj);
    ObjectWeakRef& operator=(std::nullptr_t) = delete;

public:
    template <Related<T> S>
    ObjectWeakRef<S> as() const;

    T* get();
    T& operator*();
    T* operator->();
    const T* get() const;
    const T& operator*() const;
    const T* operator->() const;

    template <Related<T> S>
    bool operator==(const ObjectWeakRef<S>& other) const;
    template <Related<T> S>
    bool operator==(const ObjectTrackedRef<S>& other) const;

    operator bool() const;
};

template <typename T>
ObjectWeakRef<T>::ObjectWeakRef(const ObjectWeakRef& other)
    : obj(other.obj)
{
}

template <typename T>
ObjectWeakRef<T>::ObjectWeakRef(ObjectWeakRef&& other)
    : obj(other.obj)
{
}

template <typename T>
template <ObjectRefRelatedType<T> S>
ObjectWeakRef<T>::ObjectWeakRef(S&& other)
    : obj(other.obj)
{
}

template <typename T>
template <Related<T> S>
ObjectWeakRef<T>::ObjectWeakRef(S* _obj)
    : obj(_obj)
{
}

template <typename T>
ObjectWeakRef<T>& ObjectWeakRef<T>::operator=(const ObjectWeakRef& other)
{
    this->obj = other.obj;
    return *this;
}

template <typename T>
ObjectWeakRef<T>& ObjectWeakRef<T>::operator=(ObjectWeakRef&& other)
{
    this->obj = other.obj;
    return *this;
}

template <typename T>
template <ObjectRefRelatedType<T> S>
ObjectWeakRef<T>& ObjectWeakRef<T>::operator=(S&& other)
{
    this->obj = other.obj;
    return *this;
}

template <typename T>
template <Related<T> S>
ObjectWeakRef<T>& ObjectWeakRef<T>::operator=(S* _obj)
{
    this->obj = _obj;
    return *this;
}

template <typename T>
template <Related<T> S>
ObjectWeakRef<S> ObjectWeakRef<T>::as() const
{
    return ObjectWeakRef<S>(static_cast<S*>(this->obj));
}

template <typename T>
T* ObjectWeakRef<T>::get()
{
    return static_cast<T*>(this->obj);
}

template <typename T>
T& ObjectWeakRef<T>::operator*()
{
    return *static_cast<T*>(this->obj);
}

template <typename T>
T* ObjectWeakRef<T>::operator->()
{
    return static_cast<T*>(this->obj);
}

template <typename T>
const T* ObjectWeakRef<T>::get() const
{
    return static_cast<T*>(this->obj);
}

template <typename T>
const T& ObjectWeakRef<T>::operator*() const
{
    return *static_cast<T*>(this->obj);
}

template <typename T>
const T* ObjectWeakRef<T>::operator->() const
{
    return static_cast<T*>(this->obj);
}

template <typename T>
template <Related<T> S>
bool ObjectWeakRef<T>::operator==(const ObjectWeakRef<S>& other) const
{
    return this->obj == other.obj;
}

template <typename T>
template <Related<T> S>
bool ObjectWeakRef<T>::operator==(const ObjectTrackedRef<S>& other) const
{
    return this->obj == other.obj;
}

template <typename T>
ObjectWeakRef<T>::operator bool() const
{
    return static_cast<bool>(*static_cast<Object*>(this->obj));
}

// -----------------------------------------------------------------------------

template <typename T>
class ObjectTrackedKeyRef : public ObjectWeakRef<T> {
    friend struct ObjectRefHash;
    friend struct ObjectRefEqual;

public:
    template <typename S>
    using rebind = ObjectTrackedKeyRef<S>;

private:
    Object& owner;

public:
    ObjectTrackedKeyRef(const ObjectTrackedKeyRef& other);
    ObjectTrackedKeyRef(ObjectTrackedKeyRef&& other);
    template <Related<T> S>
    ObjectTrackedKeyRef(Object& owner, const ObjectTrackedKeyRef<S>& other);
    template <Related<T> S>
    ObjectTrackedKeyRef(Object& owner, ObjectTrackedKeyRef<S>&& other);
    template <ObjectRefRelatedType<T> S>
    ObjectTrackedKeyRef(Object& owner, S&& other);
    template <Related<T> S>
    ObjectTrackedKeyRef(Object& owner, S* obj);
    ObjectTrackedKeyRef(std::nullptr_t) = delete;

    ~ObjectTrackedKeyRef();

    ObjectTrackedKeyRef& operator=(const ObjectTrackedKeyRef& other);
    ObjectTrackedKeyRef& operator=(ObjectTrackedKeyRef&& other);
    template <ObjectRefRelatedType<T> S>
    ObjectTrackedKeyRef& operator=(S&& other);
    template <Related<T> S>
    ObjectTrackedKeyRef& operator=(S* obj);
    ObjectTrackedKeyRef& operator=(std::nullptr_t) = delete;

    template <typename S>
    friend std::ostream& operator<<(std::ostream& out, const ObjectTrackedKeyRef<S>& obj);
};

template <typename T>
ObjectTrackedKeyRef<T>::ObjectTrackedKeyRef(const ObjectTrackedKeyRef& other)
    : ObjectWeakRef<T>(other.obj)
    , owner(other.owner)
{
    this->owner.track_reference(&this->obj);
}

template <typename T>
ObjectTrackedKeyRef<T>::ObjectTrackedKeyRef(ObjectTrackedKeyRef&& other)
    : ObjectWeakRef<T>(other.obj)
    , owner(other.owner)
{
    this->owner.track_reference(&this->obj);
}

template <typename T>
template <Related<T> S>
ObjectTrackedKeyRef<T>::ObjectTrackedKeyRef(Object& _owner, const ObjectTrackedKeyRef<S>& other)
    : ObjectWeakRef<T>(other.obj)
    , owner(_owner)
{
    this->owner.track_reference(&this->obj);
}

template <typename T>
template <Related<T> S>
ObjectTrackedKeyRef<T>::ObjectTrackedKeyRef(Object& _owner, ObjectTrackedKeyRef<S>&& other)
    : ObjectWeakRef<T>(other.obj)
    , owner(_owner)
{
    this->owner.track_reference(&this->obj);
    other.obj = nullptr;
}

template <typename T>
template <ObjectRefRelatedType<T> S>
ObjectTrackedKeyRef<T>::ObjectTrackedKeyRef(Object& _owner, S&& other)
    : ObjectWeakRef<T>(other)
    , owner(_owner)
{
    this->owner.track_reference(&this->obj);
}

template <typename T>
template <Related<T> S>
ObjectTrackedKeyRef<T>::ObjectTrackedKeyRef(Object& _owner, S* _obj)
    : ObjectWeakRef<T>(_obj)
    , owner(_owner)
{
    this->owner.track_reference(&this->obj);
}

template <typename T>
ObjectTrackedKeyRef<T>::~ObjectTrackedKeyRef()
{
    if (this->obj)
        this->owner.untrack_reference(&this->obj);
}

template <typename T>
ObjectTrackedKeyRef<T>& ObjectTrackedKeyRef<T>::operator=(const ObjectTrackedKeyRef& other)
{
    this->obj = other.obj;
    return *this;
}

template <typename T>
ObjectTrackedKeyRef<T>& ObjectTrackedKeyRef<T>::operator=(ObjectTrackedKeyRef&& other)
{
    this->obj = other.obj;
    return *this;
}

template <typename T>
template <ObjectRefRelatedType<T> S>
ObjectTrackedKeyRef<T>& ObjectTrackedKeyRef<T>::operator=(S&& other)
{
    this->obj = other.obj;
    return *this;
}

template <typename T>
template <Related<T> S>
ObjectTrackedKeyRef<T>& ObjectTrackedKeyRef<T>::operator=(S* _obj)
{
    this->obj = _obj;
    return *this;
}

// --------------------------------------------------------------------------------

template <typename T>
class ObjectTrackedRef : public ObjectTrackedKeyRef<T> {
    template <typename S>
    friend class ObjectWeakRef;
    template <typename S>
    friend class ObjectTrackedRef;
    template <typename S>
    friend class ObjectRef;
    friend struct ObjectRefHash;
    friend struct ObjectRefEqual;

public:
    template <typename S>
    using rebind = ObjectTrackedRef<S>;

public:
    template <ObjectRefRelatedType<T> S>
    ObjectTrackedRef(Object& owner, S&& other);
    template <Related<T> S>
    ObjectTrackedRef(Object& owner, S* obj);
    ObjectTrackedRef(std::nullptr_t) = delete;

    ObjectTrackedRef& operator=(const ObjectTrackedRef& other);
    ObjectTrackedRef& operator=(ObjectTrackedRef&& other);
    template <ObjectRefRelatedType<T> S>
    ObjectTrackedRef& operator=(S&& other);
    template <Related<T> S>
    ObjectTrackedRef& operator=(S* obj);
    ObjectTrackedRef& operator=(std::nullptr_t) = delete;

    template <typename S>
    friend std::ostream& operator<<(std::ostream& out, const ObjectTrackedRef<S>& obj);
};

template <typename T>
template <ObjectRefRelatedType<T> S>
ObjectTrackedRef<T>::ObjectTrackedRef(Object& _owner, S&& other)
    : ObjectTrackedKeyRef<T>(_owner, std::forward<S>(other))
{
}

template <typename T>
template <Related<T> S>
ObjectTrackedRef<T>::ObjectTrackedRef(Object& _owner, S* _obj)
    : ObjectTrackedKeyRef<T>(_owner, _obj)
{
}

template <typename T>
ObjectTrackedRef<T>& ObjectTrackedRef<T>::operator=(const ObjectTrackedRef& other)
{
    this->obj = other.obj;
    return *this;
}

template <typename T>
ObjectTrackedRef<T>& ObjectTrackedRef<T>::operator=(ObjectTrackedRef&& other)
{
    this->obj = other.obj;
    return *this;
}

template <typename T>
template <ObjectRefRelatedType<T> S>
ObjectTrackedRef<T>& ObjectTrackedRef<T>::operator=(S&& other)
{
    this->obj = other.obj;
    return *this;
}

template <typename T>
template <Related<T> S>
ObjectTrackedRef<T>& ObjectTrackedRef<T>::operator=(S* _obj)
{
    this->obj = _obj;
    return *this;
}

// -----------------------------------------------------------------------------

template <typename T>
class ObjectRef : public ObjectWeakRef<T> {
    template <typename S>
    friend class ObjectTrackedRef;
    template <typename S>
    friend class ObjectWeakRef;
    template <typename S>
    friend class ObjectRef;
    friend struct ObjectRefHash;
    friend struct ObjectRefEqual;

public:
    template <typename S>
    using rebind = ObjectRef<S>;

public:
    ObjectRef(const ObjectRef& other);
    ObjectRef(ObjectRef&& other);
    template <ObjectRefRelatedType<T> S>
    ObjectRef(S&& other);
    template <Related<T> S>
    ObjectRef(S* obj);
    ObjectRef(std::nullptr_t) = delete;

    ~ObjectRef();

    ObjectRef& operator=(const ObjectRef& other);
    ObjectRef& operator=(ObjectRef&& other);
    template <ObjectRefRelatedType<T> S>
    ObjectRef& operator=(S&& other);
    template <Related<T> S>
    ObjectRef& operator=(S* obj);
    ObjectRef& operator=(std::nullptr_t) = delete;

    template <typename S>
    friend std::ostream& operator<<(std::ostream& out, const ObjectRef<S>& obj);
};

template <typename T>
ObjectRef<T>::ObjectRef(const ObjectRef& other)
    : ObjectWeakRef<T>(other)
{
    Object::protect_object(&this->obj);
}

template <typename T>
ObjectRef<T>::ObjectRef(ObjectRef&& other)
    : ObjectWeakRef<T>(other)
{
    Object::protect_object(this->alma, &this->obj);
}

template <typename T>
template <ObjectRefRelatedType<T> S>
ObjectRef<T>::ObjectRef(S&& other)
    : ObjectWeakRef<T>(std::forward<S>(other))
{
    Object::protect_object(&this->obj);
}

template <typename T>
template <Related<T> S>
ObjectRef<T>::ObjectRef(S* _obj)
    : ObjectWeakRef<T>(_obj)
{
    Object::protect_object(&this->obj);
}

template <typename T>
ObjectRef<T>::~ObjectRef()
{
    Object::unprotect_object(&this->obj);
}

template <typename T>
ObjectRef<T>& ObjectRef<T>::operator=(const ObjectRef& other)
{
    this->obj = other.obj;
    return *this;
}

template <typename T>
ObjectRef<T>& ObjectRef<T>::operator=(ObjectRef&& other)
{
    this->obj = other.obj;
    return *this;
}

template <typename T>
template <ObjectRefRelatedType<T> S>
ObjectRef<T>& ObjectRef<T>::operator=(S&& other)
{
    this->obj = other.obj;
    return *this;
}

template <typename T>
template <Related<T> S>
ObjectRef<T>& ObjectRef<T>::operator=(S* _obj)
{
    this->obj = _obj;
    return *this;
}

template <typename S>
std::ostream& operator<<(std::ostream& out, const ObjectRef<S>& obj)
{
    ObjectRef<Object> self(obj);
    out << self->to_string(self);
    return out;
}

// Defined here because they need the complete definition of ObjectRef

template <typename S>
std::ostream& operator<<(std::ostream& out, const ObjectTrackedKeyRef<S>& obj)
{
    ObjectRef<Object> self(obj);
    out << self->to_string(self);
    return out;
}

template <typename S>
std::ostream& operator<<(std::ostream& out, const ObjectTrackedRef<S>& obj)
{
    ObjectRef<Object> self(obj);
    out << self->to_string(self);
    return out;
}

// --------------------------------------------------------------------------------

// Make ObjectRef and ObjectWeakRef hashable and transparent to be usable in std::unordered_map

struct ObjectRefHash {
    using is_transparent = void;

    template <ObjectRefType T>
    std::size_t operator()(const T& obj) const noexcept
    {
        return std::hash<GCObject*>()(obj.obj);
    }

    template <typename T>
    std::size_t operator()(const ObjectTrackedKeyRef<T>& obj) const noexcept
    {
        return std::hash<GCObject*>()(obj.obj);
    }
};

struct ObjectRefEqual {
    using is_transparent = void;

    template <ObjectRefType T, ObjectRefType S>
    bool operator()(const T& obj1, const S& obj2) const noexcept
    {
        return obj1.obj == obj2.obj;
    }

    template <typename T, ObjectRefType S>
    bool operator()(const ObjectTrackedKeyRef<T>& obj1, const S& obj2) const noexcept
    {
        return obj1.obj == obj2.obj;
    }

    template <ObjectRefType T, typename S>
    bool operator()(const T& obj1, const ObjectTrackedKeyRef<S>& obj2) const noexcept
    {
        return obj1.obj == obj2.obj;
    }

    template <typename T, typename S>
    bool operator()(const ObjectTrackedKeyRef<T>& obj1, const ObjectTrackedKeyRef<S>& obj2) const noexcept
    {
        return obj1.obj == obj2.obj;
    }
};
