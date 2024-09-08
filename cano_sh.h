#ifndef CANO_SH_H
    #define CANO_SH_H

    #include <stddef.h>

void clear_line(size_t line, size_t width);

char **parse_command(char *command);
void execute_command(char **args, size_t *line);
void handle_command(char **args, size_t *line);

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

char *str_to_cstr(String str);

#endif
