
#pragma once

#include "type.hpp"

class Statement {
public:
    virtual ~Statement() {};

    virtual std::string to_string() const = 0;
};

class Declaration : public Statement {
private:
    std::string name;
    std::unique_ptr<Type> type;

public:
    std::string to_string() const override;
};

class Declarations {
private:
    std::vector<>
};
