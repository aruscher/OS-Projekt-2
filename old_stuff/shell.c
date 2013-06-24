#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <pwd.h>

#include "parser.h"

extern char **environ;

void setup_input_redirection(const char *file)
{
    int fh;

    if ((fh = open(file, O_RDONLY)) == -1) { //Open for reading only. 
        perror("input redirection failed");
        exit(EXIT_FAILURE);
    }

    if (dup2(fh, STDIN_FILENO) == -1) {    //duplicate
        perror("input redirection failed");
        exit(EXIT_FAILURE);
    }
}

void setup_output_redirection(const char *file)
{
    int fh;

    if ((fh = open(file, O_WRONLY)) == -1) {	
        perror("output redirection failed");
        exit(EXIT_FAILURE);
    }

    if (dup2(fh, STDOUT_FILENO) == -1) {
        perror("output redirection failed");
        exit(EXIT_FAILURE);
    }
}



void signal_handler(int signal)  //switch case with a printout of the message
				//maybe we break and exit with the error code or we just return
{

	switch( signal )
	{
	       case SIGHUP:
		   	printf("Fehler: Die Verbindung wurde beendet.\r\n");
		   	break;

	       case SIGINT:
			//ignore because our bash shouldn't be interrupted
			//however the children will be interruptable
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

		case SIGCHLD: //maybe this is a little annoying 
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
			printf("Fehler: Ungueltige Speicherreferenz.\r\n");
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

		case SIGURG:
			printf("Hinweis: Urgent bedingung am Socket.\r\n" );
			return;

		default:
			printf( "Hinweis: Unbekanntes Signal: %d\r\n", signal );
			return;
   }

	exit(signal);
}


//compare to lecture 4 pages 14-18
void setup_signal_handler(int signal, void (*handler)(int))
{
    struct sigaction sa;

    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = handler;

    if (sigaction(signal, &sa, NULL) == -1) {
        perror("installing signal handler failed");
        exit(EXIT_FAILURE);
    }
}

void setup_remaining_signal_handlers(void)
{
    // SIGINT intentionally missing from this list
    // SIGKILL and SIGSTOP can't be caught

    int signals[] = { SIGHUP, SIGQUIT, SIGILL, SIGTRAP, SIGABRT, SIGFPE,
        SIGUSR1, SIGSEGV, SIGUSR2, SIGPIPE, SIGALRM, SIGTERM, SIGCHLD,
        SIGCONT, SIGTSTP, SIGTTIN, SIGTTOU, //man kill
    };

    const unsigned int signals_count = sizeof(signals)/sizeof(int);

    for (unsigned int i=0; i<signals_count; ++i) {
        setup_signal_handler(signals[i], signal_handler);
    }
}


//this function deals with the corresponding data structure within parser.h
/*typedef struct prog_args
{
        char* input;   
	char* output;
	int background;                
	int argc;
	char** argv;               
	struct prog_args* next; 
} prog_args; */
//redirection takes place within the child process, therefor the parents in/output isn't changed
//the stderror channel is allways the same, so we can see errors on the console and they don't come up in the (normal) output stream
void execute_prog(const cmds* cmd)
{
    if (!cmd)
        return;

    assert(cmd->kind == PROG);

    /* run command in its own child process */
    pid_t child_pid = fork();

    if (child_pid == -1) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }

    if (child_pid == 0) { /* child */

        /* forground job should handle SIGINT (instead of ignoring it) */
        if (!cmd->prog.background) {
            setup_signal_handler(SIGINT, SIG_DFL); //interrupt - default
	  //jobs -- not used now
	  //  setup_signal_handler(SIGCONT, SIG_DFL);
        }

        /* since we did not ignore the remaining signals, they are reseted
         * during execve (man 7 signal) */

        /* Input redirection e.g.:"$ cat < input.txt" */
        if (cmd->prog.input) {
            setup_input_redirection(cmd->prog.input);
        }

        /* Output redirection "$ ls > output.txt" */
        if (cmd->prog.output) {
            setup_output_redirection(cmd->prog.output);
        }

        /* the following code depends on a NULL terminated array */
        assert(cmd->prog.argv[cmd->prog.argc] == NULL);

        /* replace the current (child) process with the new program */
        execvpe(cmd->prog.argv[0], cmd->prog.argv, environ);

        /* continues if execve() fails, e.g. prog.argv[0] was not found */
        perror("execute failed");
        exit(EXIT_FAILURE);
    }
    else { /* parent */

        /* wait for forground job */
        if (!cmd->prog.background) {

            /* wait for child to finish */
            int status = 0;
            if (waitpid(child_pid, &status, 0) == -1) {
                perror("waiting for child failed");
                exit(EXIT_FAILURE);
            }

            if (WIFEXITED(status)) {
                // returns true if the child terminated normally, <sys/wait.h>
            }
        }
    }
}

void execute_pipe(const cmds* cmd)
{
    // We decided to only support a maximum of 50 children in a pipe
    // if you feel like you need more children, you can end the first pipe with a document to write in and create another pipe whith the document as input
    //stderror is in every process the same, so we can see it on the console and it doesn't get into the next process of the pipe

    pid_t children[50] = { 0 };
    const unsigned int children_size = sizeof(children)/sizeof(pid_t);
    unsigned int children_counter = 0;

    int start = 1;
    int end;       //is it the last child?

    int pipe_prev[2];
    int pipe_next[2];

    const unsigned READ  = 0;
    const unsigned WRITE = 1;

    const prog_args *prog = &(cmd->prog);
    while (prog) {
        end = !prog->next;

        if (!end) { //if it is the last child there is no need for a new pipe
            if (pipe(pipe_next) == -1) {
                perror("pipe setup failed");
                exit(EXIT_FAILURE);
            }
        }

        children[children_counter] = fork();

        if (children[children_counter] == -1) {
            perror("fork failed");
            exit(EXIT_FAILURE);
        }

        if (children[children_counter] != 0) { // parent
            /* it is essential to close the file handles in the parent,
             * otherwise the processes in the pipe don't get end-of-file and
             * might thus run forever.
             */
            if (!end) {//if(end) there is no new pipe_next
                close(pipe_next[WRITE]);
                // don't close pipe_next[READ], next child will need it as
                // pipe_prev[READ]
            }
            if (!start) {  //if(start) {it is the first time we are here and there is no pipe_prev}
                close(pipe_prev[READ]);
            }
        }
        else { // child
            // gets pipe_prev[READ], pipe_next[READ], pipe_next[WRITE]

            if (!start) { //if(start) we want to read, as it is, form stdin by addressing stdin
                dup2(pipe_prev[READ], STDIN_FILENO);  //we want to read form the pipe by addressing stdin
                close(pipe_prev[WRITE]);    // never write to previous
            }
            else {
                // don't close pipe_prev, since it is not initialized, since we are the first process in the chain
            }

            if (!end) {
                close(pipe_next[READ]);    // never read from next
                dup2(pipe_next[WRITE], STDOUT_FILENO); //same here, we want to write to the pipe by addressing stout
            }
            else {
                // don't close pipe_next, since it was not set up, since we are the last
            }

            /* forground job should handle SIGINT (instead of ignoring it) */
            if (!prog->background) {
                setup_signal_handler(SIGINT, SIG_DFL); //interrupt default
	     //   //jobs -- not used now
	     //   setup_signal_handler(SIGCONT, SIG_DFL);
            }

            /* since we did not ignore the remaining signals, they are reseted
             * during execve (man 7 signal) */

            /* Input redirection "$ cat < input.txt" */
            if (start && prog->input) {
                setup_input_redirection(cmd->prog.input);
            }

            /* Output redirection "$ ls > output.txt" */
            if (end && prog->output) {
		setup_output_redirection(prog->output); //here was a bad copy-paste mistake cmp->prog.output kai 3.7.
            }

            /* replace the current (child) process with the new program */
            execvpe(prog->argv[0], prog->argv, environ);

            /* continues if execve() fails, e.g. prog.argv[0] was not found */
            perror("execute failed");
            exit(EXIT_FAILURE);
        }

        // move next to prev
        pipe_prev[0] = pipe_next[0];
        pipe_prev[1] = pipe_next[1];

        children_counter++;
        if (children_counter >= children_size) {
            fprintf(stderr, "Error: our shell only supports up to %d children in a pipe\n",
                    children_size);
        }

        start = 0;
        prog = prog->next;
    }

    prog = &(cmd->prog);
    while (prog->next) {
        prog = prog->next;
    } //get the last command ...

    /* wait for all children to complete */
    if (!prog->background) {  //last prog is not marked as background

        int finished = 0;
        while (!finished) {

            int status = 0;
            pid_t child = waitpid(-1, &status, 0);

            for (unsigned int i=0; i<children_size; ++i) {
                if (child == children[i]) {
                    children[i] = 0;
                    break;
                }
            }

            finished = 1;
            for (unsigned int i=0; i<children_size; ++i) {
                if (children[i]) {
                    finished = 0;
                    break;
                }
            }//if every child has ended finised will be == 1 
        }
    }
}




//the functions deals with the data structure of parser.h
/*enum cmd_kind  /* type of command (internal, external, or in a pipe)      
{
	EXIT,      /* builtin 'exit'                                          
	CD,        /* builtin 'cd [path]'                                     
	ENV,       /* builtin '[un]set variable [value]'                      
	JOB,       /* builtin 'jobs [id]', 'bg [id], and fg [id]'             
	PROG,      /* external command/program                                
	PIPE       /* external commands in a pipe                             
}; */
//it merely distributes the commands to the functions designed by us
int do_command(const cmds *cmd) 
{
    if (!cmd)
        return 0;

    switch (cmd->kind) {
        case EXIT:
            return 1;   // finished
            break;
        case CD://change directory from <unistd.h>
	    if(cmd->cd.path == NULL){
		chdir(getenv("HOME"));
	    }
            if (chdir(cmd->cd.path) == -1) {
                perror("Change directory");
                return 0;
            }
            break;
        case ENV:
            if (cmd->env.value != NULL) { /* setenv from <stdlib.h> */
                if (setenv(cmd->env.name, cmd->env.value, 1) == -1) {
                    perror("Set/change environment variable");
                    return 0;
                }
            }
            else { /* unsetenv from <stdlib.h>*/
                if (unsetenv(cmd->env.name) == -1) {
                    perror("Delete environment variable");
                    return 0;
                }
            }

            break;
        case JOB:
		//dunno ... we don't do that
		/*switch (cmd->job.kind)
			{
			case INFO:
				showJobs();
				break;

			case BG:
				sendBack( cmd->job.id ); SIGCONT
				break;http://en.wikipedia.org/wiki/SIGCONT

			case FG:
				sendForeground( cmd->job.id ); SIGCONT
				break;
			}*/
            break;
        case PROG:
            execute_prog(cmd);
            break;
        case PIPE:
            execute_pipe(cmd);
            break;
        default:
            fprintf(stderr, "Parse Error");
            break;
    }

    return 0;
}


void setup_environment_vars(void)
{
    struct passwd *pwentry;

    /* man getpwuid(3): If one wants to check errno after the call, it should
     * be  set  to  zero  before  the call.
     */
    int errno = 0;
    if (!(pwentry = getpwuid(getuid()))) {
        perror("reading password database records failed");
        exit(EXIT_FAILURE);
    }
		/*  The getpwuid() function returns a pointer to a structure containing the
     *  broken-out fields of the record in the password database  that  matches
     *  the user ID uid.
		 */

    if (setenv("HOME", pwentry->pw_dir, 1) == -1) {
        perror("Set HOME failed");
        exit(EXIT_FAILURE);
    }

    /* must not free pwentry */
}

// don't warn about unused parameters argc, argv
#pragma GCC diagnostic ignored "-Wunused-parameter"
int main(int argc, char** argv) {

    /* ignore SIGINT */
    setup_signal_handler(SIGINT, SIG_IGN);
    setup_remaining_signal_handlers();

    setup_environment_vars();

    int finished = 0;

    char path[310] = "";


    while (!finished) {

	getcwd( path, 300 ); //copies path name of current directory
	strcat( path, " $ " ); //string concatanation


        /* read a line of input */
        char *line = readline(path);

        if (!line || !strcmp("", line)) {
            // line could be NULL, if the user hit Ctrl-D.
            continue;
        }

        add_history(line);

        /* parse line */
        cmds* cmdlist = parser_parse(line);

        if (parser_status != PARSER_OK) { /* parsing line failed */
            printf("ERROR: %s @ %d:%d\n", parser_message, error_line, error_column);
            continue;
        }

        /* execute commands */
        cmds *c = cmdlist;

        while (c) {
            finished = do_command(c);
            if (finished)
                break;
            c = c->next;
        }

        /* free pointers */
        parser_free(cmdlist);
        free(line);
        line = NULL;

    }

    return EXIT_SUCCESS;
}
