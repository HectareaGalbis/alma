
#pragma once

#include "object.hpp"
#include "symbol.hpp"
#include <optional>
#include <vector>

class Environment;

// -----------------------------------------------------------------------------

class Procedure : public Object {
protected:
    virtual ObjectRef<Object> eval_body(
        const std::vector<ObjectRef<Object>>& arg_list,
        ObjectRef<Environment> enviroment)
        = 0;

    void check_types(
        const std::vector<ObjectRef<Object>>& arg_list,
        const std::vector<std::string>& alma_types);
    void check_types(
        const std::vector<ObjectRef<Object>>& arg_list,
        const std::vector<std::string>& alma_types,
        const std::string& alma_rest_type);

public:
    Procedure(Alma& alma);

    virtual bool typep(ObjectRef<Object> self, ObjectRef<Object> type) override;
    virtual ObjectRef<Object> apply(
        ObjectRef<Object> self,
        const std::vector<ObjectRef<Object>>& arg_list,
        ObjectRef<Environment> enviroment) override;
};

// -----------------------------------------------------------------------------

class Function : public Procedure {
public:
    Function(Alma& alma);
    virtual bool typep(ObjectRef<Object> self, ObjectRef<Object> type) override;
    virtual ObjectRef<Object> apply(
        ObjectRef<Object> self,
        const std::vector<ObjectRef<Object>>& arg_list,
        ObjectRef<Environment> enviroment) override;
};

// -----------------------------------------------------------------------------

class Macro : public Procedure {
public:
    Macro(Alma& alma);
    virtual ObjectRef<Object> transform(
        ObjectRef<Object> self,
        const std::vector<ObjectRef<Object>>& arg_list,
        ObjectRef<Environment> enviroment) override;
    virtual ObjectRef<Object> apply(
        ObjectRef<Object> self,
        const std::vector<ObjectRef<Object>>& arg_list,
        ObjectRef<Environment> enviroment) override;
    virtual bool typep(ObjectRef<Object> self, ObjectRef<Object> type) override;
};

// -----------------------------------------------------------------------------

class FunctionUser : public Function {
private:
    std::vector<ObjectTrackedKeyRef<Symbol>> param_list;
    std::optional<ObjectTrackedRef<Symbol>> param_rest;
    mutable ObjectTrackedRef<Environment> closure;
    std::vector<ObjectTrackedKeyRef<Object>> body;

protected:
    virtual ObjectRef<Object> eval_body(
        const std::vector<ObjectRef<Object>>& arg_list,
        ObjectRef<Environment> enviroment) override;

public:
    FunctionUser(
        Alma& alma,
        const std::vector<ObjectRef<Object>>& param_list,
        const std::optional<ObjectRef<Object>>& param_rest,
        ObjectRef<Environment> closure,
        const std::vector<ObjectRef<Object>>& body);

    virtual bool typep(ObjectRef<Object> self, ObjectRef<Object> type) override;
};

// -----------------------------------------------------------------------------

class MacroUser : public Macro {
private:
    std::vector<ObjectTrackedKeyRef<Symbol>> param_list;
    std::optional<ObjectTrackedRef<Symbol>> param_rest;
    mutable ObjectTrackedRef<Environment> closure;
    std::vector<ObjectTrackedKeyRef<Object>> body;

protected:
    virtual ObjectRef<Object> eval_body(
        const std::vector<ObjectRef<Object>>& arg_list,
        ObjectRef<Environment> enviroment) override;

public:
    MacroUser(
        Alma& alma,
        const std::vector<ObjectRef<Object>>& param_list,
        const std::optional<ObjectRef<Object>>& param_rest,
        ObjectRef<Environment> closure,
        const std::vector<ObjectRef<Object>>& body);

    virtual bool typep(ObjectRef<Object> self, ObjectRef<Object> type) override;
};

// -----------------------------------------------------------------------------
