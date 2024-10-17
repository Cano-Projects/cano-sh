#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "all_builtins.h"

bool builtin_cd(char **args)
{
	char const *dir = "~";

	if (args[1] != NULL) {
		dir = args[1];
	}
	if (chdir(dir) < 0) {
		fprintf(stderr, "%s %s", dir, strerror(errno));
	}
	return true;
}
