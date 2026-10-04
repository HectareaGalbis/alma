
#pragma once

#include "object.hpp"
#include <string>

class String : public Object {
private:
    std::string content;

public:
    String(const std::string& content);

    virtual std::string to_string(ObjectRef<Object> self) override;
    virtual ObjectRef<Object> type(ObjectRef<Object> self) const override;
};
