
#pragma once

#include <algorithm>
#include <cstddef>
#include <type_traits>
#include <utility>
#include <vector>

class GCObject;
class GarbageCollector;
template <typename T>
concept ExtendsGCObject = std::is_base_of_v<GCObject, T>;

class GCNode {
    friend GarbageCollector;

private:
    GCObject* const* slot;
    GCNode* next;
    GCNode** pprev;

public:
    explicit GCNode(GCObject* const* slot);
    GCNode(const GCNode&) = delete;
    GCNode& operator=(const GCNode&) = delete;

    void link(GCNode*& head);
    void unlink();
};

inline GCNode::GCNode(GCObject* const* _slot)
    : slot(_slot)
    , next(nullptr)
    , pprev(nullptr)
{
}

inline void GCNode::link(GCNode*& head)
{
    this->next = head;
    if (head)
        head->pprev = &this->next;
    head = this;
    this->pprev = &head;
}

inline void GCNode::unlink()
{
    *this->pprev = this->next;
    if (this->next)
        this->next->pprev = this->pprev;
}

class GarbageCollector {
private:
    static constexpr std::size_t min_threshold = 1024;

    std::size_t threshold;
    std::size_t root_count;
    GCNode* roots;
    std::vector<GCObject*> objects;
    std::vector<GCObject*> mark_stack;

private:
    void mark_object(GCObject* obj);
    void mark();
    void sweep();

public:
    GarbageCollector();
    ~GarbageCollector();
    template <ExtendsGCObject T, typename... AS>
    T* make_object(AS&&... as);
    void track_root_object(GCNode& ref);
    void untrack_root_object(GCNode& ref);
    void collect();
    void try_collect();
    std::size_t size() const;
};

template <ExtendsGCObject T, typename... AS>
T* GarbageCollector::make_object(AS&&... as)
{
    T* newObject = new T(std::forward<AS>(as)...);
    this->objects.push_back(newObject);
    return newObject;
}

inline void GarbageCollector::track_root_object(GCNode& ref)
{
    ref.link(this->roots);
    this->root_count++;
}

inline void GarbageCollector::untrack_root_object(GCNode& ref)
{
    ref.unlink();
    this->root_count--;
}

inline void GarbageCollector::try_collect()
{
    if (this->objects.size() >= this->threshold) {
        this->collect();
        this->threshold = std::max(min_threshold, this->objects.size() * 2);
    }
}

class GCObject {
    friend GarbageCollector;

private:
    GCNode* references;
    bool marked;

public:
    GCObject();
    GCObject(const GCObject& other);
    GCObject& operator=(const GCObject& other);
    virtual ~GCObject() = default;
    void track_reference(GCNode& ref);
    void untrack_reference(GCNode& ref);
};

inline GCObject::GCObject()
    : references(nullptr)
    , marked(false)
{
}

inline GCObject::GCObject(const GCObject& other [[maybe_unused]])
    : references(nullptr)
    , marked(false)
{
}

inline GCObject& GCObject::operator=(const GCObject& other [[maybe_unused]])
{
    return *this;
}

inline void GCObject::track_reference(GCNode& ref)
{
    ref.link(this->references);
}

inline void GCObject::untrack_reference(GCNode& ref)
{
    ref.unlink();
}
