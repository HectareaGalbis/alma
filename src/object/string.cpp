
#include "string.hpp"
#include "alma.hpp"
#include "debug.hpp"
#include "interpiler/type.hpp"
#include <cstdio>

String::String(const std::string& _content)
    : content(_content)
{
}

std::string String::to_string(ObjectRef<Object> self [[maybe_unused]])
{
    std::string ss;
    ss.push_back('"');
    for (char c : this->content) {
        switch (c) {
        case '"':
            ss += "\\\"";
            break;
        case '\\':
            ss += "\\\\";
            break;
        default:
            ss.push_back(c);
            break;
        }
    }
    ss.push_back('"');
    return ss;
}

const std::string& String::get_content() const
{
    return this->content;
}

ObjectRef<Object> String::type(ObjectRef<Object> self [[maybe_unused]]) const
{
    return Alma::alma.intern_alma_symbol("string");
}

void String::emit_top_level(ObjectRef<Object> self, ObjectRef<Environment> environment [[maybe_unused]],
    Declarations& declarations [[maybe_unused]], std::stringstream& output [[maybe_unused]])
{
    athrow("The string " << this->to_string(self) << " cannot be emitted at the top level");
}

void String::emit_statement(ObjectRef<Object> self, ObjectRef<Environment> environment,
    Declarations& declarations, std::stringstream& output)
{
    Alma::alma.emit_expression(self, environment, declarations, output);
    output << ";\n";
}

std::unique_ptr<Type> String::emit_expression(ObjectRef<Object> self [[maybe_unused]],
    ObjectRef<Environment> environment [[maybe_unused]],
    Declarations& declarations [[maybe_unused]], std::stringstream& output)
{
    output << '"';
    for (unsigned char c : this->content) {
        switch (c) {
        case '"':
            output << "\\\"";
            break;
        case '\\':
            output << "\\\\";
            break;
        case '\n':
            output << "\\n";
            break;
        case '\t':
            output << "\\t";
            break;
        default:
            if (c < 0x20 || c == 0x7f) {
                char octal[5];
                std::snprintf(octal, sizeof(octal), "\\%03o", c);
                output << octal;
            } else {
                output << c;
            }
            break;
        }
    }
    output << '"';
    return std::make_unique<ValueType>("char", std::vector<Specifier> { Pointer {} });
}
