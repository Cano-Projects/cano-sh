#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>

#include <sys/wait.h>

#include <ncurses.h>

#define ctrl(x) ((x) & 0x1f)
#define SHELL "[canosh]$ "
#define ENTER 10
#define UP_ARROW 259
#define DOWN_ARROW 258
#define LEFT_ARROW 260
#define RIGHT_ARROW 261
#define DATA_START_CAPACITY 128

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
        void *new = calloc(((da)->capacity+1), sizeof(*(da)->data));                    \
        ASSERT(new,"outta ram");                                                       \
        memcpy(new, (da)->data, (da)->count);                                          \
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
	
void clear_line(size_t line, size_t width) {
	for(size_t i = sizeof(SHELL)-1; i < width-sizeof(SHELL)-1; i++) mvprintw(line, i, " ");
}

void execute_command(char **args, size_t *line) {
	char buf[4096] = {0};
	int stdout_des[2];
	int stderr_des[2];
	if(pipe(stdout_des) < 0) {
		mvprintw(*line, 0, "error %s", strerror(errno));
	}

	if(pipe(stderr_des) < 0) {
		mvprintw(*line, 0, "error %s", strerror(errno));
	}
	
	int status;
	int pid = fork();
	if(pid < 0) { // error
		mvprintw(*line, 0, "error %s", strerror(errno));
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
			mvprintw(*line, 0, "%s", buf);
			for(size_t i = 0; buf[i] != '\0'; i++) {
				if(buf[i] == '\n') *line += 1;
			}
			refresh();
			memset(buf, 0, sizeof(buf));
		}

		while((nbytes = read(stderr_des[0], buf, sizeof(buf)-1)) != 0) {
			mvprintw(*line, 0, "%s", buf);
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

		refresh();
	}	
}
	
void handle_command(char **args, size_t *line) {
	if(*args == NULL) {
		mvprintw(*line, 0, "error, no command\n");		
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
		char *dir = "~";
		if(args[1] != NULL) {
			dir = args[1];
		}
		if(chdir(dir) < 0) {
			mvprintw(*line, 0, "`%s` %s", dir, strerror(errno));
			*line += 1;
		}
	} else {
		execute_command(args, line);
	}
}

	
char *str_to_cstr(String str) {
	char *cstr = malloc(sizeof(char)*str.count+1);
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

int main() {
	initscr();
	raw();
	noecho();
	keypad(stdscr, TRUE);
	
	String command = {0};
	Strings command_his = {0};
	
	int ch;
	size_t line = 0;
	size_t command_max = 0;
	size_t command_pos = 0;
	
	size_t height = 0;
	(void)height;
	size_t width = 0;
	
	bool QUIT = false;
	while(!QUIT) {
		getmaxyx(stdscr, height, width);
		clear_line(line, width);
		
		mvprintw(line, 0, SHELL);
		mvprintw(line, sizeof(SHELL)-1, "%.*s", (int)command.count, command.data);
		
		move(line, sizeof(SHELL)-1+command_pos);

		ch = getch();
		switch(ch) {
			case ctrl('q'):
				QUIT = true;
				break;
			case KEY_ENTER:
			case ENTER:
				line++;
				char **args = NULL;
				if(command.count > 0) {				
					args = parse_command(str_to_cstr(command));
				}
				mvprintw(line, command.count, "\n\r");
				if(args != NULL) {
					handle_command(args, &line);
					DA_APPEND(&command_his, command);
					if(command_his.count > command_max) command_max = command_his.count;
				}
				command = (String){0};
				break;
			case ctrl('c'):
				line++;
				command = (String){0};
				break;
			case KEY_BACKSPACE:
				if(command.count > 0) command.data[--command.count] = '\0';
				break;
			case LEFT_ARROW:
				if(command_pos > 0) {
					command_pos--;
				}
				break;
			case RIGHT_ARROW:
				if(command_pos < command.count) {
					command_pos++;
				}
				break;
			case UP_ARROW:
				if(command_his.count > 0) {
					command_his.count--;
					command = command_his.data[command_his.count];
					command_pos = command.count;
				}
				break;
			case DOWN_ARROW:
				if(command_his.count < command_max) {
					command_his.count++;
					command = command_his.data[command_his.count];
					command_pos = command.count;
				}
				break;
			default:
				if(command_pos > command.count) command_pos = command.count;
				DA_APPEND(&command, ch);
				memmove(&command.data[command_pos+1], &command.data[command_pos], command.count - 1 - command_pos);
				command.data[command_pos++] = ch;
				break;
		}
		if(command_pos > command.count) command_pos = command.count;
	}
	
	refresh();
	
	endwin();
	for(size_t i = 0; i < command_his.count; i++) {
		free(command_his.data[i].data);
	}
	return 0;
}
