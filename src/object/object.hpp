
#pragma once

#include "gc/gc.hpp"
#include <string>

namespace ALMA::core {

class Alma;
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
//     virtual bool typep(ObjectRef<Object> self, ObjectRef<Object> type);
//     operator bool();
// };

class Object : public GCObject {
    template <typename S>
    friend class ObjectRef;

private:
    static void protect_object(Alma& alma, GCObject** object);
    static void unprotect_object(Alma& alma, GCObject** object);

public:
    virtual ObjectRef<Object> eval(ObjectRef<Object> self);
    virtual bool is_true();
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
    Alma& alma;

protected:
    ObjectWeakRef(const ObjectWeakRef& other);
    ObjectWeakRef(ObjectWeakRef&& other);
    template <ObjectRefRelatedType<T> S>
    ObjectWeakRef(S&& other);
    template <Related<T> S>
    ObjectWeakRef(Alma& alma, S* obj);
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
    , alma(other.alma)
{
}

template <typename T>
ObjectWeakRef<T>::ObjectWeakRef(ObjectWeakRef&& other)
    : obj(other.obj)
    , alma(other.alma)
{
}

template <typename T>
template <ObjectRefRelatedType<T> S>
ObjectWeakRef<T>::ObjectWeakRef(S&& other)
    : obj(other.obj)
    , alma(other.alma)
{
}

template <typename T>
template <Related<T> S>
ObjectWeakRef<T>::ObjectWeakRef(Alma& _alma, S* _obj)
    : obj(_obj)
    , alma(_alma)
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
    return ObjectWeakRef<S>(this->alma, static_cast<S*>(this->obj));
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
    ObjectTrackedKeyRef(Object& owner, Alma& alma, S* obj);
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
    : ObjectWeakRef<T>(other.alma, other.obj)
    , owner(other.owner)
{
    this->owner.track_reference(&this->obj);
}

template <typename T>
ObjectTrackedKeyRef<T>::ObjectTrackedKeyRef(ObjectTrackedKeyRef&& other)
    : ObjectWeakRef<T>(other.alma, other.obj)
    , owner(other.owner)
{
    this->owner.track_reference(&this->obj);
}

template <typename T>
template <Related<T> S>
ObjectTrackedKeyRef<T>::ObjectTrackedKeyRef(Object& _owner, const ObjectTrackedKeyRef<S>& other)
    : ObjectWeakRef<T>(other.alma, other.obj)
    , owner(_owner)
{
    this->owner.track_reference(&this->obj);
}

template <typename T>
template <Related<T> S>
ObjectTrackedKeyRef<T>::ObjectTrackedKeyRef(Object& _owner, ObjectTrackedKeyRef<S>&& other)
    : ObjectWeakRef<T>(other.alma, other.obj)
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
ObjectTrackedKeyRef<T>::ObjectTrackedKeyRef(Object& _owner, Alma& _alma, S* _obj)
    : ObjectWeakRef<T>(_alma, _obj)
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

template <typename S>
std::ostream& operator<<(std::ostream& out, const ObjectTrackedKeyRef<S>& obj)
{
    out << obj.alma.to_string(obj);
    return out;
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
    ObjectTrackedRef(Object& owner, Alma& alma, S* obj);
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
ObjectTrackedRef<T>::ObjectTrackedRef(Object& _owner, Alma& _alma, S* _obj)
    : ObjectTrackedKeyRef<T>(_owner, _alma, _obj)
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

template <typename S>
std::ostream& operator<<(std::ostream& out, const ObjectTrackedRef<S>& obj)
{
    out << obj.alma.to_string(obj);
    return out;
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
    ObjectRef(Alma& alma, S* obj);
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
    Object::protect_object(this->alma, &this->obj);
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
    Object::protect_object(this->alma, &this->obj);
}

template <typename T>
template <Related<T> S>
ObjectRef<T>::ObjectRef(Alma& _alma, S* _obj)
    : ObjectWeakRef<T>(_alma, _obj)
{
    Object::protect_object(this->alma, &this->obj);
}

template <typename T>
ObjectRef<T>::~ObjectRef()
{
    Object::unprotect_object(this->alma, &this->obj);
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
    out << obj.alma.to_string(obj);
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

}
