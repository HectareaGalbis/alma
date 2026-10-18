
#pragma once

#include "interfaces/emittable.hpp"
#include "object.hpp"
#include <string>

class String : public Object, public Emittable {
private:
    std::string content;

public:
    String(const std::string& content);

    const std::string& get_content() const;

    virtual std::string to_string(ObjectRef<Object> self) override;
    virtual ObjectRef<Object> type(ObjectRef<Object> self) const override;

    virtual void emit_top_level(ObjectRef<Object> self, ObjectRef<Environment> environment,
        Declarations& declarations, std::stringstream& output) override;
    virtual void emit_statement(ObjectRef<Object> self, ObjectRef<Environment> environment,
        Declarations& declarations, std::stringstream& output) override;
    virtual std::unique_ptr<Type> emit_expression(ObjectRef<Object> self, ObjectRef<Environment> environment,
        Declarations& declarations, std::stringstream& output) override;
};
