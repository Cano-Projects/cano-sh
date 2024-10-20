#include <assert.h>
#include <errno.h>
#include <locale.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include <linux/limits.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "builtins/all_builtins.h"
#include "cano_sh.h"

#define ctrl(x) ((x) & 0x1f)
#define SHELL "[canosh]$ "

static __attribute__((nonnull))
void execute_command(char **args) {
	int status;
	int pid = fork();
	if(pid < 0) { // error
		fprintf(stderr, "error %s", strerror(errno));
		return;
	} else if(!pid) { // child process
		if(execvp(args[0], args) < 0) {
			fprintf(stderr, "%s\n", strerror(errno));
		}
		exit(1);
	} else { // parent process
		int wpid = waitpid(pid, &status, 0);
		while(!WIFEXITED(status) && !WIFSIGNALED(status)) {
			wpid = waitpid(pid, &status, 0);
		}
		(void)wpid;
	}	
}

__attribute__((nonnull))
void handle_command(char **args) {
	bltn_handler *handler;

	if (*args == NULL) {
		fprintf(stderr, "error, no command\n");
		return;
	}

	handler = shell_builtin_lookup(*args);
	if (handler != NULL) {
		handler(args);
		return;
	}

	execute_command(args);
}

	
char *str_to_cstr(String str) {
	char *cstr = malloc(sizeof(char)*str.count+1);
	if (cstr == NULL)
		return NULL;
	memcpy(cstr, str.data, sizeof(char)*str.count);
	cstr[str.count] = '\0';
	return cstr;
}

char **parse_command(char *command) {
	char const *marker = &command[strlen(command) + 1];
	char *cur = strtok(command, " ");
	if(cur == NULL) {
		return NULL;
	}
	size_t args_s = 8;
	char **args = malloc(sizeof(char*)*args_s);
    if (args == NULL) {
        return NULL;
    }
	size_t args_cur = 0;
	char **resized;
	while(cur != NULL) {
		if(args_cur+2 >= args_s) {
			args_s *= 2;
			resized = realloc(args, sizeof(char*)*args_s);
			if (resized == NULL) {
				free(args);
				return false;
			}
			args = resized;
		}
		while(command[0] != '\0') command++;
		command++;
		assert(command);
		if(marker < command && command[0] == '\'') {
			command++;
			args[args_cur++] = cur;
			cur = command;
			args[args_cur++] = cur;
			while(command[0] != '\'' && command[0] != '\0') command++;
			if(command[0] == '\0') break;
			command[0] = '\0';
			command++;
			cur = strtok(command, " ");
			continue;
		}
		
		args[args_cur++] = cur;
		cur = strtok(NULL, " ");
	}

	args[args_cur] = NULL;

	return args;
}

#ifndef HOT_RELEAD
int main(void) {
	setlocale(LC_ALL, "");
	return shell_repl_run();
}
#endif
