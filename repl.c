#include <stdlib.h>
#include <string.h>

#include <ncurses.h>

#include "cano_sh.h"
#define ctrl(x) ((x) & 0x1f)

static const char SHELL_PROMPT[] = "[canosh]$ ";
static const Repl REPL_INIT = { .is_running = true };

static
bool shell_repl_initialize(Repl *repl) {
    *repl = REPL_INIT;
	initscr();
	raw();
	noecho();

	// TODO: change pad size on resize
	size_t width;
	size_t height;
	getmaxyx(stdscr, height, width);
	repl->buffer = newpad(height*4, width);

	keypad(repl->buffer, TRUE);
	scrollok(repl->buffer, TRUE);
    return true;
}

static
void clear_line(WINDOW* window, size_t line, size_t width) {
	for (size_t i = SSTR_LEN(SHELL_PROMPT); i < width - SSTR_LEN(SHELL_PROMPT); i++)
		mvwprintw(window, line, i, " ");
}

static
bool shell_readline(Repl *repl)
{
	String command = repl->input;
	size_t buf_height;
	size_t buf_width;
	size_t height;
	size_t width;
	size_t line = repl->line;
	size_t position = 0;
	size_t command_max = repl->command_his.count;
	int ch;
	size_t top_row = 0;
	
	command.count = 0;
	while (true) {
		getmaxyx(repl->buffer, buf_height, buf_width);
		if(line >= buf_height/2) {
			wresize(repl->buffer, buf_height*2, buf_width);
		}
			
		getmaxyx(stdscr, height, width);
		clear_line(repl->buffer, line, width);
		
		if(line >= height) top_row = line - height+1;
		mvwprintw(repl->buffer, line, 0, "%s%.*s", SHELL_PROMPT, (int)command.count, command.data);
		
		if (position > command.count)
			position = command.count;

		wmove(repl->buffer, line, SSTR_LEN(SHELL_PROMPT) + position);
		prefresh(repl->buffer, top_row, 0, 0, 0, height-1, width-1);

		ch = wgetch(repl->buffer);
		switch (ch) {
			case KEY_RESIZE:
				getmaxyx(stdscr, height, width);
				wresize(repl->buffer, buf_height, width);
				break;
			case KEY_ENTER:
			case '\n': {
				line += 1 + (command.count+SSTR_LEN(SHELL_PROMPT))/width;
				if (command.count == 0)
					continue;
				repl->line = line;
				repl->input = command;
			} return true;
			case ctrl('d'):
			case ctrl('q'):
				repl->is_running = false;
				return true;
			case ctrl('a'):
				position = 0;
				break;
			case ctrl('e'):
				position = command.count;
				break;
			case ctrl('k'):
				if(repl->clipboard.capacity < command.count-position) {
					repl->clipboard.capacity = command.count*2;
					repl->clipboard.count = command.count-position;
					repl->clipboard.data = realloc(repl->clipboard.data, sizeof(char)*repl->clipboard.capacity);
					if(repl->clipboard.data == NULL) {
						endwin();
						repl->is_running = false;
						fprintf(stderr, "Could not allocate space for clipboard\n");
						return 0;
					}
				}
				strncpy(repl->clipboard.data, &command.data[position], sizeof(char)*(command.count-position));
				command.count = position;
				break;
			case ctrl('y'): 
				for(size_t i = 0; i < repl->clipboard.count; i++) {
					ch = repl->clipboard.data[i];
					DA_APPEND(&command, ch);
					memmove(&command.data[position+1], &command.data[position], command.count - 1 - position);
					command.data[position++] = ch;
				}
				break;
			case ctrl('c'):
				line++;
				command.count = 0;
				break;
			case KEY_BACKSPACE:
				if (command.count > 0)
					command.data[--command.count] = '\0';
				break;
			case KEY_LEFT:
				if (position > 0)
					position--;
				break;
			case KEY_RIGHT:
				position++;
				break;
			case KEY_UP:
				if (repl->command_his.count > 0) {
					repl->command_his.count--;
					command = repl->command_his.data[repl->command_his.count];
					position = command.count;
				}
				break;
			case KEY_DOWN:
				if (repl->command_his.count < command_max) {
					repl->command_his.count++;
					command = repl->command_his.data[repl->command_his.count];
					position = command.count;
				}
				break;
			default:
				DA_APPEND(&command, ch);
				memmove(&command.data[position+1], &command.data[position], command.count - 1 - position);
				command.data[position++] = ch;
				break;
		}
	}
}

static
bool shell_evaluate(Repl *repl)
{
	char **args = parse_command(str_to_cstr(repl->input));

	if (args != NULL) {
		handle_command(repl, args, &repl->line);
		DA_APPEND(&repl->command_his, repl->input);
	}
	repl->input = (String){ 0 };
	return true;
}

int shell_repl_run(void)
{
    Repl repl = { 0 };

    if (!shell_repl_initialize(&repl))
        return EXIT_FAILURE;
    while (repl.is_running) {
        if (!shell_readline(&repl))
            continue;
        if (!shell_evaluate(&repl))
            break;
    }
	endwin();
	echo();
    return EXIT_SUCCESS;
}
