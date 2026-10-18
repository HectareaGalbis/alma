
#pragma once

#include "interfaces/expandable.hpp"
#include "object.hpp"
#include "procedure.hpp"

class CharacterMacro : public Object, public Expandable {
private:
    ObjectTrackedRef<Procedure> transformer;
    ObjectTrackedRef<Object> obj;

public:
    CharacterMacro(ObjectRef<Procedure> transformer, ObjectRef<Object> obj);

    virtual ObjectRef<Object> expand(ObjectRef<Object> self, ObjectRef<Environment> enviroment) override;
    virtual std::string to_string(ObjectRef<Object> self) override;
    virtual ObjectRef<Object> type(ObjectRef<Object> self) const override;
};
