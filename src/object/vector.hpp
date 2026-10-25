#pragma once

#include "interfaces/applicable.hpp"
#include "object.hpp"
#include <deque>
#include <vector>

/** A sequence of objects with fast access by index and fast insertion at both ends.
    Applying a vector to an index returns its element: (v 0) */
class Vector : public Object, public Applicable {
private:
    std::deque<ObjectTrackedKeyRef<Object>> elements;

    void check_index(size_t index, size_t size) const;

public:
    Vector(const std::vector<ObjectRef<Object>>& elements);

    size_t size() const;
    ObjectRef<Object> get(size_t index);
    void set(size_t index, ObjectRef<Object> value);

    ObjectRef<Object> front();
    ObjectRef<Object> back();
    void push_front(ObjectRef<Object> value);
    void push_back(ObjectRef<Object> value);
    ObjectRef<Object> pop_front();
    ObjectRef<Object> pop_back();

    /** Insert before the position index, that can be the size of the vector to insert at the end */
    void insert(size_t index, ObjectRef<Object> value);
    ObjectRef<Object> erase(size_t index);
    void clear();

    std::vector<ObjectRef<Object>> to_vector();

    virtual ObjectRef<Object> apply(ObjectRef<Object> self, const std::vector<ObjectRef<Object>>& arg_list,
        ObjectRef<Environment> environment) override;
    virtual std::string to_string(ObjectRef<Object> self) override;
    virtual ObjectRef<Object> type(ObjectRef<Object> self) const override;
};
