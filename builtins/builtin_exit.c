#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "all_builtins.h"

static
bool str_isdigit(char const *str)
{
	for(size_t i = 0; str[i] != '\0'; i++)
		if(!isdigit(str[i]))
			return false;
	return true;
}

bool builtin_exit(char **args)
{
	int exit_code = 0;

	if(args[1] != NULL) {
		if(!str_isdigit(args[1])) {
			fprintf(stderr, "numeric argument required - %s\n", args[1]);
			return true;
		}
		exit_code = strtol(args[1], NULL, 10);
	}
	printf("exit\n");
	exit(exit_code);
	return true;
}
