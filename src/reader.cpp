
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
#include <regex>

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
    int c = this->input.get();
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
    this->input.unget();
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

std::optional<ObjectRef<Object>> Reader::Input::read_character()
{
    int d = this->read_char();
    if (d != '.') {
        this->unread_char();
        return std::nullopt;
    }

    int e = this->read_char();
    if (e == '\\') {
        int f = this->read_char();
        int special_char;
        switch (f) {
        case 'n':
            special_char = '\n';
            break;
        case 'b':
            special_char = '\b';
            break;
        case 't':
            special_char = '\t';
            break;
        case '\\':
            special_char = '\\';
            break;
        default:
            ithrow("Expected a special character but found " << f);
        }
        return this->alma.make<Character>(special_char);
    } else {
        return this->alma.make<Character>(e);
    }
}

std::optional<ObjectRef<Object>> Reader::Input::read_character_macro()
{
    int h = this->read_char();
    if (h != '#') {
        this->unread_char();
        return std::nullopt;
    }

    int c = this->read_char();
    std::optional<ObjectRef<Object>> obj = this->read_next_object();
    iassert(obj, "Expected an object after #" << c);

    std::optional<ObjectRef<Procedure>> proc = this->alma.find_character_macro(c);
    iassert(proc.has_value(), "The character " << c << " is not associated to a character macro");

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
    std::regex int_regex(R"(^[+-]?\d+$)");
    if (std::regex_match(token, int_regex)) {
        int64_t value = std::stoll(token);
        return alma.make<Integer>(value);
    }
    return std::nullopt;
}

static std::pair<size_t, size_t> find_next_delimiter(const std::string& s, const std::vector<std::string>& delimiters, size_t start)
{
    for (const std::string& delimiter : delimiters) {
        size_t end = s.find(delimiter, start);
        if (end != std::string::npos) {
            return { end, delimiter.size() };
        }
    }
    return { std::string::npos, 0 };
}

static std::vector<std::string> splitString(const std::string& s, const std::vector<std::string>& delimiters)
{
    std::vector<std::string> tokens;
    size_t start = 0;
    while (true) {
        auto [end, delSize] = find_next_delimiter(s, delimiters, start);
        tokens.push_back(s.substr(start, end - start));
        start = end + delSize;
        if (end == std::string::npos)
            break;
    }

    return tokens;
}

static std::vector<std::string> parse_token(const std::string& token)
{
    return splitString(token, { ":" }); // Order matters. Most specific first
}

// TODO: Hacer 1-lisp
// TODO: Crear arrays [m 0 1]
// TODO: Hacer diccionarios {a 3 b 4 c 5}

ObjectRef<Symbol> Reader::Input::findSymbol(const std::vector<std::string>& splittedTokens)
{
    ObjectRef<Package> packageIt = alma.get_current_package();
    for (size_t i = 0; i < splittedTokens.size() - 1; i++) {
        ObjectRef<Symbol> packageSymbol = packageIt->intern_symbol(splittedTokens[i]);
        std::optional<ObjectRef<Package>> next_package = packageSymbol->get_package();
        if (!next_package) {
            std::string currentSymbol;
            for (size_t j = 0; j < i; j++)
                currentSymbol += splittedTokens[j] + ":";
            currentSymbol += splittedTokens[i];
            ithrow("The symbol " << currentSymbol << " does not denote a package.");
        }
        packageIt = *next_package;
    }
    return packageIt->intern_symbol(splittedTokens.back());
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

    std::vector<std::string> splittedTokens = parse_token(token);
    return findSymbol(splittedTokens);
}

#define maybe(EXPR)            \
    if (auto __obj__ = EXPR) { \
        return __obj__;        \
    }

std::optional<ObjectRef<Object>> Reader::Input::read_next_object()
{
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
