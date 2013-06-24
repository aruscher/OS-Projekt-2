#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <pwd.h>


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

/* switch case with a printout of the message */
/* lecture 3 -> Page 10: "Signale" */
/* http://openbook.galileocomputing.de/shell_programmierung/shell_009_000.htm */
void signal_handler(int signal)  
{

	switch( signal )
	{
		case SIGHUP:
			printf("Fehler: Die Verbindung wurde beendet.\r\n");
			break;

		case SIGINT:
			/* ignore because this shell shouldn't be interrupted */
			/* children will be interruptable */
			break;

		case SIGQUIT:
			printf("Hinweis: Quit-Signal erhalten. Shell wird beendet.\r\n");
			break;

		case SIGILL:
			printf("Fehler: Illegale Operation.\r\n");
			break;

		case SIGTRAP:
			printf("Hinweis: Überwachungs oder Stoppunkt erreicht.\r\n");
			return;

		case SIGABRT:
			printf("Hinweis: Abbruchssignal erhalten.\r\n");
			break;

		case SIGBUS:
			printf("Fehler: Unerwarteter Busfehler.\r\n");
			break;

		case SIGCHLD:
			printf("Hinweis: Der Status eines Kind-Prozess hat sich geändert.\r\n" );
			return;

		case SIGFPE:
			printf("Fehler: Fließkomma- Überschreitung.\r\n");
			break;

		case SIGKILL:
			printf("Hinweis: Beendigungssignal erhalten.\r\n");
			break;

		case SIGUSR1:
			printf("Hinweis: Benutzerdefiniertes Signal 1 erhalten.\r\n");
			return;

		case SIGSEGV:
			printf("Fehler: Ungueltige Speicherreferenz.");
			break;

		case SIGUSR2:
			printf("Hinweis: Benutzerdefiniertes Signal 2 erhalten.\r\n");
			return;

		case SIGPIPE:
			printf("Fehler: Die Pipe-Kommunikation wurde unterbrochen.");
			return;

		case SIGALRM:
			printf("Hinweis: Zeitsignal erhalten.\r\n");
			return;

		case SIGTERM:
			printf("Hinweis: Beendigungssignal erhalten.\r\n");
			break;

		case SIGSTKFLT:
			printf("Fehler: Stack des Coprozessors korrumpiert.\r\n");
			break;

		case SIGCONT:
			printf("Hinweis: Prozess wird fortgefuehrt.\r\n");
			return;

		case SIGSTOP:
			printf("Hinweis: Prozess wird gestoppt.\r\n");
			return;

		case SIGTSTP:
			printf("Hinweis: Stop soll an ein Kind gesendet werden.\r\n");
			return;

		case SIGTTIN:
			printf("Hinweis: Hintergrundprozess liest von TTY.\r\n");
			return;

		case SIGTTOU:
			printf("Hinweis: Hintergrundprozess schreibt an TTY.\r\n");
			return;


		case SIGIO:
			printf("Hinweis: Ein-/Ausgabe ist wieder verfügbar.\r\n");
			return;

		case SIGXCPU:
			printf("Fehler: CPU ist ueberlastet.\r\n");
			break;

		case SIGXFSZ:
			printf("Fehler: Dateigroesse hat ihr Limit ueberschritten.\r\n");
			break;

		case SIGVTALRM:
			printf("Hinweis: Vitueller Zeitalarm.\r\n");
			return;

		case SIGPROF:
			printf("Hinweis: Profil- Zeitalarm ist aufgetreten.\r\n");
			return;

		case SIGWINCH:
			printf("Hinweis: Fenstergroesse wurde veraendert.\r\n");
			return;

		case SIGPWR:
			printf("Hinweis: Neustart nach Energieversorgungsfehler ist aufgetreten.\r\n");
			return;

		case SIGSYS:
			printf("Fehler: Falscher Systemaufuf.\r\n");
			break;

		default:
			printf( "Hinweis: Unbekanntes Signal: %d\r\n", signal );
			return;
	}	

	exit(signal);
}

/* lecture 3 -> Page 14pp: "Beispielprogramm" */
void setup_signal_handler(int signal, void (*handler)(int))
{
	struct sigaction sa;
		/* The sigemptyset() function initialises the signal set pointed to by set. */
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = handler;

		/* The sigaction() system call is used to change the action taken by a process on receipt of a specific signal. */
	if (sigaction(signal, &sa, NULL) == -1) {
		perror("installing signal handler failed");
		exit(EXIT_FAILURE);
	}
}

void setup_remaining_signal_handlers(void)
{
	/* SIGINT intentionally missing from this list */
	/* SIGKILL and SIGSTOP can't be caught */

	int signals[] = { SIGHUP, SIGQUIT, SIGILL, SIGTRAP, SIGABRT, SIGFPE,
		SIGUSR1, SIGSEGV, SIGUSR2, SIGPIPE, SIGALRM, SIGTERM, SIGCHLD,
		SIGCONT, SIGTSTP, SIGTTIN, SIGTTOU, //man kill
	};
	
	
	const unsigned int signals_count = sizeof(signals)/sizeof(int);
	
	unsigned int i;
	
	for (i=0; i<signals_count; ++i) {
	setup_signal_handler(signals[i], signal_handler);
	}
}



/* everything starts here */
int main(void) 
{
    /* ignore SIGINT */
    setup_signal_handler(SIGINT, SIG_IGN);
    setup_remaining_signal_handlers();

	char * line;

	/* variables to build prompt */
	char *user;
	char hostname[256];

	/* create custom prompt text */
	prompt = malloc(1024);
	memset(prompt, 0, 1024); //??

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
				if(command->prog.output) //CMD > file
				{	
					FILE *file;
					file=fopen((command->prog.output),"w");	// Datei neu/überschreiben
					if(file)
					{	
						char* fileCmd = command->prog.argv[0]; // arg value 0 speichern
						int i;
						for(i = 1; i < command->prog.argc; i++) // rest anhängen
						{
						   strcat(fileCmd, " ");
						   strcat(fileCmd, command->prog.argv[i]);
						}
						//read unix pipe.
						FILE *pipe = popen(fileCmd,"r");
						if(pipe)
						{
							/*while((fscanf(pipe,"%500s",line)) != EOF)
								fprintf(file,"%s\n",line);*/
							while(fgets(line, sizeof(line), pipe))
								fprintf(file,"%s",line);
							pclose(pipe);
							printf("Wrote to file.\n");
						}
						else
						   perror("Couldn't execute command.");

						fclose(file);
					}
					else
					{
						perror("Couldn't write to file\n");
					}
				}
				else if(command->prog.input) //CMD < file
				{	printf("input\n");				
					//execl(cwd, line, (char *) 0);
				}
				else if(command->prog.background)
				{	
					char* fileCmd = command->prog.argv[0]; // arg value 0 speichern
					int i;
					for(i = 1; i < command->prog.argc; i++) // rest anhängen
					{
					   strcat(fileCmd, " ");
					   strcat(fileCmd, command->prog.argv[i]);
					}
					printf("%s\n",fileCmd);
					//read unix pipe.
					FILE *pipe = popen(fileCmd,"r");
					if(pipe)
					{	printf("Execute in background ok.\n");
						pclose(pipe);
					}
					else
						perror("Couldn't execute command in background\n");
				}
				else
					("Couldn't identify program.\n");
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
