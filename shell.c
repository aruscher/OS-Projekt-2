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

//struct cmds* command; //TODO: struct/static??
static cmds* command;

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
	char hostname[256];

	/* create custom prompt text */
	prompt = malloc(50);
	memset(prompt, 0, 50); //??

	//TODO: immer für prompt GETENV nutzen??

	/* setting user variables for prompt */
	// TODO: im home oder aktuellen verzeichnis starten?
	if (getcwd(cwd, sizeof(cwd)) == NULL)
           perror("getcwd() error");

	user = getenv("USER");
	gethostname(hostname,256);
	sprintf(prompt, "%s@%s:~%s$ ", user, hostname, cwd);

	printf("\nWilkommen!\n");

	/* continue until user signals exit i.e. writes 'quit' */
	for (; !quit;) {
		/* input cursor with prompt */
		line = readline(prompt);

		/* process input */
		processLine(line);
		
		if(parser_status != PARSER_OK) // ERROR-Messages des Parsers ausgeben
			printf("%s\n", parser_message);

		sprintf(prompt, "%s@%s:~%s$ ", user, hostname, cwd);

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

	// loop until there is no command left
	while(command != NULL) 
	{
		switch(command->kind) // found the kinds in parser.h: enum cmd_kind  
		{
			case EXIT: // check if user wants to quit
				quit = 1;
				printf("\nShell beendet.\n\n");
				return;
			/*case JOB :
				break;*/ //Aufgabe Option Prozesssynchronisation
			case CD: // change directory
				if(chdir(command->cd.path) == -1)
					perror("no such directory");
				else
				{	if (getcwd(cwd, sizeof(cwd)) == NULL)
	           				perror("getcwd() error");
				}
				break;
			case ENV :
				if(command->env.value != NULL) // set environment var
				{
					if((getenv(command->env.name)) != NULL)
						printf("Variable already exists\n");
					else
					{	setenv(command->env.name, command->env.value, 0); 
								// 0==NoOverwrite?
						printf("Variable set.\n");
					}
				}
				else if(command->env.name != NULL) // unset environment var
				{
					if((getenv(command->env.name)) != NULL)
					{
						unsetenv(command->env.name);
						printf("Variable unset.\n");
					}
					else
						printf("Variable does not exist.\n");
				}
				else
					printf("Bad arguments.\n");
				break;
			case PROG : 
				printf("start programm\n");
				break;
			/*case PIPE :
				//cmd->prog.next,cmd->prog,cmd->prog.input,cmd->prog.output
				//break;*/  //Aufgabe Option Pipes
			default:
				printf("command not found\n");
		}
		// get next command
		command = command->next;
	}
	parser_free(command);
}
