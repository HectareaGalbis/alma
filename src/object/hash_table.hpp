#pragma once

#include "object.hpp"
#include <optional>
#include <unordered_map>
#include <vector>

/** Equality predicates of alma */
bool alma_eql(ObjectRef<Object> a, ObjectRef<Object> b);
bool alma_equal(ObjectRef<Object> a, ObjectRef<Object> b);
bool alma_equalp(ObjectRef<Object> a, ObjectRef<Object> b);

/** A table from keys to values. Two keys are the same key when its test says so:
    eq, eql, equal, equalp or a function given by the user together with a hash function */
class HashTable : public Object {
public:
    enum class Test { Eq,
        Eql,
        Equal,
        Equalp,
        Custom };

private:
    struct Hash {
        using is_transparent = void;
        HashTable* table;
        template <typename K>
        size_t operator()(const K& key) const
        {
            return table->hash(ObjectRef<Object>(key));
        }
    };

    struct Equal {
        using is_transparent = void;
        HashTable* table;
        template <typename A, typename B>
        bool operator()(const A& a, const B& b) const
        {
            return table->equal(ObjectRef<Object>(a), ObjectRef<Object>(b));
        }
    };

    Test test;
    std::optional<ObjectTrackedRef<Object>> test_function;
    std::optional<ObjectTrackedRef<Object>> hash_function;
    std::unordered_map<ObjectTrackedKeyRef<Object>, ObjectTrackedRef<Object>, Hash, Equal> entries;

public:
    HashTable(Test test, const std::optional<ObjectRef<Object>>& test_function,
        const std::optional<ObjectRef<Object>>& hash_function, size_t size);

    size_t hash(ObjectRef<Object> key);
    bool equal(ObjectRef<Object> a, ObjectRef<Object> b);

    Test get_test() const;
    std::optional<ObjectRef<Object>> get_test_function();

    std::optional<ObjectRef<Object>> get(ObjectRef<Object> key);
    void put(ObjectRef<Object> key, ObjectRef<Object> value);
    bool remove(ObjectRef<Object> key);
    void clear();
    size_t count() const;
    std::vector<std::pair<ObjectRef<Object>, ObjectRef<Object>>> to_vector();

    virtual std::string to_string(ObjectRef<Object> self) override;
    virtual ObjectRef<Object> type(ObjectRef<Object> self) const override;
};
