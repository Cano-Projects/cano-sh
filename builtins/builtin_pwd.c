#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <linux/limits.h>

bool builtin_pwd(char **args)
{
	char path[PATH_MAX] = {0};

	(void)args;
	if(getcwd(path, PATH_MAX) == NULL) {
		fprintf(stderr, "%s", strerror(errno));
	}
	write(STDOUT_FILENO, path, strlen(path));
	write(STDOUT_FILENO, "\n", 1);
	return true;
}
