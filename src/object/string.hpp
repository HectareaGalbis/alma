
#pragma once

#include "object.hpp"
#include <string>

class String : public Object {
private:
    std::string content;

public:
    String(Alma& alma, const std::string& content);

    virtual std::string to_string() override;
    virtual bool typep(ObjectRef<Object> self, ObjectRef<Object> type) override;
};
