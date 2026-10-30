
#pragma once

#include "object.hpp"
#include <optional>
#include <string>

class Character : public Object {
private:
    char c;

public:
    Character(char c);

    char get_char() const;

    /** The characters with a name, like space or newline */
    static std::optional<char> from_name(const std::string& name);
    static std::optional<std::string> name_of(char c);

    virtual std::string to_string(ObjectRef<Object> self) override;
    virtual ObjectRef<Object> type(ObjectRef<Object> self) const override;
};
