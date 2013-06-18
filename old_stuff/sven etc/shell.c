/**********************************************************
***********************************************************
***														***
*** Project 2 - Praktikum Betriebssysteme				***
*** 													***
*** Group Members: 	Robert Engelke						***
***                	Stephan Rudolph						***
***                	John Trimpop						***
***					Sven Berger							***
***														***
*** Task:		Commandshell for inter-process			***
***				communication in UNIX/Linux				***
***														***
***********************************************************
**********************************************************/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h> 

#include <unistd.h>
#include "parser.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <malloc.h>   /* dynamic memory management                       */
#include <setjmp.h>   /* longjumps to simplify error handling            */
#include <stdbool.h>  /* constants                                       */
#include <string.h>   /* string manipulations                            */
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

#define MAX_LENGTH (1024)

// true if a keyboardhit is executed
int kbhit(void)
{
    struct termios oldt, newt;
    int ch;
    int oldf;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);

    if(ch != EOF)
    {
      ungetc(ch, stdin);
      return 1;
    }

    return 0;
}

main(void){

	//getting system username
	char* getUser = getenv("USER");
	if(getUser == NULL) return EXIT_FAILURE;
	
	//change to default directory
	char pathName[] = "/home/";
	char upathName[MAX_LENGTH];
	strcpy(upathName, ("%s", getUser));
	strcat(pathName, upathName);
	chdir(pathName);
	char newPath[MAX_LENGTH]; //for later use

	//input line
	char line[MAX_LENGTH];

   	cmds* cmd = NULL;
	
	int i = 0;
	int maxhistoryEntries = 100;
	char history[maxhistoryEntries][200];
	int historyEntries = 0;
	int historyEntry = 0;

top: //returning from invalid command

	printf("SuperMegaShell : %s > ", pathName);

	int key;
	key = 1;
	int usehistory = 0;
	int gethistory = 0;
	char ersteszeichen;
	while(key)
	{
    	if(kbhit())
		{
    		ersteszeichen = getchar();
            if(ersteszeichen == 27) 
			{
				char zeichen = getchar();
				if(zeichen == 91) 
				{
					zeichen = getchar();
					// code for arrow UP Do history changing
					if(zeichen == 65)
					{
						if(!usehistory)
						{
							usehistory = 1;
							printf("\nSuperMegaShell : %s > %s", pathName, history[gethistory]);
						}
						else if(gethistory<(historyEntries-1))
						{
							gethistory++;
							printf("\nSuperMegaShell : %s > %s", pathName, history[gethistory]);
						}
					}
					// code for arrow DOWN do change to input mode
					if(zeichen == 66)
					{
						if(0<gethistory)
						{
							gethistory--;
							printf("\nSuperMegaShell : %s > %s", pathName, history[gethistory]);
						}
						else if(usehistory)
						{
							usehistory = 0;
							printf("\nSuperMegaShell : %s > ", pathName);
						}
					}
				}
			}
            else 
			{
				key = 0;
            }
        }
	}

	if(usehistory)
	{
		strcpy(line, history[gethistory]);
	}
	else
	{
		printf("%c", ersteszeichen);
		gets(line);

		char helpline[MAX_LENGTH];
		sprintf(helpline, "%c", ersteszeichen);
		strcat(helpline, line);
		strcpy(line, helpline);
	}

	i = historyEntries;
	for(; 0<i; i--)
		strcpy(history[i], history[i-1]);

	strcpy(history[0], line);
	if(historyEntries<(maxhistoryEntries-1)) historyEntries++;

	/*
	printf("\nhistory: %i einträge\n", historyEntries);
	i = 0;
	for(; i<historyEntries; i++)
		if(0<strlen(history[i])) printf("%s\n", history[i]);
	*/

	cmd = parser_parse(line);

	//start mega while
	while (cmd != NULL){
	
		if(cmd->kind == EXIT){
			printf("\nClosing SuperMegaShell...\n");
			return 0;
		}	
		else if(cmd->kind == ENV){
			if(cmd->env.value != NULL){
				setenv(cmd->env.name,cmd->env.value,1);
			}
			else unsetenv(cmd->env.name);
		}
		else if(cmd->kind == CD){
			if(chdir(cmd->cd.path)==0){
				getcwd(newPath, MAX_LENGTH);
				strcpy(pathName, newPath);
				goto top;
			}
			else{
				printf("\nNo valid directory.\n");
				//to avoid buggy prompt
				getcwd(newPath, MAX_LENGTH);
				strcpy(pathName, newPath);
				goto top;
			}			
		}
		else if(cmd->kind == PROG){
			if(cmd->prog.output){
			//CMD > file
			execl(pathName, line, (char *) 0);
			}
			else if(cmd->prog.input){
			//CMD < file
			execl(pathName, line, (char *) 0);
			}
			else if(cmd->prog.background){
			// vi&
			}
			else break;
			/*
			{
				char *command;
				int i ;
				strcpy(command,cmd->prog.argv[0]);
				// add all parameters
				for (i = 1 ; i < cmd->prog.argc; i++){
					strcat(command, " ");
					strcat(command, cmd->prog.argv[i]);
					}
				char *pathn;
				int find = 1;
				strcpy(pathn, cmd->prog.argv[0]);
				if(fork() > 0){
					execl(pathn, command , (char *) 0);
					perror(pathName);
					exit(1);
					}
				}
			}	
			*/
		}
		else if(cmd->kind == PIPE){
   			int i, j, k;
   			j = k = 0;
   			char commands[100][MAX_LENGTH];
   			for(i = 0; i < MAX_LENGTH; i++){
    			if(line[i] == '|'){
     				j++;
     				k = 0;
    			}
				else if(i>0 && line[i] == ' '){ 
     				if(line[i-1] == '|'){
      					continue;
     				}
    			}
				else{
     				commands[j][k] = line[i];
     				k++;
    			}
   			}
   			pipeCommands(commands, pathName, j);
  		}
		
		else if(cmd->kind == JOB){
		}		
			
		else break;					
	}	
	
	printf("\nPlease enter a valid command.\n");
	
	//clear memory	
	parser_free(cmd);

	goto top;
	return 0;
}



/*
	This function is used to pipe commands. Input contains
	of the commands in working sequence, the path and the 
	number of Commands to be processed.
*/
pipeCommands(char *cmds[], char *path, int numberOfCommands){
	// iterator throug commands
	int i;
	// filedescriptors used for accessing the files
	int fd[2];
	int ffd[2];
	pid_t child;
	pid_t fchild;
	if(pipe(ffd)){
		perror("pipe");
		return;
	}
	if(fchild = fork() > 0){
		/* First child */
		close(ffd[0]);
		dup2(ffd[1], 1);
		close(ffd[1]);
		execl(path, cmds[0], (char *) 0);
		perror(path);
		exit(1);
	}
	else{
		for(i = 1; i < numberOfCommands-1; i++){
			if(pipe(fd)){
				perror("pipe");
				return;
			}
			if(child = fork() > 0){
				close(ffd[1]);
				dup2(ffd[0], 0);
				close(ffd[0]);
				close(fd[0]);
				dup2(fd[1], 1);
				close(fd[1]);
				execl(path, cmds[i], (char *) 0);
				perror(path);
				exit(i+1);
			}
			ffd[0] = fd[0];
			ffd[1] = fd[1];
		}
		i++;
		if(i >= numberOfCommands){
			i = numberOfCommands - 1;
		}
		if(child = fork() > 0){
			close(ffd[1]);
			dup2(ffd[0], 0);
			close(ffd[0]);
			execl(path, cmds[i], (char *) 0);
			perror(path);
			exit(i+1);
		}
	}
	return;
}			
		



