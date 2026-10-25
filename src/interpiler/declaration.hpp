
#pragma once

#include "record.hpp"
#include "type.hpp"
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <unordered_map>
#include <unordered_set>

class Declaration {
private:
    std::string name;
    std::unique_ptr<Type> type;
    bool defined;
    std::optional<std::string> header;
    bool constant;

public:
    Declaration(const std::string& name, const std::unique_ptr<Type>& type, bool defined = false,
        const std::optional<std::string>& header = std::nullopt, bool constant = false);
    Declaration(const Declaration& other);
    Declaration& operator=(const Declaration& other);
    Declaration(Declaration&& other) noexcept;
    Declaration& operator=(Declaration&& other) noexcept;

    const std::string& getName() const;
    const Type* getType() const;
    bool isDefined() const;
    /** Header of the C standard library where it is declared, if any */
    const std::optional<std::string>& getHeader() const;
    /** A constant, like the macros of the standard library (EOF, INT_MAX...) */
    bool isConstant() const;

    void define();

    bool operator==(const Declaration& other) const;
};

class Declarations {
private:
    std::unordered_map<std::string, Declaration> declarations;
    std::unordered_set<std::string> locals;
    std::optional<Declaration> function;
    std::shared_ptr<std::set<std::string>> headers = std::make_shared<std::set<std::string>>();
    Records records;
    /** Names of the constants of each enum */
    std::map<std::string, std::vector<std::string>> enums;
    bool in_loop = false;
    bool in_switch = false;

    /** Labels of the current function, shared by all its blocks */
    struct Labels {
        std::set<std::string> defined;
        std::set<std::string> used;
    };
    std::shared_ptr<Labels> labels;

public:
    /** Add a top level declaration. A declaration of the C standard library is replaced by
        any other declaration with the same name, unless both are equal. */
    void add(const Declaration& declaration);
    bool contains(const std::string& name) const;
    const Declaration& get(const std::string& name) const;
    /** Names of all the declarations, sorted */
    std::vector<std::string> names() const;

    /** Like get, but remembers the header of the declaration to include it in the output */
    const Declaration& use(const std::string& name);
    /** Remember the headers of the standard types used by a type */
    void use_type(const Type& type);
    /** Headers used by the emitted code */
    const std::set<std::string>& get_headers() const;

    /** Copy that does not share the used headers with these declarations.
        Scopes of functions share them so that every use inside a function is remembered. */
    Declarations deep_copy() const;

    /** Add a declaration that belongs to the current scope. It shadows any previous
        declaration with the same name, but cannot be declared twice in the same scope. */
    void add_local(const Declaration& declaration);

    /** Return a copy of these declarations to check the body of a function.
        The copy starts a new scope and remembers the function it belongs to. */
    Declarations function_scope(const Declaration& function) const;

    /** Return a copy of these declarations to check a block inside a function. The copy starts a new scope. */
    Declarations block_scope() const;
    /** Like block_scope, for the body of a loop or a switch */
    Declarations loop_scope() const;
    Declarations switch_scope() const;

    /** Labels of the current function. A goto can jump to a label defined later, so the labels
        used by goto are checked when the function ends. */
    void define_label(const std::string& name);
    void use_label(const std::string& name);
    /** Labels used by goto that are not defined */
    std::vector<std::string> undefined_labels() const;

    /** break is valid inside a loop or a switch, and continue inside a loop */
    bool can_break() const;
    bool can_continue() const;

    /** Return the function whose body is being checked, if any */
    const Declaration* get_function() const;

    /** Structures declared or defined */
    Records& get_records();
    const Records& get_records() const;

    /** Enums and the names of their constants */
    void add_enum(const std::string& name, const std::vector<std::string>& constants);
    bool is_enum(const std::string& name) const;
    const std::vector<std::string>& get_enum_constants(const std::string& name) const;
    std::vector<std::string> enum_names() const;

    /** Return the structure of a type, if it is the name of a declared structure */
    const Record* get_record(const Type& type) const;
    /** Types whose size is known: not void, not a function, and not an undefined structure or an array of them */
    bool is_complete(const Type& type) const;

    template <typename... Args>
    void emplace(Args&&... args)
    {
        Declaration declaration(std::forward<Args>(args)...);
        if (!contains(declaration.getName())) {
            std::string name = declaration.getName();
            declarations.emplace(std::move(name), std::move(declaration));
            return;
        }
        add(declaration);
    }
};
