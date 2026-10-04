
#pragma once

#include "object/object.hpp"
#include <iostream>
#include <optional>
#include <vector>

class Alma;
class Symbol;
class String;

class Reader {
private:
    class Input {
    private:
        std::string name;
        Alma& alma;
        std::istream& input;
        size_t pos;
        size_t line;
        bool changed_line;
        size_t last_pos;

        size_t quasiquote_level;

    private:
        ObjectRef<Symbol> findSymbol(const std::vector<std::string>& splittedTokens);

    public:
        template <typename... AS>
        Input(Alma& alma, const std::string& name, AS&&... args);

        bool eof() const;

        bool read_until_next_object(bool eofp);

        int read_char();
        void unread_char();
        bool read_whitespace();
        bool read_comment();
        void read_blank();
        bool read_next_char(char c);
        std::optional<ObjectRef<String>> read_string();
        std::optional<ObjectRef<Object>> read_list();
        std::optional<ObjectRef<Object>> read_character();
        std::optional<ObjectRef<Object>> read_character_macro();
        std::optional<ObjectRef<Object>> read_quote();
        std::optional<ObjectRef<Object>> read_quasiquote();
        std::optional<ObjectRef<Object>> read_unquote();
        std::optional<ObjectRef<Object>> read_token();
        std::optional<ObjectRef<Object>> read_next_object();
    } input;

public:
    template <typename... AS>
    Reader(Alma& alma, const std::string& name, AS&&... args);

    std::optional<ObjectRef<Object>> read(bool eof = true);
};

template <typename... AS>
Reader::Input::Input(Alma& _alma, const std::string& _name, AS&&... args)
    : name(_name)
    , alma(_alma)
    , input(std::forward<AS>(args)...)
    , pos(1)
    , line(1)
    , changed_line(false)

    , quasiquote_level(0)
{
    this->input.exceptions(std::ios::eofbit);
}

template <typename... AS>
Reader::Reader(Alma& _alma, const std::string& name, AS&&... args)
    : input(_alma, name, std::forward<AS>(args)...)
{
}
