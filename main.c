#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include <ncurses.h>

#include "cano_sh.h"

#define ctrl(x) ((x) & 0x1f)
#define SHELL "[canosh]$ "

static
void close_pipe(int pipes[2]) {
	close(pipes[0]);
	close(pipes[1]);
}

void execute_command(Repl *repl, char **args, size_t *line) {
	char buf[4096] = {0};
	int stdout_des[2];
	int stderr_des[2];
	if(pipe(stdout_des) < 0) {
		mvwprintw(repl->buffer, *line, 0, "error %s", strerror(errno));
        return;
	}

	if(pipe(stderr_des) < 0) {
		mvwprintw(repl->buffer, *line, 0, "error %s", strerror(errno));
        close_pipe(stdout_des);
        return;
	}
	
	int status;
	int pid = fork();
	if(pid < 0) { // error
		mvwprintw(repl->buffer, *line, 0, "error %s", strerror(errno));
        close_pipe(stdout_des);
        close_pipe(stderr_des);
		return;
	} else if(!pid) { // child process
		close(stdout_des[0]);
		close(stderr_des[0]);
		if(dup2(stdout_des[1], STDOUT_FILENO) < 0) {
			printf("%s\n", strerror(errno));
		}

		if(dup2(stderr_des[1], STDERR_FILENO) < 0) {
			printf("%s\n", strerror(errno));
		}
		close(stdout_des[1]);
		close(stderr_des[1]);

		if(execvp(args[0], args) < 0) {
			printf("%s\n", strerror(errno));
		}
		exit(1);
	} else { // parent process
		close(stdout_des[1]);
		close(stderr_des[1]);

		int nbytes = 0;
		while((nbytes = read(stdout_des[0], buf, sizeof(buf)-1)) != 0) {
			mvwprintw(repl->buffer, *line, 0, "%s", buf);
			for(size_t i = 0; buf[i] != '\0'; i++) {
				if(buf[i] == '\n') *line += 1;
			}
			refresh();
			memset(buf, 0, sizeof(buf));
		}

		while((nbytes = read(stderr_des[0], buf, sizeof(buf)-1)) != 0) {
			mvwprintw(repl->buffer, *line, 0, "%s", buf);
			for(size_t i = 0; buf[i] != '\0'; i++) {
				if(buf[i] == '\n') *line += 1;
			}
			refresh();
			memset(buf, 0, sizeof(buf));
		}
			
		int wpid = waitpid(pid, &status, 0);
		while(!WIFEXITED(status) && !WIFSIGNALED(status)) {
			wpid = waitpid(pid, &status, 0);
		}
		(void)wpid;
		
		close(stdout_des[0]);
		close(stderr_des[0]);

		refresh();
	}	
}
	
void handle_command(Repl *repl, char **args, size_t *line) {
	if(*args == NULL) {
		mvwprintw(repl->buffer, *line, 0, "error, no command\n");		
		return;
	}
	if(strcmp(args[0], "exit") == 0) {
		endwin();
		printf("exit\n");
		int exit_code = 0;
		if(args[1] != NULL) {
			for(size_t i = 0; args[1][i] != '\0'; i++) {
				if(!isdigit(args[1][i])) {
					fprintf(stderr, "numeric argument required - %s\n", args[1]);		
					exit(2);
				}
			}
			exit_code = strtol(args[1], NULL, 10);
		}
		exit(exit_code);
	} else if(strcmp(args[0], "cd") == 0) {
		char const *dir = "~";
		if(args[1] != NULL) {
			dir = args[1];
		}
		if(chdir(dir) < 0) {
			mvwprintw(repl->buffer, *line, 0, "`%s` %s", dir, strerror(errno));
			*line += 1;
		}
	} else {
		execute_command(repl, args, line);
	}
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
	while(cur != NULL) {	
		if(args_cur+2 >= args_s) {
			args_s *= 2;
			args = realloc(args, sizeof(char*)*args_s);
		}
		while(command[0] != '\0') command++;
		command++;
		assert(command);
		if(command[0] == '\'') {
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

int main(void) {
	return shell_repl_run();
}
