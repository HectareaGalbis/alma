
#pragma once

#include "interfaces/applicable.hpp"
#include "interfaces/transformable.hpp"
#include "object.hpp"
#include "symbol.hpp"
#include <optional>
#include <vector>

class Environment;

// -----------------------------------------------------------------------------

class Procedure : public Object, public Applicable {
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
    Procedure();

    virtual std::string to_string(ObjectRef<Object> self) override;
    virtual ObjectRef<Object> type(ObjectRef<Object> self) const override;

    virtual ObjectRef<Object> apply(
        ObjectRef<Object> self,
        const std::vector<ObjectRef<Object>>& arg_list,
        ObjectRef<Environment> enviroment) override;
};

// -----------------------------------------------------------------------------

class Function : public Procedure {
public:
    Function();
    virtual std::string to_string(ObjectRef<Object> self) override;
    virtual ObjectRef<Object> type(ObjectRef<Object> self) const override;
    virtual ObjectRef<Object> apply(
        ObjectRef<Object> self,
        const std::vector<ObjectRef<Object>>& arg_list,
        ObjectRef<Environment> enviroment) override;
};

// -----------------------------------------------------------------------------

class Macro : public Procedure, public Transformable {
public:
    Macro();
    virtual std::string to_string(ObjectRef<Object> self) override;
    virtual ObjectRef<Object> type(ObjectRef<Object> self) const override;
    virtual ObjectRef<Object> transform(
        ObjectRef<Object> self,
        const std::vector<ObjectRef<Object>>& arg_list,
        ObjectRef<Environment> enviroment) override;
    virtual ObjectRef<Object> apply(
        ObjectRef<Object> self,
        const std::vector<ObjectRef<Object>>& arg_list,
        ObjectRef<Environment> enviroment) override;
};

// -----------------------------------------------------------------------------

/** The parameters of a user procedure, in this order:

      a                           required
      (b default b-p)             optional, with its default value and if it was received
      :k c   :k (c default c-p)   by keyword

    &rest r and &rest-keys h can be anywhere. After the required and optional arguments, a
    keyword followed by a value is a keyword argument, and the rest of arguments go to &rest.
    &rest-keys is a hash table with the keyword arguments that are not parameters; without it
    they are an error. If there are no keyword parameters nor &rest-keys, the keywords are
    normal arguments. The default values are evaluated in each call, after the previous
    parameters are bound */
class LambdaList {
private:
    struct Parameter {
        Symbol* var;
        Object* default_form;
        Symbol* supplied;
        Symbol* keyword;
    };

    Object& owner;
    std::vector<ObjectTrackedKeyRef<Object>> objects;
    std::vector<Parameter> required;
    std::vector<Parameter> optional;
    std::vector<Parameter> keys;
    Symbol* rest;
    Symbol* rest_keys;

    Symbol* keep_variable(ObjectRef<Object> obj, std::vector<Symbol*>& names);
    Parameter parse_parameter(ObjectRef<Object> obj, bool allow_symbol, std::vector<Symbol*>& names);

public:
    LambdaList(Object& owner, ObjectRef<Object> spec);
    LambdaList(const LambdaList&) = delete;
    LambdaList& operator=(const LambdaList&) = delete;

    /** Bind the arguments in the current layer of the environment */
    void bind(const std::vector<ObjectRef<Object>>& arg_list, ObjectRef<Environment> environment) const;
};

// -----------------------------------------------------------------------------

class FunctionUser : public Function {
private:
    LambdaList lambda_list;
    mutable ObjectTrackedRef<Environment> closure;
    std::vector<ObjectTrackedKeyRef<Object>> body;

protected:
    virtual ObjectRef<Object> eval_body(
        const std::vector<ObjectRef<Object>>& arg_list,
        ObjectRef<Environment> enviroment) override;

public:
    FunctionUser(
        ObjectRef<Object> lambda_list,
        ObjectRef<Environment> closure,
        const std::vector<ObjectRef<Object>>& body);

    virtual std::string to_string(ObjectRef<Object> self) override;
    virtual ObjectRef<Object> type(ObjectRef<Object> self) const override;
};

// -----------------------------------------------------------------------------

class MacroUser : public Macro {
private:
    LambdaList lambda_list;
    mutable ObjectTrackedRef<Environment> closure;
    std::vector<ObjectTrackedKeyRef<Object>> body;

protected:
    virtual ObjectRef<Object> eval_body(
        const std::vector<ObjectRef<Object>>& arg_list,
        ObjectRef<Environment> enviroment) override;

public:
    MacroUser(
        ObjectRef<Object> lambda_list,
        ObjectRef<Environment> closure,
        const std::vector<ObjectRef<Object>>& body);

    virtual std::string to_string(ObjectRef<Object> self) override;
    virtual ObjectRef<Object> type(ObjectRef<Object> self) const override;
};

// -----------------------------------------------------------------------------

/** Like FunctionUser or MacroUser, but it does not evaluate its arguments nor its result */
class SpecialOperatorUser : public Procedure {
private:
    LambdaList lambda_list;
    mutable ObjectTrackedRef<Environment> closure;
    std::vector<ObjectTrackedKeyRef<Object>> body;

protected:
    virtual ObjectRef<Object> eval_body(
        const std::vector<ObjectRef<Object>>& arg_list,
        ObjectRef<Environment> enviroment) override;

public:
    SpecialOperatorUser(
        ObjectRef<Object> lambda_list,
        ObjectRef<Environment> closure,
        const std::vector<ObjectRef<Object>>& body);

    virtual std::string to_string(ObjectRef<Object> self) override;
    virtual ObjectRef<Object> type(ObjectRef<Object> self) const override;
};

// -----------------------------------------------------------------------------
