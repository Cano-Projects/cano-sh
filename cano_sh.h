#ifndef CANO_SH_H
    #define CANO_SH_H

    #include <stdbool.h>
    #include <stddef.h>

    #define DATA_START_CAPACITY 128
    #define SSTR_LEN(sstr) (sizeof(sstr) - 1)

    #define ASSERT(cond, ...) \
        do { \
            if (!(cond)) { \
                endwin();   \
                fprintf(stderr, "%s:%d: ASSERTION FAILED: ", __FILE__, __LINE__); \
                fprintf(stderr, __VA_ARGS__); \
                fprintf(stderr, "\n"); \
                exit(1); \
            } \
        } while (0)

    #define DA_APPEND(da, item) do {                                                       \
        if ((da)->count >= (da)->capacity) {                                               \
            (da)->capacity = (da)->capacity == 0 ? DATA_START_CAPACITY : (da)->capacity*2; \
            void *new = calloc(((da)->capacity+1), sizeof(*(da)->data));                   \
            ASSERT(new,"outta ram");                                                       \
            if ((da)->data != NULL)                                                        \
                memcpy(new, (da)->data, (da)->count);                                      \
            free((da)->data);                                                              \
            (da)->data = new;                                                              \
        }                                                                                  \
        (da)->data[(da)->count++] = (item);                                                \
    } while (0)


typedef struct {
	char *data;
	size_t count;
	size_t capacity;
} String;

typedef struct {
	String *data;
	size_t count;
	size_t capacity;
} Strings;

typedef struct shell_repl_s {
	String input;
    Strings command_his;
	String clipboard;
    size_t line;
	WINDOW *buffer;
    bool is_running;
} Repl;

char *str_to_cstr(String str);

int shell_repl_run(void);

char **parse_command(char *command);
void execute_command(Repl *repl, char **args, size_t *line);
void handle_command(Repl *repl, char **args, size_t *line);


#endif
