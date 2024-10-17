#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "all_builtins.h"

// TODO: implement \e \E
// Also hex and octal (and unicode?)
static
const char *get_escape_c(char const *str, size_t index)
{
	if (str[index] == '\\') {
		switch(str[index+1]) {
			case 'a': return "\a";
			case 'b': return "\b";
			case 'c': return "s";
			case 'e': return "e";
			case 'E': return "E";
			case 'f': return "\f";
			case 'n': return "\n";
			case 'r': return "\r";
			case 't': return "\t";
			case 'v': return "\v";
			case '\\': return "\\";
			case '0': return "\0";
			default:
				break;
		}
	}
	return " ";
}

static
char *handle_flags(char const *args, const char *flags)
{
	char result[64] = {0};
	size_t result_s = 0;
	for(size_t i = 0; flags[i] != '\0'; i++) {
		for(size_t j = 1; args[j] != '\0'; j++) {
			if(flags[i] == args[j])
				result[result_s++] = flags[i];
		}
	}
	char *result_a = malloc(sizeof(char)*result_s+1);
	if(result_a == NULL)
		return NULL;
	strncpy(result_a, result, result_s);
	result_a[result_s] = '\0';
	return result_a;
}

bool builtin_echo(char **args)
{
	bool newline = true;
	bool escapes = false;

	for(size_t i = 1; args[i] != NULL; i++) {
		if(args[i][0] == '-') {
			char *flags = handle_flags(args[i], "ne");
			if(flags != NULL) {
				for(size_t flag = 0; flags[flag] != '\0'; flag++) {
					switch(flags[flag]) {
						case 'n':
							newline = false;
							break;
						case 'e':
							escapes = true;
							break;
					}
				}
				free(flags);
			}
			continue;
		}
		for(size_t j = 0; args[i][j] != '\0'; j++) {
			if(args[i][j] == '\\') {
				const char *esc = get_escape_c(args[i], j);
				if(esc[0] == ' ') continue;
				if(esc[0] == 's') return true;
				if(escapes) {
					write(STDOUT_FILENO, esc, 1);
					j++;
				}
			} else {
				write(STDOUT_FILENO, args[i]+j, 1);
			}
		}
		if(args[i+1] != NULL)
			write(STDOUT_FILENO, " ", 1);
	}
	if(newline)
		write(STDOUT_FILENO, "\n", 1);
	return true;
}
