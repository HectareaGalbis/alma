
#pragma once

#include <unordered_set>

class GCObject;
template <typename T>
concept ExtendsGCObject = std::is_base_of_v<GCObject, T>;

class GarbageCollector {
private:
    size_t max_objects;
    std::unordered_set<GCObject**> root_objects;
    std::unordered_set<GCObject*> object_pool;
    std::unordered_set<GCObject*> safe_pool;

private:
    void save_object(GCObject& obj);
    bool is_object_safe(GCObject& obj) const;

    void mark_object_recursively(GCObject& obj);
    void mark();
    void sweep();
    void swap_pools();

public:
    GarbageCollector();
    ~GarbageCollector();
    template <ExtendsGCObject T, typename... AS>
    T* make_object(AS&&... as);
    void track_root_object(GCObject** ref);
    void untrack_root_object(GCObject** ref);
    void collect();
    void try_collect();
    size_t size() const;
};

template <ExtendsGCObject T, typename... AS>
T* GarbageCollector::make_object(AS&&... as)
{
    GCObject* newObject = new T(std::forward<AS>(as)...);
    this->object_pool.insert(newObject);
    return static_cast<T*>(newObject);
}

class GCObject {
    friend GarbageCollector;

private:
    std::unordered_set<GCObject**> references;

public:
    virtual ~GCObject() = default;
    std::unordered_set<GCObject**>& get_references();
    bool is_reference_tracked(GCObject** ref) const;
    void track_reference(GCObject** ref);
    void untrack_reference(GCObject** ref);
};
