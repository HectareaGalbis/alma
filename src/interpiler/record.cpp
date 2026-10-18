
#include "record.hpp"
#include "../debug.hpp"

static Record::Members cloneMembers(const Record::Members& members)
{
    Record::Members result;
    result.reserve(members.size());
    for (const auto& [name, type] : members)
        result.emplace_back(name, type->clone());
    return result;
}

Record::Record(const std::string& _name, bool _unionRecord, const Members& _members, bool _defined)
    : name(_name)
    , unionRecord(_unionRecord)
    , members(cloneMembers(_members))
    , defined(_defined)
{
}

Record::Record(const Record& other)
    : name(other.name)
    , unionRecord(other.unionRecord)
    , members(cloneMembers(other.members))
    , defined(other.defined)
{
}

Record& Record::operator=(const Record& other)
{
    if (this != &other) {
        name = other.name;
        unionRecord = other.unionRecord;
        members = cloneMembers(other.members);
        defined = other.defined;
    }
    return *this;
}

Record::Record(Record&& other) noexcept = default;

Record& Record::operator=(Record&& other) noexcept = default;

const std::string& Record::getName() const { return name; }

bool Record::isUnion() const { return unionRecord; }

const Record::Members& Record::getMembers() const { return members; }

bool Record::isDefined() const { return defined; }

void Record::define() { defined = true; }

bool Record::operator==(const Record& other) const
{
    if (name != other.name || unionRecord != other.unionRecord || members.size() != other.members.size())
        return false;
    for (size_t i = 0; i < members.size(); ++i)
        if (members[i].first != other.members[i].first || !(*members[i].second == *other.members[i].second))
            return false;
    return true;
}

void Records::add(const Record& record)
{
    const std::string& name = record.getName();
    auto it = records.find(name);
    if (it == records.end()) {
        records.try_emplace(name, record);
        return;
    }

    if (record.isDefined()) {
        aassert(!it->second.isDefined(), "Redefinition of '" << name << "'");
        it->second = record;
    }
}

bool Records::contains(const std::string& name) const { return records.contains(name); }

const Record& Records::get(const std::string& name) const
{
    auto it = records.find(name);
    aassert(it != records.end(), "Record '" << name << "' not found");
    return it->second;
}
