/*
 * shell project by Christian Delfs, Andreas Ruscheinski, Fabienne Lambusch
 *
 * to execute install readline & ncurses libraries
 * and compile with "gcc shell.c parser.c -o shell -lreadline -lncurses"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "parser.h"

struct cmds *command;

char cwd[1024];

/* prototype -> processes read in line */
void processLine(/*const*/ char *);

/* flag to signal exit */
static unsigned char quit = 0;

/* prompt text */
static char * prompt = NULL;


/* everything starts here */
int main(void) 
{
	char * line;

	/* variables to build prompt */
	char *user;
	char *host;

	/* create custom prompt text */
	prompt = malloc(50);
	memset(prompt, 0, 50); //??

	/* setting user variables for prompt */	 //IMMER GETENV nutzen?
	if (getcwd(cwd, sizeof(cwd)) == NULL)
           perror("getcwd() error");

	user = getenv("USER");
	host = getenv("HOSTNAME");
	if (host==NULL)
	{	char hostname[256];
		gethostname(hostname,256);
		sprintf(prompt, "%s@%s:~%s$ ", user, hostname, cwd);
	}
	else
		sprintf(prompt, "%s@%s:~%s$ ", user, host, cwd);

	printf("\nWilkommen!\n");

	/* continue until user signals exit i.e. writes 'quit' */
	for (; !quit;) {
		/* input cursor with prompt */
		line = readline(prompt);

		/* process input */
		processLine(line);

		sprintf(prompt, "%s@%s:~%s$ ", user, host, cwd);

		/* sanity checks for history i.e. actual text in line */
		if (line && *line) {
			/* add line to history */
			add_history(line);
		}

		/* return heap space i.e. readline dynamically allocates space for input */
		free(line);
	}

	/* success */
	return EXIT_SUCCESS;
}

/* process input stored in line i.e. print line */
void processLine(/*const*/ char * line) 
{
	command = parser_parse(line);

	/* print length and content of line */
	//printf("%zd - %s\n", strlen(line), line);

	/* check if user wants to quit */
	if (!strcmp("exit", line)) {
		quit = 1;
		printf("\nBis bald!\n\n");
		return;
	}

	switch(command->kind) 
	{
		case CD:
			if(chdir(command->cd.path) == -1)
				perror("no such directory");
			else
			{
       				if (getcwd(cwd, sizeof(cwd)) == NULL)
           				perror("getcwd() error");
			}
			break;
		default:
			printf("command not found\n");
	}
}
