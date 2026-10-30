
#include "reader.hpp"
#include "alma.hpp"
#include "object/character.hpp"
#include "object/character_macro.hpp"
#include "object/cons.hpp"
#include "debug.hpp"
#include "object/integer.hpp"
#include "object/package.hpp"
#include "object/string.hpp"
#include <iostream>
#include <optional>

#define ithrow(MSG)                                                                       \
    {                                                                                     \
        std::stringstream gensym(ss);                                                     \
        gensym(ss) << this->name << ":" << this->line << ":" << this->pos << ": " << MSG; \
        throw std::runtime_error(gensym(ss).str());                                       \
    }

#define iassert(COND, MSG) \
    if (!(COND))           \
    ithrow(MSG)

bool Reader::Input::eof() const
{
    return this->input.eof();
}

bool Reader::Input::read_until_next_object(bool eofp)
{
    try {
        this->read_blank();
    } catch (const std::ios_base::failure& e) {
        if (!eofp)
            ithrow("Found EOF");
        return false;
    }
    return true;
}

bool Reader::Input::read_next_char(char c)
{
    this->read_blank();
    int d = this->read_char();
    if (c != d) {
        this->unread_char();
        return false;
    }
    return true;
}

int Reader::Input::read_char()
{
    int c = this->input.rdbuf()->sbumpc();
    if (c == std::char_traits<char>::eof())
        this->input.setstate(std::ios::eofbit | std::ios::failbit);
    this->last_pos = this->pos;
    this->changed_line = (c == '\n');
    if (this->changed_line) {
        this->line++;
        this->pos = 1;
    } else {
        this->pos++;
    }
    return c;
}

void Reader::Input::unread_char()
{
    if (this->input.eof())
        this->input.clear(this->input.rdstate() & ~std::ios::eofbit);
    if (!this->input.good())
        this->input.setstate(std::ios::failbit);
    else if (this->input.rdbuf()->sungetc() == std::char_traits<char>::eof())
        this->input.setstate(std::ios::badbit);
    iassert(!this->input.fail(), "Error unreading the last character");
    if (this->changed_line) {
        this->line--;
        this->changed_line = false;
        this->pos = this->last_pos;
    }
}

bool Reader::Input::read_whitespace()
{
    bool found = false;
    while (true) {
        int c = this->read_char();
        if (!(c == ' ' || c == '\n' || c == '\t')) {
            this->unread_char();
            break;
        }
        found = true;
    }

    return found;
}

bool Reader::Input::read_comment()
{
    int c = this->read_char();
    if (c != ';') {
        this->unread_char();
        return false;
    }
    while (this->read_char() != '\n') { }
    return true;
}

void Reader::Input::read_blank()
{
    while (this->read_whitespace() || this->read_comment()) { }
}

std::optional<ObjectRef<String>> Reader::Input::read_string()
{
    int q = this->read_char();
    if (q != '"') {
        this->unread_char();
        return std::nullopt;
    }
    std::string content;
    bool next_special = false;
    try {
        while (input) {
            int d = this->read_char();
            if (next_special) {
                switch (d) {
                case 'n':
                    content.push_back('\n');
                    break;
                case 'b':
                    content.push_back(' ');
                    break;
                case 't':
                    content.push_back('\t');
                    break;
                case '"':
                    content.push_back('"');
                    break;
                case '\\':
                    content.push_back('\\');
                    break;
                }
                next_special = false;
            } else {
                if (d == '\\')
                    next_special = true;
                else if (d == '"')
                    break;
                else
                    content.push_back(d);
            }
        }
    } catch (const std::ios_base::failure& e) {
        ithrow("Unfinished string");
    }
    return this->alma.make<String>(content);
}

std::optional<ObjectRef<Object>> Reader::Input::read_list()
{
    // Read left paren
    {
        int lp = this->read_char();
        if (lp != '(') {
            this->unread_char();
            return std::nullopt;
        }
    }

    // Read list objects
    std::vector<ObjectRef<Object>> objects;
    bool proper_list = true;
    try {
        while (input) {
            if (this->read_next_char('.')) {
                proper_list = false;
                break;
            }
            std::optional<ObjectRef<Object>> object = this->read_next_object();
            if (object)
                objects.push_back(*object);
            else
                break;
        }
    } catch (const std::ios_base::failure& e) {
        ithrow("Expected the character ')'");
    }

    // Read non proper object
    std::optional<ObjectRef<Object>> non_proper_object;
    try {
        if (!proper_list) {
            non_proper_object = this->read_next_object();
            iassert(non_proper_object, "Expected an object after the dot character");
            this->read_blank();
        }
    } catch (const std::ios_base::failure& e) {
        ithrow("Expected an object after the dot character");
    }

    // Read right paren
    try {
        int rp = this->read_char();
        iassert(rp == ')', "Expected the character ')' but found '" << static_cast<char>(rp) << "'");
    } catch (const std::ios_base::failure& e) {
        ithrow("Expected the character ')'");
    }

    // Make result
    if (objects.empty())
        return this->alma.intern_alma_symbol("nil");
    else if (non_proper_object) {
        return this->alma.make<Cons>(objects, *non_proper_object);
    } else {
        return this->alma.make<Cons>(objects);
    }
}

static bool is_token_character(int c);

std::optional<ObjectRef<Object>> Reader::Input::read_character()
{
    int d = this->read_char();
    if (d != '\\') {
        this->unread_char();
        return std::nullopt;
    }

    // A backslash followed by one character is that character, even a delimiter. If more token
    // characters follow, they are the name of a character, like space or newline
    int e = this->read_char();
    std::string text(1, static_cast<char>(e));
    if (is_token_character(e)) {
        while (true) {
            int f = this->read_char();
            if (!is_token_character(f)) {
                this->unread_char();
                break;
            }
            text.push_back(static_cast<char>(f));
        }
    }
    if (text.size() == 1)
        return this->alma.make<Character>(text[0]);
    std::optional<char> named = Character::from_name(text);
    iassert(named, "Unknown character \\" << text);
    return this->alma.make<Character>(*named);
}

std::optional<ObjectRef<Object>> Reader::Input::read_character_macro()
{
    int h = this->read_char();
    if (h != '#') {
        this->unread_char();
        return std::nullopt;
    }

    int c = this->read_char();
    std::optional<ObjectRef<Object>> obj;
    if (c == '(' || c == '[' || c == '{') {
        // The brackets are read until their pair, and their objects are received as a list
        char closing = c == '(' ? ')' : c == '[' ? ']' : '}';
        std::vector<ObjectRef<Object>> objects;
        try {
            for (std::optional<ObjectRef<Object>> object = this->read_next_object(); object;
                object = this->read_next_object())
                objects.push_back(*object);
            int d = this->read_char();
            iassert(d == closing, "Expected the character '" << closing << "' but found '" << static_cast<char>(d) << "'");
        } catch (const std::ios_base::failure& e) {
            ithrow("Expected the character '" << closing << "'");
        }
        if (objects.empty())
            obj = this->alma.intern_alma_symbol("nil");
        else
            obj = this->alma.make<Cons>(objects);
    } else {
        obj = this->read_next_object();
        iassert(obj, "Expected an object after #" << static_cast<char>(c));
    }

    std::optional<ObjectRef<Procedure>> proc = this->alma.get_current_package()->find_character_macro(c);
    iassert(proc.has_value(), "The character " << static_cast<char>(c) << " is not associated to a character macro");

    return this->alma.make<CharacterMacro>(*proc, *obj);
}

std::optional<ObjectRef<Object>> Reader::Input::read_quote()
{
    int q = this->read_char();
    if (q != '\'') {
        this->unread_char();
        return std::nullopt;
    }
    std::optional<ObjectRef<Object>> object = this->read_next_object();
    iassert(object, "Expected an object after the quote");
    ObjectRef<Object> qs = this->alma.intern_alma_symbol("quote");

    return this->alma.make<Cons>(std::vector<ObjectRef<Object>> { qs, *object });
}

std::optional<ObjectRef<Object>> Reader::Input::read_quasiquote()
{
    int q = this->read_char();
    if (q != '`') {
        this->unread_char();
        return std::nullopt;
    }
    this->quasiquote_level++;
    std::optional<ObjectRef<Object>> object = this->read_next_object();
    iassert(object, "Expected an object after the backquote");
    this->quasiquote_level--;
    ObjectRef<Object> qs = this->alma.intern_alma_symbol("quasiquote");

    return this->alma.make<Cons>(std::vector<ObjectRef<Object>> { qs, *object });
}

std::optional<ObjectRef<Object>> Reader::Input::read_unquote()
{
    int q = this->read_char();
    if (q != ',') {
        this->unread_char();
        return std::nullopt;
    }
    bool slice = true;
    int s = this->read_char();
    if (s != '@') {
        slice = false;
        this->unread_char();
    }
    iassert(quasiquote_level != 0, (slice ? "slice-unquote" : "unquote") << " outside quasiquote");

    quasiquote_level--;
    std::optional<ObjectRef<Object>> object = this->read_next_object();
    iassert(object, "Expected an object after the " << (slice ? "slice-unquote" : "unquote"));
    quasiquote_level++;
    ObjectRef<Object> qs = this->alma.intern_alma_symbol(slice ? "slice-unquote" : "unquote");

    return this->alma.make<Cons>(std::vector<ObjectRef<Object>> { qs, *object });
}

static bool is_token_character(int c)
{
    return c == '!' || (c >= '#' && c <= '&') || (c >= '*' && c <= '+') || (c >= '-' && c <= ':')
        || (c >= '<' && c <= 'Z') || c == '_' || (c >= 'a' && c <= 'z');
}

static std::optional<ObjectRef<Integer>> parse_number(const std::string& token, Alma& alma)
{
    size_t start = (!token.empty() && (token[0] == '+' || token[0] == '-')) ? 1 : 0;
    bool is_integer = token.size() > start;
    for (size_t i = start; is_integer && i < token.size(); i++)
        is_integer = token[i] >= '0' && token[i] <= '9';
    if (is_integer) {
        int64_t value = std::stoll(token);
        return alma.make<Integer>(value);
    }
    return std::nullopt;
}

// TODO: Hacer 1-lisp
// TODO: Crear arrays [m 0 1]
// TODO: Hacer diccionarios {a 3 b 4 c 5}

ObjectRef<Symbol> Reader::Input::find_symbol(const std::string& token)
{
    size_t colon = token.find(':');
    if (colon == std::string::npos)
        return alma.get_current_package()->intern_symbol(token);

    if (token.starts_with("::")) {
        std::string symbol_name = token.substr(2);
        iassert(!symbol_name.empty() && symbol_name.find(':') == std::string::npos, "Invalid symbol " << token);
        return alma.make<Symbol>(symbol_name);
    }

    if (colon == 0) {
        std::string keyword_name = token.substr(1);
        iassert(!keyword_name.empty() && keyword_name.find(':') == std::string::npos, "Invalid keyword " << token);
        return alma.intern_keyword(keyword_name);
    }

    std::string package_name = token.substr(0, colon);
    std::string symbol_name = token.substr(colon + 1);
    iassert(symbol_name.empty() || symbol_name[0] != ':',
        "Cannot reference " << token << ": only the exported symbols of a package can be referenced");
    iassert(!symbol_name.empty() && symbol_name.find(':') == std::string::npos, "Invalid symbol " << token);
    std::optional<ObjectRef<Package>> package = alma.find_package(package_name);
    iassert(package, "The package " << package_name << " does not exist");
    if ((*package)->is_keyword())
        return alma.intern_keyword(symbol_name);
    std::optional<ObjectRef<Symbol>> symbol = (*package)->find_external_symbol(symbol_name);
    iassert(symbol, "The symbol " << symbol_name << " is not exported from the package " << package_name);
    return *symbol;
}

std::optional<ObjectRef<Object>> Reader::Input::read_token()
{
    int c = this->input.peek();
    if (!is_token_character(c))
        return std::nullopt;
    std::string token;
    while (input) {
        int d = this->read_char();
        if (!is_token_character(d)) {
            this->unread_char();
            break;
        }
        token.push_back(d);
    }

    std::optional<ObjectRef<Integer>> number = parse_number(token, this->alma);
    if (number)
        return number;

    return this->find_symbol(token);
}

#define maybe(EXPR)            \
    if (auto __obj__ = EXPR) { \
        return __obj__;        \
    }

std::optional<ObjectRef<Object>> Reader::Input::read_next_object()
{
    this->alma.check_stack();
    this->read_blank();
    maybe(this->read_string());
    maybe(this->read_list());
    maybe(this->read_character());
    maybe(this->read_character_macro());
    maybe(this->read_quote());
    maybe(this->read_quasiquote());
    maybe(this->read_unquote());
    maybe(this->read_token());
    return std::nullopt;
}

std::optional<ObjectRef<Object>> Reader::read(bool eof)
{
    if (this->input.read_until_next_object(eof))
        return this->input.read_next_object();
    return std::nullopt;
}
