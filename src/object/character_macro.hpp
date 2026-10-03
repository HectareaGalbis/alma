
#pragma once

#include "object.hpp"
#include "procedure.hpp"

class CharacterMacro : public Object {
private:
    ObjectTrackedRef<Procedure> transformer;
    ObjectTrackedRef<Object> obj;

public:
    CharacterMacro(Alma& alma, ObjectRef<Procedure> transformer, ObjectRef<Object> obj);

    virtual ObjectRef<Object> expand(ObjectRef<Object> self, ObjectRef<Environment> enviroment) override;
    virtual ObjectRef<Object> eval(ObjectRef<Object> self, ObjectRef<Environment> enviroment) override;
    virtual std::string to_string() override;
    virtual bool typep(ObjectRef<Object> self, ObjectRef<Object> type) override;
};
