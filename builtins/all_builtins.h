#include <stdbool.h>
#include <stddef.h>

typedef bool (bltn_handler)(char **);
bltn_handler *shell_builtin_lookup(char const *name);

#define X(func) bool (builtin_ ## func)(char **args);
#include "meta/_list.xmacro"
#undef X
