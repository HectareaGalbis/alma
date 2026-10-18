
#include "standard.hpp"
#include "../alma.hpp"
#include "../debug.hpp"
#include "../reader.hpp"
#include "declaration.hpp"
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

/*
Declarations of the C standard library. The types use the syntax of alma:

  int                      -> int
  (char *)                 -> char *
  (int ((char *) &rest))   -> int (char *, ...)
  (void ((void *)) *)      -> void (*)(void *)

There are no qualifiers, so 'const char *' is written as 'char *'. Macros that behave
like constants or functions are declared as variables or functions.
*/

struct StandardDeclaration {
    const char* header;
    const char* name;
    const char* type;
};

// The tables are local to functions because they are used while the static Alma object is
// constructed, and the static variables of this file may not be initialized yet.

static const std::vector<StandardDeclaration>& standard_declarations()
{
    static const std::vector<StandardDeclaration> declarations = {
    // assert.h
    { "assert.h", "assert", "(void (int))" },

    // ctype.h
    { "ctype.h", "isalnum", "(int (int))" },
    { "ctype.h", "isalpha", "(int (int))" },
    { "ctype.h", "isblank", "(int (int))" },
    { "ctype.h", "iscntrl", "(int (int))" },
    { "ctype.h", "isdigit", "(int (int))" },
    { "ctype.h", "isgraph", "(int (int))" },
    { "ctype.h", "islower", "(int (int))" },
    { "ctype.h", "isprint", "(int (int))" },
    { "ctype.h", "ispunct", "(int (int))" },
    { "ctype.h", "isspace", "(int (int))" },
    { "ctype.h", "isupper", "(int (int))" },
    { "ctype.h", "isxdigit", "(int (int))" },
    { "ctype.h", "tolower", "(int (int))" },
    { "ctype.h", "toupper", "(int (int))" },

    // errno.h
    { "errno.h", "errno", "int" },
    { "errno.h", "EDOM", "int" },
    { "errno.h", "ERANGE", "int" },
    { "errno.h", "EILSEQ", "int" },

    // float.h
    { "float.h", "FLT_MAX", "float" },
    { "float.h", "FLT_MIN", "float" },
    { "float.h", "FLT_EPSILON", "float" },
    { "float.h", "DBL_MAX", "double" },
    { "float.h", "DBL_MIN", "double" },
    { "float.h", "DBL_EPSILON", "double" },

    // limits.h
    { "limits.h", "CHAR_BIT", "int" },
    { "limits.h", "CHAR_MIN", "int" },
    { "limits.h", "CHAR_MAX", "int" },
    { "limits.h", "SCHAR_MIN", "int" },
    { "limits.h", "SCHAR_MAX", "int" },
    { "limits.h", "UCHAR_MAX", "int" },
    { "limits.h", "SHRT_MIN", "int" },
    { "limits.h", "SHRT_MAX", "int" },
    { "limits.h", "USHRT_MAX", "int" },
    { "limits.h", "INT_MIN", "int" },
    { "limits.h", "INT_MAX", "int" },
    { "limits.h", "UINT_MAX", "unsigned" },
    { "limits.h", "LONG_MIN", "long" },
    { "limits.h", "LONG_MAX", "long" },

    // locale.h
    { "locale.h", "setlocale", "((char *) (int (char *)))" },
    { "locale.h", "LC_ALL", "int" },
    { "locale.h", "LC_COLLATE", "int" },
    { "locale.h", "LC_CTYPE", "int" },
    { "locale.h", "LC_MONETARY", "int" },
    { "locale.h", "LC_NUMERIC", "int" },
    { "locale.h", "LC_TIME", "int" },

    // math.h (the float versions are added below)
    { "math.h", "frexp", "(double (double (int *)))" },
    { "math.h", "ldexp", "(double (double int))" },
    { "math.h", "modf", "(double (double (double *)))" },
    { "math.h", "fma", "(double (double double double))" },
    { "math.h", "ilogb", "(int (double))" },
    { "math.h", "lround", "(long (double))" },
    { "math.h", "lrint", "(long (double))" },
    { "math.h", "scalbn", "(double (double int))" },
    { "math.h", "isnan", "(int (double))" },
    { "math.h", "isinf", "(int (double))" },
    { "math.h", "isfinite", "(int (double))" },
    { "math.h", "signbit", "(int (double))" },
    { "math.h", "HUGE_VAL", "double" },
    { "math.h", "INFINITY", "float" },
    { "math.h", "NAN", "float" },

    // signal.h
    { "signal.h", "signal", "((void (int) *) (int (void (int) *)))" },
    { "signal.h", "raise", "(int (int))" },
    { "signal.h", "SIG_DFL", "(void (int) *)" },
    { "signal.h", "SIG_IGN", "(void (int) *)" },
    { "signal.h", "SIG_ERR", "(void (int) *)" },
    { "signal.h", "SIGABRT", "int" },
    { "signal.h", "SIGFPE", "int" },
    { "signal.h", "SIGILL", "int" },
    { "signal.h", "SIGINT", "int" },
    { "signal.h", "SIGSEGV", "int" },
    { "signal.h", "SIGTERM", "int" },

    // stdbool.h
    { "stdbool.h", "true", "bool" },
    { "stdbool.h", "false", "bool" },

    // stddef.h
    { "stddef.h", "NULL", "(void *)" },

    // stdio.h
    { "stdio.h", "stdin", "(FILE *)" },
    { "stdio.h", "stdout", "(FILE *)" },
    { "stdio.h", "stderr", "(FILE *)" },
    { "stdio.h", "EOF", "int" },
    { "stdio.h", "BUFSIZ", "int" },
    { "stdio.h", "FILENAME_MAX", "int" },
    { "stdio.h", "SEEK_SET", "int" },
    { "stdio.h", "SEEK_CUR", "int" },
    { "stdio.h", "SEEK_END", "int" },
    { "stdio.h", "_IOFBF", "int" },
    { "stdio.h", "_IOLBF", "int" },
    { "stdio.h", "_IONBF", "int" },
    { "stdio.h", "printf", "(int ((char *) &rest))" },
    { "stdio.h", "fprintf", "(int ((FILE *) (char *) &rest))" },
    { "stdio.h", "sprintf", "(int ((char *) (char *) &rest))" },
    { "stdio.h", "snprintf", "(int ((char *) size_t (char *) &rest))" },
    { "stdio.h", "scanf", "(int ((char *) &rest))" },
    { "stdio.h", "fscanf", "(int ((FILE *) (char *) &rest))" },
    { "stdio.h", "sscanf", "(int ((char *) (char *) &rest))" },
    { "stdio.h", "puts", "(int ((char *)))" },
    { "stdio.h", "fputs", "(int ((char *) (FILE *)))" },
    { "stdio.h", "putchar", "(int (int))" },
    { "stdio.h", "fputc", "(int (int (FILE *)))" },
    { "stdio.h", "putc", "(int (int (FILE *)))" },
    { "stdio.h", "getchar", "(int ())" },
    { "stdio.h", "fgetc", "(int ((FILE *)))" },
    { "stdio.h", "getc", "(int ((FILE *)))" },
    { "stdio.h", "ungetc", "(int (int (FILE *)))" },
    { "stdio.h", "fgets", "((char *) ((char *) int (FILE *)))" },
    { "stdio.h", "fopen", "((FILE *) ((char *) (char *)))" },
    { "stdio.h", "freopen", "((FILE *) ((char *) (char *) (FILE *)))" },
    { "stdio.h", "fclose", "(int ((FILE *)))" },
    { "stdio.h", "fflush", "(int ((FILE *)))" },
    { "stdio.h", "fread", "(size_t ((void *) size_t size_t (FILE *)))" },
    { "stdio.h", "fwrite", "(size_t ((void *) size_t size_t (FILE *)))" },
    { "stdio.h", "fseek", "(int ((FILE *) long int))" },
    { "stdio.h", "ftell", "(long ((FILE *)))" },
    { "stdio.h", "rewind", "(void ((FILE *)))" },
    { "stdio.h", "fgetpos", "(int ((FILE *) (fpos_t *)))" },
    { "stdio.h", "fsetpos", "(int ((FILE *) (fpos_t *)))" },
    { "stdio.h", "feof", "(int ((FILE *)))" },
    { "stdio.h", "ferror", "(int ((FILE *)))" },
    { "stdio.h", "clearerr", "(void ((FILE *)))" },
    { "stdio.h", "perror", "(void ((char *)))" },
    { "stdio.h", "remove", "(int ((char *)))" },
    { "stdio.h", "rename", "(int ((char *) (char *)))" },
    { "stdio.h", "tmpfile", "((FILE *) ())" },
    { "stdio.h", "tmpnam", "((char *) ((char *)))" },
    { "stdio.h", "setbuf", "(void ((FILE *) (char *)))" },
    { "stdio.h", "setvbuf", "(int ((FILE *) (char *) int size_t))" },

    // stdlib.h
    { "stdlib.h", "EXIT_SUCCESS", "int" },
    { "stdlib.h", "EXIT_FAILURE", "int" },
    { "stdlib.h", "RAND_MAX", "int" },
    { "stdlib.h", "MB_CUR_MAX", "size_t" },
    { "stdlib.h", "malloc", "((void *) (size_t))" },
    { "stdlib.h", "calloc", "((void *) (size_t size_t))" },
    { "stdlib.h", "realloc", "((void *) ((void *) size_t))" },
    { "stdlib.h", "free", "(void ((void *)))" },
    { "stdlib.h", "abort", "(void ())" },
    { "stdlib.h", "exit", "(void (int))" },
    { "stdlib.h", "atexit", "(int ((void () *)))" },
    { "stdlib.h", "getenv", "((char *) ((char *)))" },
    { "stdlib.h", "system", "(int ((char *)))" },
    { "stdlib.h", "atoi", "(int ((char *)))" },
    { "stdlib.h", "atol", "(long ((char *)))" },
    { "stdlib.h", "atof", "(double ((char *)))" },
    { "stdlib.h", "strtol", "(long ((char *) (char * *) int))" },
    { "stdlib.h", "strtod", "(double ((char *) (char * *)))" },
    { "stdlib.h", "strtof", "(float ((char *) (char * *)))" },
    { "stdlib.h", "rand", "(int ())" },
    { "stdlib.h", "srand", "(void (unsigned))" },
    { "stdlib.h", "abs", "(int (int))" },
    { "stdlib.h", "labs", "(long (long))" },
    { "stdlib.h", "div", "(div_t (int int))" },
    { "stdlib.h", "ldiv", "(ldiv_t (long long))" },
    { "stdlib.h", "qsort", "(void ((void *) size_t size_t (int ((void *) (void *)) *)))" },
    { "stdlib.h", "bsearch", "((void *) ((void *) (void *) size_t size_t (int ((void *) (void *)) *)))" },

    // string.h
    { "string.h", "strlen", "(size_t ((char *)))" },
    { "string.h", "strcpy", "((char *) ((char *) (char *)))" },
    { "string.h", "strncpy", "((char *) ((char *) (char *) size_t))" },
    { "string.h", "strcat", "((char *) ((char *) (char *)))" },
    { "string.h", "strncat", "((char *) ((char *) (char *) size_t))" },
    { "string.h", "strcmp", "(int ((char *) (char *)))" },
    { "string.h", "strncmp", "(int ((char *) (char *) size_t))" },
    { "string.h", "strcoll", "(int ((char *) (char *)))" },
    { "string.h", "strxfrm", "(size_t ((char *) (char *) size_t))" },
    { "string.h", "strchr", "((char *) ((char *) int))" },
    { "string.h", "strrchr", "((char *) ((char *) int))" },
    { "string.h", "strstr", "((char *) ((char *) (char *)))" },
    { "string.h", "strpbrk", "((char *) ((char *) (char *)))" },
    { "string.h", "strspn", "(size_t ((char *) (char *)))" },
    { "string.h", "strcspn", "(size_t ((char *) (char *)))" },
    { "string.h", "strtok", "((char *) ((char *) (char *)))" },
    { "string.h", "strerror", "((char *) (int))" },
    { "string.h", "memcpy", "((void *) ((void *) (void *) size_t))" },
    { "string.h", "memmove", "((void *) ((void *) (void *) size_t))" },
    { "string.h", "memset", "((void *) ((void *) int size_t))" },
    { "string.h", "memcmp", "(int ((void *) (void *) size_t))" },
    { "string.h", "memchr", "((void *) ((void *) int size_t))" },

    // time.h
    { "time.h", "CLOCKS_PER_SEC", "clock_t" },
    { "time.h", "clock", "(clock_t ())" },
    { "time.h", "time", "(time_t ((time_t *)))" },
    { "time.h", "difftime", "(double (time_t time_t))" },
    { "time.h", "ctime", "((char *) ((time_t *)))" },
    };
    return declarations;
}

/** Functions of math.h of the form double f(double), with a float version f + "f" */
static const std::vector<const char*>& math_unary_functions()
{
    static const std::vector<const char*> functions = {
        "acos", "asin", "atan", "cos", "sin", "tan", "acosh", "asinh", "atanh", "cosh", "sinh", "tanh",
        "exp", "exp2", "expm1", "log", "log10", "log2", "log1p", "logb", "sqrt", "cbrt", "fabs",
        "ceil", "floor", "round", "trunc", "rint", "nearbyint", "erf", "erfc", "tgamma", "lgamma"
    };
    return functions;
}

/** Functions of math.h of the form double f(double, double), with a float version f + "f" */
static const std::vector<const char*>& math_binary_functions()
{
    static const std::vector<const char*> functions = {
        "atan2", "pow", "fmod", "hypot", "fmax", "fmin", "fdim", "remainder", "copysign", "nextafter"
    };
    return functions;
}

std::optional<std::string> standard_type_header(const std::string& name)
{
    static const std::unordered_map<std::string, std::string> type_headers = {
        { "bool", "stdbool.h" },
        { "size_t", "stddef.h" },
        { "ptrdiff_t", "stddef.h" },
        { "wchar_t", "stddef.h" },
        { "int8_t", "stdint.h" },
        { "int16_t", "stdint.h" },
        { "int32_t", "stdint.h" },
        { "int64_t", "stdint.h" },
        { "uint8_t", "stdint.h" },
        { "uint16_t", "stdint.h" },
        { "uint32_t", "stdint.h" },
        { "uint64_t", "stdint.h" },
        { "intptr_t", "stdint.h" },
        { "uintptr_t", "stdint.h" },
        { "FILE", "stdio.h" },
        { "fpos_t", "stdio.h" },
        { "div_t", "stdlib.h" },
        { "ldiv_t", "stdlib.h" },
        { "sig_atomic_t", "signal.h" },
        { "time_t", "time.h" },
        { "clock_t", "time.h" },
    };
    auto it = type_headers.find(name);
    if (it == type_headers.end())
        return std::nullopt;
    return it->second;
}

static std::unique_ptr<Type> parse_standard_type(const std::string& text)
{
    std::istringstream input(text + "\n");
    Reader reader(Alma::alma, "<standard>", input);
    std::optional<ObjectRef<Object>> obj = reader.read();
    massert(obj, "Cannot read the standard type " << text);
    return Type::parse_object(*obj);
}

/** Variables of the standard library that are not constants */
static bool is_standard_variable(const std::string& name)
{
    static const std::unordered_set<std::string> variables = {
        "errno", "stdin", "stdout", "stderr", "MB_CUR_MAX"
    };
    return variables.contains(name);
}

static void declare(Declarations& declarations, const std::string& header, const std::string& name,
    const std::string& type)
{
    std::unique_ptr<Type> parsed = parse_standard_type(type);
    bool constant = !parsed->is_function() && !is_standard_variable(name);
    declarations.add(Declaration(name, parsed, false, header, constant));
}

void declare_standard(Declarations& declarations)
{
    for (const StandardDeclaration& declaration : standard_declarations())
        declare(declarations, declaration.header, declaration.name, declaration.type);

    for (const char* name : math_unary_functions()) {
        declare(declarations, "math.h", name, "(double (double))");
        declare(declarations, "math.h", std::string(name) + "f", "(float (float))");
    }
    for (const char* name : math_binary_functions()) {
        declare(declarations, "math.h", name, "(double (double double))");
        declare(declarations, "math.h", std::string(name) + "f", "(float (float float))");
    }
}
