
#include "gc.hpp"

// --------------------------------------------------------------------------------

GarbageCollector::GarbageCollector()
    : threshold(min_threshold)
    , root_count(0)
    , roots(nullptr)
{
}

GarbageCollector::~GarbageCollector()
{
    this->collect();
}

void GarbageCollector::mark_object(GCObject* obj)
{
    if (obj && !obj->marked) {
        obj->marked = true;
        this->mark_stack.push_back(obj);
    }
}

void GarbageCollector::mark()
{
    for (GCNode* ref = this->roots; ref; ref = ref->next)
        this->mark_object(*ref->slot);
    while (!this->mark_stack.empty()) {
        GCObject* obj = this->mark_stack.back();
        this->mark_stack.pop_back();
        for (GCNode* ref = obj->references; ref; ref = ref->next)
            this->mark_object(*ref->slot);
    }
}

void GarbageCollector::sweep()
{
    std::size_t alive = 0;
    for (std::size_t i = 0; i < this->objects.size(); i++) {
        GCObject* obj = this->objects[i];
        if (obj->marked) {
            obj->marked = false;
            this->objects[alive++] = obj;
        } else {
            delete obj;
        }
    }
    this->objects.resize(alive);
}

void GarbageCollector::collect()
{
    this->mark();
    this->sweep();
}

std::size_t GarbageCollector::size() const
{
    return this->root_count + this->objects.size();
}
