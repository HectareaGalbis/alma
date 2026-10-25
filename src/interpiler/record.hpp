
#pragma once

#include "type.hpp"
#include <unordered_map>
#include <utility>

class Record {
public:
    using Members = std::vector<std::pair<std::string, std::unique_ptr<Type>>>;

private:
    std::string name;
    bool unionRecord;
    Members members;
    bool defined;

public:
    Record(const std::string& name, bool unionRecord, const Members& members, bool defined = false);
    Record(const Record& other);
    Record& operator=(const Record& other);
    Record(Record&& other) noexcept;
    Record& operator=(Record&& other) noexcept;

    const std::string& getName() const;
    bool isUnion() const;
    const Members& getMembers() const;
    bool isDefined() const;

    void define();

    bool operator==(const Record& other) const;
};

class Records {
private:
    std::unordered_map<std::string, Record> records;

public:
    void add(const Record& record);
    bool contains(const std::string& name) const;
    const Record& get(const std::string& name) const;
    /** Names of all the structs and unions, sorted */
    std::vector<std::string> names() const;

    template <typename... Args>
    void emplace(Args&&... args)
    {
        Record record(std::forward<Args>(args)...);
        if (!contains(record.getName())) {
            std::string name = record.getName();
            records.emplace(std::move(name), std::move(record));
            return;
        }
        add(record);
    }
};
