#pragma once

#include <optional>
#include <string>

class Declarations;

/** Header of the C standard library that defines a type, if any */
std::optional<std::string> standard_type_header(const std::string& name);

/** Add the functions, variables and macros of the C standard library */
void declare_standard(Declarations& declarations);
