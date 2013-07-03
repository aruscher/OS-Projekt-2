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
 * and compile with makefile or "gcc shell.c parser.c -o shell -lreadline -lncurses"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "parser.h"


#define READ_END 0
#define WRITE_END 1

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
	{//TODO: break durch return ersetzen bzw. exit(..) entfernen?
		case SIGHUP:
			printf("Fehler: Die Verbindung wurde beendet.\r\n");
			break;

		case SIGINT:
			// ignore because this shell shouldn't be interrupted (but in foreground prog)
			printf("^C\n");
			break;

		case SIGQUIT:
			printf("Hinweis: Quit-Signal erhalten. Shell wird beendet.\r\n");
			break;

		case SIGILL:
			printf("Fehler: Illegale Operation.\r\n");
			break;

		case SIGTRAP:
			printf("Hinweis: Überwachungs- oder Stoppunkt erreicht.\r\n");
			return;

		case SIGABRT:
			printf("Hinweis: Abbruchssignal erhalten.\r\n");
			break;

		case SIGBUS:
			printf("Fehler: Unerwarteter Busfehler.\r\n");
			break;

		case SIGCHLD:
			//printf("Hinweis: Der Status eines Kind-Prozesses hat sich geändert.\r\n" );
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
			printf("Fehler: Ungültige Speicherreferenz.\n");
			break;
            //return;

		case SIGUSR2:
			printf("Hinweis: Benutzerdefiniertes Signal 2 erhalten.\r\n");
			return;

		case SIGPIPE:
			printf("Fehler: Die Pipe-Kommunikation wurde unterbrochen.\n");
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
	printf("exit in signal handler"); //TODO: wird nicht ausgegeben bei strg+c??
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

	/* sigaction() is used to change the action taken by a process on receipt of a specific signal. */
	if (sigaction(signal, &sa, NULL) == -1) {
		perror("installing signal handler failed");	printf("exit install");
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

void closePipe(int *fd){
    close(fd[0]);
    close(fd[1]);
}

int systemProg(prog_args* cmd){
	char exe1[256];
	char exe2[256];
	char *input = cmd->argv[0]; //sys prog
	sprintf(exe1,"/usr/bin/%s",input); //alternative part
	sprintf(exe2,"/bin/%s",input);
	//printf("EXE:%s\n",exe1);
	char *arguments[256];
	int i;
	int error;
	int status;
	pid_t pid;
	for(i = 0; i < cmd->argc; i++) // collect args
		{
			arguments[i]=cmd->argv[i];//printf("argument[%d]:%s\n",i,arguments[i]);
		}
	//special case ls without arguments -> show current dir
	if(strcmp(input,"ls")==0 && cmd->argc == 1)
		{
			arguments[0] = "ls";
			arguments[1] = ".";
			arguments[2] = NULL;
		}
	//printf("After Argu\n");
	arguments[i+1]=NULL;
	switch (pid=fork()) //fork for returning to parent process
		{
			case -1: perror("fork");
			case 0: 
				{	printf("SystemProg case 0\n");

 					error = execv(exe1,arguments);
					if(error!=-1) {
							exit(666);
						}
					//printf("Exe2:%s\n",exe2);
					error = execv(exe2,arguments);
					if(error==-1){
							printf("Can't find programm\n");
						}
					exit(742);
				}
			default: //parent waits for exit in child
				{	printf("SystemProg default\n");

					wait(&status);
					break;
					}
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

	/* setting user variables for prompt */
	if (getcwd(cwd, sizeof(cwd)) == NULL)
           perror("getcwd() error");

	user = getenv("USER");
	gethostname(hostname,256);
	sprintf(prompt, "%s@%s:~%s$ ", user, hostname, cwd);

	printf("\nWilkommen!\n");

	/* continue until user signals exit i.e. writes 'quit' */
	while(!quit) {
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
		{printf("\nswitch\n\n");
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
			{	pid_t child_pid = fork(); //Start in own process

				if (child_pid == -1) //failure
				{	perror("fork failed"); break; }

				if (child_pid == 0) // child process
				{
					if (!command->prog.background) // foreground program
						setup_signal_handler(SIGINT, SIG_DFL); //enable interrupt

        				if (command->prog.input) // Input-File -> set as standard in
						freopen (command->prog.input, "r", stdin);

        				if (command->prog.output) // Output-File -> set as standard out
						freopen (command->prog.output, "w", stdout);

        				// there has to be Null in the end
        				command->prog.argv[command->prog.argc] = NULL;

        				// execute program
					execvpe(command->prog.argv[0], command->prog.argv, NULL);

					//Couldn't execute
        				perror("Couldn't find program");
					exit(742);
    				}
				else // parent process
				{
					// foreground process
					if (!command->prog.background) 
					{
            					// wait for child to finish
            					int status = 0;
            					if (waitpid(child_pid, &status, 0) == -1) 
	                				perror("Child process not finish");
					}
				}
				break;
			}
			case PIPE :
				{//cmd->prog.next,cmd->prog,cmd->prog.input,cmd->prog.output
				//break;*/  //Aufgabe Option Pipes
				pid_t child1; //child1
				pid_t child2; //child2
				int fd[2];
				pipe(fd);
				int status;
				char buffer[10000];
				//parent read and child write!!!!
				prog_args* test = command->prog.next->next;
				if (test == NULL){
					//only one pipe
					//printf("SINGLE\n");
					if((child1=fork())<0){
						perror("fork");
						exit(-1);
					}
					if(child1==0){
						dup2(fd[WRITE_END], 1);
						closePipe(fd);
						execvp(command->prog.argv[0], command->prog.argv);
					} 
					child2 = fork();
					if(child2==0) {
						dup2(fd[READ_END],0);
						closePipe(fd);
						execvp(command->prog.next->argv[0],command->prog.next->argv);
					}
					closePipe(fd);
					waitpid(child1,NULL,0);
					waitpid(child2,NULL,0);
					break;
				} else {
					//multiple pipes
					printf("MULTIPIP\n");
                }
                
				break;}
			default:
				printf("Command not found\n");
		}
		// get next command
		command = command->next;
	}
	parser_free(command);
}
