
#include "gc.hpp"
#include <stdexcept>

// --------------------------------------------------------------------------------

GarbageCollector::GarbageCollector()
    : max_objects(0)
{
}

GarbageCollector::~GarbageCollector()
{
    this->collect();
}

void GarbageCollector::save_object(GCObject& obj)
{
    if (!this->object_pool.contains(&obj))
        throw std::runtime_error("Trying to save an object not being in the pool.");
    if (this->safe_pool.contains(&obj))
        throw std::runtime_error("Trying to save an object already safe.");
    this->object_pool.erase(&obj);
    this->safe_pool.insert(&obj);
}

bool GarbageCollector::is_object_safe(GCObject& obj) const
{
    return this->safe_pool.contains(&obj);
}

void GarbageCollector::mark_object_recursively(GCObject& obj)
{
    if (this->is_object_safe(obj))
        return;
    this->save_object(obj);
    for (GCObject** ref : obj.get_references())
        this->mark_object_recursively(**ref);
}

void GarbageCollector::mark()
{
    for (GCObject** ref : this->root_objects)
        this->mark_object_recursively(**ref);
}

void GarbageCollector::sweep()
{
    for (GCObject* obj : this->object_pool)
        delete obj;
    this->object_pool.clear();
}

void GarbageCollector::swap_pools()
{
    std::swap(this->object_pool, this->safe_pool);
}

void GarbageCollector::track_root_object(GCObject** ref)
{
    // debugf("Rooting " << *ref);
    this->root_objects.insert(ref);
}

void GarbageCollector::untrack_root_object(GCObject** ref)
{
    if (!this->root_objects.contains(ref))
        throw std::runtime_error("Trying to untrack a non-tracked root object");
    // debugf("Unrooting " << *ref);
    this->root_objects.erase(ref);
}

void GarbageCollector::collect()
{
    if (this->root_objects.empty()) {
        this->sweep();
    } else {
        this->mark();
        this->sweep();
        this->swap_pools();
    }
}

void GarbageCollector::try_collect()
{
    if (this->object_pool.size() > this->max_objects) {
        this->collect();
        if (this->object_pool.size() > this->max_objects)
            this->max_objects *= 2;
        else if (this->max_objects > 1024 && this->object_pool.size() * 2 < this->max_objects)
            this->max_objects /= 2;
    }
}

size_t GarbageCollector::size() const
{
    return this->root_objects.size() + this->object_pool.size() + this->safe_pool.size();
}

// --------------------------------------------------------------------------------

std::unordered_set<GCObject**>& GCObject::get_references()
{
    return this->references;
}

bool GCObject::is_reference_tracked(GCObject** ref) const
{
    return this->references.contains(ref);
}
void GCObject::track_reference(GCObject** ref)
{
    if (this->is_reference_tracked(ref))
        throw std::runtime_error("Trying to track an already tracked reference.");
    // debugf("Tracking " << *ref);
    this->references.insert(ref);
}

void GCObject::untrack_reference(GCObject** ref)
{
    if (!this->is_reference_tracked(ref))
        throw std::runtime_error("Trying to untrack an untracked reference.");
    // debugf("Untracking " << *ref);
    this->references.erase(ref);
}
