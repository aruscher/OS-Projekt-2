/*
 * KAC - Shell
 *
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#include "parser.h"
#include "shell.h"

#include <readline/readline.h>
#include <readline/history.h>

#include <dirent.h>

struct cmds *command;

/* variables to build prompt */
char *user;
char *host;
char *pwd;
char *pathvar;

/* exits shell if set to 1 */
int shell_exit = 0;

/* let's go */
int main(void) {
	char *line, *expansion;
	int result;

	/* read history */
	using_history();
	read_history(NULL);

	/* enables shell completion with tab*/
	rl_bind_key('\t', rl_complete);

	/* setting user variables for prompt */
	user = getenv("USER");
	host = getenv("HOSTNAME");
	pwd = getenv("HOME");
	pathvar = getenv("PATH");
	cmd_cd(pwd);

	/* print welcome message */
	printf("Welcome to KAC-Shell (type 'help' for command list)\n");
	/* loop until user enters 'exit' */
	for (; !shell_exit;) {
		/* print prompt */
		printf("%s@%s (%s)$ ", user, host, pwd);

		/* reads input from prompt */
		line = readline(NULL);

		/* add typed commands to history */
		result = history_expand(line, &expansion);

		if(result) {
			fprintf(stderr, "%s\n", expansion);
		}

		if(result < 0 || result == 2) {
			free(expansion);
			continue;
		}

		strncpy(line, expansion, sizeof(line) - 1);
		free(expansion);
		read_history(NULL);
		write_history(NULL);

		register HIST_ENTRY **history = NULL;
		register int i;

		if(history) {
			for(i = 0; history[i]; i++) {
				printf("%d: %s\n", i + history_base, history[i]->line);
			}
		}

		if(strlen(line) > 0) {
			add_history(line);
		}
		/* ***** */

		/* process input */
		process_input(line);

		/* free memory */
		free(line);
	}

	/* success */
	return EXIT_SUCCESS;
}

void process_input(char * line) {
	command = parser_parse(line);

	FILE *pipe;
	char *sys;
	int i;

	/* loop until command is NULL */
	while(command != NULL) {
		switch(command->kind) {
		case CD: /* command CD */
			/* change working directory */
			if(strcmp(command->cd.path, "--help") == 0) {
				print_cmd_help("cd");
				break;
			}

			if(cmd_cd(command->cd.path) == CMD_SUCCESS) {
				strcpy(pwd, getcwd(NULL, 0));
			} else {
				perror("no such directory");
				print_cmd_help("cd");
			}
			break;
		case ENV: /* set/unset environment variables */
			/* set environment variables */
			if(command->env.value != NULL) {
				setenv(command->env.name, command->env.value, 1);
			} else {
				unsetenv(command->env.name);
			}
			break;
		case JOB: /* job specific stuff fg/bg/... */
			printf("JOB");
			break;
		case PROG: /* execute command */
			if(strcmp(command->prog.argv[0], "help") == 0) {
				print_cmd_help("");
			} else if(command->prog.output != NULL) {
				/* pipe output from prog to file */
				sys = command->prog.argv[0];

				for(i = 1; i < command->prog.argc; i++) {
				   strcat(sys, " ");
				   strcat(sys, command->prog.argv[i]);
				}

				if(!(pipe = (FILE*)popen(sys,"r")) ) {
				   perror("Problems with pipe");
				   exit(1);
				}

				while(fgets(line, sizeof(line), pipe)) {
					FILE *file;
					file = fopen(command->prog.output,"a+");
					fprintf(file,"%s",line);
					fclose(file);
				}
				pclose(pipe);
			} else if(command->prog.input != NULL) {
				/* read from file as input for prog*/
			} else if(strcmp(command->prog.argv[0], "echo") == 0) {
				/* echo given arguments to prompt */
				char *str;
				if(command->prog.argc > 1) {
					str = command->prog.argv[1];
					for(i = 2; i < command->prog.argc; i++) {
						strcat(str, " ");
						strcat(str, command->prog.argv[i]);
					}
					cmd_echo(str);
				}
			} else if(strcmp(command->prog.argv[0], "ls") == 0) {
				/* list directory */
				if(command->prog.argc > 1) {
					if(strcmp(command->prog.argv[1], "--help") == 0) {
						print_cmd_help("ls");
					} else {
						cmd_ls(pwd, command->prog.argv[1]);
					}
				} else {
					cmd_ls(pwd, "");
				}

			} else {
	//			execlp(line, pathvar, (char *) 0);
			}
			break;
		case PIPE: /* pipe */
			printf("PIPE");

			break;
		case EXIT: /* exit shell */
			printf("Bye!...\n");
			cmd_exit();
			break;
		default:
			break;
		}
		/* switch to next command in list */
		command = command->next;
	}
}

/*
 * get error string for given errno
 */
char *err2str(int err_code) {
	if(NULL != shell_error_msg[err_code]) {
		return shell_error_msg[err_code];
	}

	return (char *) NULL;
}

void shell_error(int err, const char *fmt, ...) {
	va_list vargs;
	va_start(vargs, fmt);
	vprintf(fmt, vargs);
	va_end(vargs);

	if(NULL != err2str(err)) {
		printf(" (%s)\n", err2str(err));
	} else {
		printf("unknown error accured (%d)\n", err);
	}
}

char *command2string(int argc, char **argv) {
	char *ret;
	int i;

	ret = argv[0];
	for(i = 1; i < argc; i++) {
		strcat(ret, " ");
		strcat(ret, argv[i]);
	}
	strcat(ret, " 2>&1");

	return ret;
}

void print_cmd_help(char *cmd) {
	if(strcmp(cmd, "cd") == 0) {
		printf("Usage: cd PATH \n");
		printf("PATH\t path to change\n\n");
		printf("Options:\n");
		printf("--help\t print help\n");
	} else if(strcmp(cmd, "ls") == 0) {
		printf("prints out files in current directory\n");
		printf("Usage: ls [OPTIONS] \n\n");
		printf("Options:\n");
		printf("-l\t print out files as list\n");
		printf("--help\t print help\n");
	} else {
		printf("KAC-Shell Help Index\n");
		printf("Commands:\n");
		printf("cd\t change directory\n");
		printf("ls\t prints out content of current directory\n");
		printf("echo\t prints out following arguments\n");
		printf("help\t print general help\n");
		printf("exit\t exit shell\n");
	}
}

int cmd_exit() {
	shell_exit = 1;
	return CMD_SUCCESS;
}

int cmd_cd(char *path) {
	if(chdir(path) != -1) {
		return CMD_SUCCESS;
	}
	return CMD_ERROR;
}

int cmd_echo(char *str) {
	printf("%s\n", str);
	return CMD_SUCCESS;
}

int cmd_ls(char *path, char *mode) {
	DIR *dir;
	struct dirent *ent;

	dir = opendir(pwd);

	if(dir != NULL) {
		while((ent = readdir(dir)) != NULL) {
			if(strcmp(mode, "-l") == 0) {
				printf ("%d\t %s\n", ent->d_type, ent->d_name);
			} else {
				printf ("%s\t", ent->d_name);

			}

		}
		closedir(dir);
		return CMD_SUCCESS;
	} else {
		/* raise error if directory is not present */
		perror("");
	}
	return CMD_ERROR;
}
