#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <sys/wait.h>

#include <ncurses.h>

#define ctrl(x) ((x) & 0x1f)
#define SHELL "[canosh]$ "
#define ENTER 10
#define UP_ARROW 259
#define DOWN_ARROW 258
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
	
void clear_line(size_t line) {
	for(size_t i = sizeof(SHELL)-1; i < sizeof(SHELL)-1+32; i++) mvprintw(line, i, " ");
}
	
void handle_command(char **args, size_t *line) {
	char buf[4096] = {0};
	int filedes[2];
	if(pipe(filedes) < 0) {
		printw("error %s\n", strerror(errno));
	}
	
	int status;
	int pid = fork();
	if(pid < 0) { // error
		printw("error %s\n", strerror(errno));
		return;
	} else if(!pid) { // child process
		close(filedes[0]);
		if(dup2(filedes[1], STDOUT_FILENO) < 0) {
			printw("error %s\n", strerror(errno));
		}
		close(filedes[1]);
		
		if(execvp(args[0], args) < 0) {
			endwin();
			fprintf(stderr, "error %s\n", strerror(errno));
			exit(1);
		}
	} else { // parent process
		close(filedes[1]);
		int wpid = waitpid(pid, &status, 0);
		while(!WIFEXITED(status) && !WIFSIGNALED(status)) {
			wpid = waitpid(pid, &status, 0);
		}
		int nbytes = read(filedes[0], buf, sizeof(buf)-1);
		if(nbytes < 0) {
			printw("error %s\n", strerror(errno));
		}
		close(filedes[0]);
		mvprintw(*line, 0, "%s", buf);
		for(size_t i = 0; buf[i] != '\0'; i++) {
			if(buf[i] == '\n') *line += 1;
		}
		refresh();
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
		if(args_cur >= args_s) {
			args_s *= 2;
			args = realloc(args, sizeof(char*)*args_s);
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
	
	bool QUIT = false;
	while(!QUIT) {
		clear_line(line);
		
		mvprintw(line, 0, SHELL);
		mvprintw(line, sizeof(SHELL)-1, "%.*s", (int)command.count, command.data);
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
					//line++;
					handle_command(args, &line);
					DA_APPEND(&command_his, command);
					if(command_his.count > command_max) command_max = command_his.count;
				}
				command = (String){0};
				break;
			case UP_ARROW:
				if(command_his.count > 0) {
					command_his.count--;
					command = command_his.data[command_his.count];
				}
				break;
			case DOWN_ARROW:
				if(command_his.count < command_max) {
					command_his.count++;
					command = command_his.data[command_his.count];
				}
				break;
			default:
				DA_APPEND(&command, ch);
				break;
		}
	}
	
	refresh();
	
	endwin();
	for(size_t i = 0; i < command_his.count; i++) {
		free(command_his.data[i].data);
	}
	return 0;
}
