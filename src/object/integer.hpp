
#pragma once

#include "interfaces/emittable.hpp"
#include "object.hpp"

// Integer
class Integer : public Object, public Emittable {
private:
    int64_t value;

public:
    Integer(int64_t _value);

    virtual std::string to_string(ObjectRef<Object> self) override;
    virtual ObjectRef<Object> type(ObjectRef<Object> self) const override;

    virtual void emit_top_level(ObjectRef<Object> self, ObjectRef<Environment> environment,
        Declarations& declarations, std::stringstream& output) override;
    virtual void emit_statement(ObjectRef<Object> self, ObjectRef<Environment> environment,
        Declarations& declarations, std::stringstream& output) override;
    virtual std::unique_ptr<Type> emit_expression(ObjectRef<Object> self, ObjectRef<Environment> environment,
        Declarations& declarations, std::stringstream& output) override;

    int64_t get_value() const;
};
