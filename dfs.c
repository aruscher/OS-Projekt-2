#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

//GlobalVariables
FILE *history; //history file
int MAXINPUTL = 10000;

//Welcome the User
void helloUser(){
    printf("Welcome to DFS\n");
    printf("--------------\n");
}

//write to History
void writeHistory(char* input){ 
    history = fopen("history.txt","a+");
    if(history==NULL){
        perror("history.fopen");
    }
    printf("WRITE HISTORY: %s\n",input);
    fprintf(history,"%s\n",input);
    fclose(history);
}


//read the Input
void readInput(char* input){
    char *cwd;
    cwd = getcwd(0,0);
    printf("%s?> ",cwd);
    fgets(input,MAXINPUTL,stdin);
    input[strlen(input)-1]='\0';
}
//START EVAL FUNCTIONS
//here implement every function for evaluate






//END EVAL FUNCTION
//evaluate Input
void evalInput(char* input){
    printf("EVAL: %s\n",input);
}

//REPL -> READ PRINT EVALUATE LOOOOPP!
void startLoop(){
    char input[MAXINPUTL];
    while(1){
    readInput(input);
    writeHistory(input);
    printf("INPUT: %s\n",input);
    evalInput(input);
    }
}

int main(int argc, const char *argv[])
{
    helloUser();
    startLoop();
    return 0;
}
