#include <string.h>

#include "../all_builtins.h"

static bltn_handler *BUILTIN_COMMANDS[] = {
    #define X(v) &(builtin_ ## v),
    #include "_list.xmacro"
    #undef X
};

static
const char *BUILTIN_NAMES[] = {
    #define X(v) #v,
    #include "_list.xmacro"
    #undef X
};

#define length_of(arr) (sizeof (arr) / sizeof *(arr))

bltn_handler *shell_builtin_lookup(char const *name)
{
    size_t lo = 0;
    size_t hi = length_of(BUILTIN_COMMANDS);
    size_t bisect;
    int cmp;

    while (lo < hi) {
        bisect = (lo + hi) >> 1;
        cmp = strcmp(BUILTIN_NAMES[bisect], name);
        if (!cmp)
            return BUILTIN_COMMANDS[bisect];
        if (cmp < 0)
            lo = bisect + 1;
        else
            hi = bisect;
    }
    return NULL;
}
