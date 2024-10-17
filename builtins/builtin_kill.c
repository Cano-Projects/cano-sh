#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define __USE_POSIX
#include <signal.h>

#include "all_builtins.h"

static
bool str_isdigit(char const *str)
{
	// TODO: use strtoll
	for(size_t i = 0; str[i] != '\0'; i++)
		if(!isdigit(str[i]))
			return false;
	return true;
}

bool builtin_kill(char **args)
{
	if(args[1] == NULL) {
		fprintf(stderr, "usage: kill <pid>\n");
		return true;
	}
	int signal = SIGTERM;
	size_t i = 1;
	if(args[i][0] == '-') {
		while(args[i][0] == '-') {
			if(strcmp(args[i], "-s") == 0 || strcmp(args[i], "--signal") == 0) {
				if(args[i+1] == NULL || !str_isdigit(args[i+1])) {
					fprintf(stderr, "error: signal must have a numerical argument\n");
					return true;
				}
				signal = strtol(args[i+1], NULL, 10);
				i += 2;
			} else {
				fprintf(stderr, "unrecognized flag `%s`\n", args[i]);
				return true;
			}
		}
	}
	if(!str_isdigit(args[i])) {
		// TODO: convert string to PID and kill
		return true;
	}
	pid_t pid = strtol(args[i], NULL, 10);
	if(kill(pid, signal) < 0) {
		fprintf(stderr, "%s", strerror(errno));
	}
	return true;
}
