
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include "parser.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <malloc.h>   // zum verwalten dynamischer speicher
#include <setjmp.h>
#include <stdbool.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

#define MAX_LENGTH (1024)

// tastatur check
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

	//benutzername des Computer auslesen
	char* Uname = getenv("USER");
	if(Uname == NULL) return EXIT_FAILURE;
	
	//zum "HOME" verzeichnis wechseln
	char Pname[] = "/home/";						//home pfad
	char UPname[MAX_LENGTH];						//user verzeichnis pfad
	strcpy(UPname, ("%s", Uname));
	strcat(Pname, UPname);
	chdir(Pname);
	char newPath[MAX_LENGTH];						//leerer pfad

	//eingabe zeile
	char line[MAX_LENGTH];

   	cmds* cmd = NULL;
	
	int i = 0;
	int maxhis = 100;								//maximale einträge in der history
	char history[maxhis][200];
	int historyEntries = 0;
	int historyEntry = 0;

top: //rückgabe bei falschem befehl

	printf("%s: %s > ",Uname, Pname);

	int key;
	key = 1;
	int usehistory = 0;
	int gethistory = 0;
	char FirstSymb;
	while(key)
	{
    	if(kbhit())
		{
    		FirstSymb = getchar();
            //wenn escape gedrückt wird
    		if(FirstSymb == 27)
			{
				char zeichen = getchar();
				//wenn "[" gedrückt wird
				if(zeichen == 91) 
				{
					zeichen = getchar();
					// Pfeiltaste nach oben wird gedrückt und di history wird einen eintrag nach hinten versetzt
					if(zeichen == 65)
					{
						if(!usehistory)
						{
							usehistory = 1;
							printf("\n&s : %s > %s",Uname, Pname, history[gethistory]);
						}
						else if(gethistory<(historyEntries-1))
						{
							gethistory++;
							printf("\n%s : %s > %s",Uname, Pname, history[gethistory]);
						}
					}
					// code for arrow DOWN do change to input mode
					if(zeichen == 66)
					{
						if(0<gethistory)
						{
							gethistory--;
							printf("\n%s : %s > %s",Uname, Pname, history[gethistory]);
						}
						else if(usehistory)
						{
							usehistory = 0;
							printf("\n%s : %s > ",Uname, Pname);
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
		//printf("%c", ersteszeichen);
		gets(line);

		char helpline[MAX_LENGTH];
		sprintf(helpline, "%c", FirstSymb);
		strcat(helpline, line);
		strcpy(line, helpline);
	}

	i = historyEntries;
	for(; 0<i; i--)
		strcpy(history[i], history[i-1]);

	strcpy(history[0], line);
	if(historyEntries<(maxhis-1)) historyEntries++;

	cmd = parser_parse(line);

	//start der while schleife
	while (cmd != NULL){
	
		if(cmd->kind == EXIT){
			printf("\nShell wird beendet...\n");
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
				strcpy(Pname, newPath);
				goto top;
			}
			else{
				printf("\nSystem kann den angegebenen Pfad nicht finden.\n");
				getcwd(newPath, MAX_LENGTH);
				strcpy(Pname, newPath);
				goto top;
			}			
		}
		else if(cmd->kind == PROG){
			if(cmd->prog.output){
			//CMD > file
			execl(Pname, line, (char *) 0);
			}
			else if(cmd->prog.input){
			//CMD < file
			execl(Pname, line, (char *) 0);
			}
			else if(cmd->prog.background){
			// vi&
			}
			else break;

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
   			pipeCommands(commands, Pname, j);
  		}
		
		else if(cmd->kind == JOB){
		}		
			
		else break;					
	}	
	
	printf("\nDer Befehl ist entweder falsch geschrieben oder konnte nicht gefunden werden.\n");
	
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
	// gibt befehle weiter
	int i;
	// dient zur zuordnungen von dateiendungen
	int fd[2];
	int ffd[2];
	pid_t child;
	pid_t fchild;
	if(pipe(ffd)){
		perror("pipe");
		return;
	}
	if(fchild = fork() > 0){
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
