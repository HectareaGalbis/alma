#include "hash_table.hpp"
#include "alma.hpp"
#include "character.hpp"
#include "cons.hpp"
#include "debug.hpp"
#include "integer.hpp"
#include "string.hpp"
#include "vector.hpp"
#include <cctype>
#include <functional>

// -----------------------------------------------------------------------------
// Equality

static bool is(ObjectRef<Object> obj, const char* type)
{
    return Alma::alma.type(obj) == Alma::alma.intern_alma_symbol(type);
}

static std::string lowercase(const std::string& text)
{
    std::string result = text;
    for (char& c : result)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return result;
}

static char lowercase(char c)
{
    return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
}

bool alma_eql(ObjectRef<Object> a, ObjectRef<Object> b)
{
    if (a == b)
        return true;
    if (is(a, "integer") && is(b, "integer"))
        return a.as<Integer>()->get_value() == b.as<Integer>()->get_value();
    if (is(a, "character") && is(b, "character"))
        return a.as<Character>()->get_char() == b.as<Character>()->get_char();
    return false;
}

bool alma_equal(ObjectRef<Object> a, ObjectRef<Object> b)
{
    Alma::alma.check_stack();
    while (Alma::alma.consp(a) && Alma::alma.consp(b)) {
        if (!alma_equal(a.as<Cons>()->get_car(), b.as<Cons>()->get_car()))
            return false;
        a = a.as<Cons>()->get_cdr();
        b = b.as<Cons>()->get_cdr();
    }
    if (is(a, "string") && is(b, "string"))
        return a.as<String>()->get_content() == b.as<String>()->get_content();
    return alma_eql(a, b);
}

bool alma_equalp(ObjectRef<Object> a, ObjectRef<Object> b)
{
    Alma::alma.check_stack();
    while (Alma::alma.consp(a) && Alma::alma.consp(b)) {
        if (!alma_equalp(a.as<Cons>()->get_car(), b.as<Cons>()->get_car()))
            return false;
        a = a.as<Cons>()->get_cdr();
        b = b.as<Cons>()->get_cdr();
    }
    if (is(a, "character") && is(b, "character"))
        return lowercase(a.as<Character>()->get_char()) == lowercase(b.as<Character>()->get_char());
    if (is(a, "string") && is(b, "string"))
        return lowercase(a.as<String>()->get_content()) == lowercase(b.as<String>()->get_content());
    if (is(a, "vector") && is(b, "vector")) {
        std::vector<ObjectRef<Object>> va = a.as<Vector>()->to_vector();
        std::vector<ObjectRef<Object>> vb = b.as<Vector>()->to_vector();
        if (va.size() != vb.size())
            return false;
        for (size_t i = 0; i < va.size(); i++)
            if (!alma_equalp(va[i], vb[i]))
                return false;
        return true;
    }
    if (is(a, "hash-table") && is(b, "hash-table")) {
        ObjectRef<HashTable> ha = a;
        ObjectRef<HashTable> hb = b;
        if (ha->count() != hb->count() || ha->get_test() != hb->get_test())
            return false;
        for (auto& [key, value] : ha->to_vector()) {
            std::optional<ObjectRef<Object>> other = hb->get(key);
            if (!other || !alma_equalp(value, *other))
                return false;
        }
        return true;
    }
    return alma_eql(a, b);
}

// -----------------------------------------------------------------------------
// Hashing

static const int max_hash_depth = 4;
static const int max_hash_elements = 8;

static size_t combine(size_t seed, size_t value)
{
    return seed * 31 + value;
}

static size_t hash_eq(ObjectRef<Object> obj)
{
    return std::hash<Object*>()(obj.get());
}

static size_t hash_eql(ObjectRef<Object> obj)
{
    if (is(obj, "integer"))
        return std::hash<int64_t>()(obj.as<Integer>()->get_value());
    if (is(obj, "character"))
        return std::hash<char>()(obj.as<Character>()->get_char());
    return hash_eq(obj);
}

static size_t hash_equal(ObjectRef<Object> obj, int depth)
{
    if (Alma::alma.consp(obj)) {
        size_t seed = 1;
        for (int i = 0; i < max_hash_elements && depth < max_hash_depth && Alma::alma.consp(obj); i++) {
            seed = combine(seed, hash_equal(obj.as<Cons>()->get_car(), depth + 1));
            obj = obj.as<Cons>()->get_cdr();
        }
        return seed;
    }
    if (is(obj, "string"))
        return std::hash<std::string>()(obj.as<String>()->get_content());
    return hash_eql(obj);
}

static size_t hash_equalp(ObjectRef<Object> obj, int depth)
{
    if (Alma::alma.consp(obj)) {
        size_t seed = 1;
        for (int i = 0; i < max_hash_elements && depth < max_hash_depth && Alma::alma.consp(obj); i++) {
            seed = combine(seed, hash_equalp(obj.as<Cons>()->get_car(), depth + 1));
            obj = obj.as<Cons>()->get_cdr();
        }
        return seed;
    }
    if (is(obj, "character"))
        return std::hash<char>()(lowercase(obj.as<Character>()->get_char()));
    if (is(obj, "string"))
        return std::hash<std::string>()(lowercase(obj.as<String>()->get_content()));
    if (is(obj, "vector")) {
        std::vector<ObjectRef<Object>> elements = obj.as<Vector>()->to_vector();
        size_t seed = combine(2, elements.size());
        for (size_t i = 0; i < elements.size() && i < max_hash_elements && depth < max_hash_depth; i++)
            seed = combine(seed, hash_equalp(elements[i], depth + 1));
        return seed;
    }
    if (is(obj, "hash-table"))
        return combine(3, obj.as<HashTable>()->count());
    return hash_eql(obj);
}

// -----------------------------------------------------------------------------
// HashTable

HashTable::HashTable(Test _test, const std::optional<ObjectRef<Object>>& _test_function,
    const std::optional<ObjectRef<Object>>& _hash_function, size_t size)
    : test(_test)
    , entries(size, Hash { this }, Equal { this })
{
    if (_test_function)
        this->test_function.emplace(*this, *_test_function);
    if (_hash_function)
        this->hash_function.emplace(*this, *_hash_function);
}

size_t HashTable::hash(ObjectRef<Object> key)
{
    switch (this->test) {
    case Test::Eq:
        return hash_eq(key);
    case Test::Eql:
        return hash_eql(key);
    case Test::Equal:
        return hash_equal(key, 0);
    case Test::Equalp:
        return hash_equalp(key, 0);
    case Test::Custom:
        break;
    }
    ObjectRef<Object> result = Alma::alma.funcall(*this->hash_function, { key });
    aassert(is(result, "integer"),
        "The hash function must return an integer but returned " << Alma::alma.to_string(result));
    return std::hash<int64_t>()(result.as<Integer>()->get_value());
}

bool HashTable::equal(ObjectRef<Object> a, ObjectRef<Object> b)
{
    switch (this->test) {
    case Test::Eq:
        return a == b;
    case Test::Eql:
        return alma_eql(a, b);
    case Test::Equal:
        return alma_equal(a, b);
    case Test::Equalp:
        return alma_equalp(a, b);
    case Test::Custom:
        break;
    }
    return Alma::alma.truep(Alma::alma.funcall(*this->test_function, { a, b }));
}

HashTable::Test HashTable::get_test() const
{
    return this->test;
}

std::optional<ObjectRef<Object>> HashTable::get_test_function()
{
    if (!this->test_function)
        return std::nullopt;
    return ObjectRef<Object>(*this->test_function);
}

std::optional<ObjectRef<Object>> HashTable::get(ObjectRef<Object> key)
{
    auto it = this->entries.find(key);
    if (it == this->entries.end())
        return std::nullopt;
    return ObjectRef<Object>(it->second);
}

void HashTable::put(ObjectRef<Object> key, ObjectRef<Object> value)
{
    auto it = this->entries.find(key);
    if (it != this->entries.end())
        it->second = value;
    else
        this->entries.try_emplace(ObjectTrackedKeyRef<Object>(*this, key), *this, value);
}

bool HashTable::remove(ObjectRef<Object> key)
{
    auto it = this->entries.find(key);
    if (it == this->entries.end())
        return false;
    this->entries.erase(it);
    return true;
}

void HashTable::clear()
{
    this->entries.clear();
}

size_t HashTable::count() const
{
    return this->entries.size();
}

std::vector<std::pair<ObjectRef<Object>, ObjectRef<Object>>> HashTable::to_vector()
{
    std::vector<std::pair<ObjectRef<Object>, ObjectRef<Object>>> result;
    for (auto& [key, value] : this->entries)
        result.emplace_back(key, value);
    return result;
}

static const char* test_name(HashTable::Test test)
{
    switch (test) {
    case HashTable::Test::Eq:
        return "eq";
    case HashTable::Test::Eql:
        return "eql";
    case HashTable::Test::Equal:
        return "equal";
    case HashTable::Test::Equalp:
        return "equalp";
    case HashTable::Test::Custom:
        break;
    }
    return "custom";
}

std::string HashTable::to_string(ObjectRef<Object> self [[maybe_unused]])
{
    return "<hash-table " + std::string(test_name(this->test)) + " " + std::to_string(this->count()) + ">";
}

ObjectRef<Object> HashTable::type(ObjectRef<Object> self [[maybe_unused]]) const
{
    return Alma::alma.intern_alma_symbol("hash-table");
}
