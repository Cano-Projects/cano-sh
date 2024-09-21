#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <readline/history.h>
#include <readline/readline.h>

#include "cano_sh.h"
#define ctrl(x) ((x) & 0x1f)

static const char SHELL_PROMPT[] = "[canosh]$ ";
static const Repl REPL_INIT = { .is_running = true };

#define export __attribute__((visibility("default")))

bool export shell_repl_initialize(Repl *repl) {
    *repl = REPL_INIT;
    return true;
}


void export shell_cleanup(Repl *repl)
{
	free(repl->input);
}

bool export shell_readline(Repl *repl)
{
	if (repl->input != NULL)
		free(repl->input);
	repl->input = readline(SHELL_PROMPT);
	// sending `^D` will cause readline to return NULL 
	if (repl->input == NULL) {
		repl->is_running = false;
		return false;
	}
	if(strlen(repl->input) > 0)
		add_history(repl->input);
	return true;
}

bool export shell_evaluate(Repl *repl)
{
	bool has_cmd = false;
	char **args;

	for (char *p = repl->input; !has_cmd && *p != '\0'; p++)
		has_cmd |= !isspace(*p);
    if (strlen(repl->input) < 1 || !has_cmd)
		return true;

	args = parse_command(repl->input);
	if (args == NULL)
		return false;
	handle_command(args);
	//DA_APPEND(&repl->command_his, repl->input);
	free(args);
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
	shell_cleanup(&repl);
	// repl should be running at this point unless we reach a memory error
    return repl.is_running ? EXIT_SUCCESS : EXIT_FAILURE;
}
