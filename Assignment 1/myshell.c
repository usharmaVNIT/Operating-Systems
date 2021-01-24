/********************************************************************************************
This is a template for assignment on writing a custom Shell.

Students may change the return types and arguments of the functions given in this template,
but do not change the names of these functions.

Though use of any extra functions is not recommended, students may use new functions if they need to,
but that should not make code unnecessorily complex to read.

Students should keep names of declared variable (and any new functions) self explanatory,
and add proper comments for every logical step.

Students need to be careful while forking a new process (no unnecessory process creations)
or while inserting the signal handler code (which should be added at the correct places).

Finally, keep your filename as myshell.c, do not change this name (not even myshell.cpp,
as you dp not need to use any features for this assignment that are supported by C++ but not by C).
*********************************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>            // exit()
#include <unistd.h>            // fork(), getpid(), exec()
#include <sys/wait.h>        // wait()
#include <signal.h>            // signal()
#include <fcntl.h>            // close(), open()

#define ANSI_COLOR_GREEN "\033[01;32m"
#define ANSI_COLOR_BLUE "\033[01;34m"
#define ANSI_COLOR_RESET "\x1b[0m"



// The function findhash is used to find "##" in the input string
int findhash(char *str){
    while(*str!='\0'){
        if(*str == '#'){
            return 1;
        }
        str++;
    }
    return 0;
}
// The function findand is used to find "&&" in the input string
int findand(char *str){
    while(*str!='\0'){
        if(*str == '&'){
            return 1;
        }
        str++;
    }
    return 0;
}
// The function findredir is used to find ">" in the input string
int findredir(char *str){
    while(*str!='\0'){
        if(*str == '>'){
            return 1;
        }
        str++;
    }
    return 0;
}

int parseInput(char *input , char ***commands){
    int i=0;
    if(findhash(input)){//Hash found
        if(findand(input) || findredir(input)){//Should not contain and or redirect
            return 5;
        }
        while(1){
            char *line = strsep(&input,"##");//Splitting the input with delimiter = "##"
            if(line == NULL){//If no input is remaining then set the command to null and move forward
                commands[i] = NULL;
                break;
            }
            if(strcmp(line,"")==0){//If the line is itself empty then continue with the splitting meaning If the command is itself cannot be seperated by " " meaning the command is empty the simply continue as the next command might not be empty.
                continue;
            }
            // Now we have seperated the input and will contain 1 commands with its options like (ls -l)
            // Therefore we need to create token from the line of command
            int j=0;
            commands[i][j] = strsep(&line," ");
            while(commands[i][j]!=NULL){
                if(strcmp(commands[i][j]," ")==0 || strcmp(commands[i][j],"")==0){
                    j-=1;
                }
                j+=1;
                //creating tokens
                commands[i][j] = strsep(&line," ");
            }
            // If the command is itself cannot be seperated by " " meaning the command is empty the simply do i-- as the next command might not be empty and if not dine the this empty command will stop the execution further
            if(commands[i][0]==NULL){
                i-=1;
            }
            i++;
        }
        return 1;//If Hash is found
    }
    if(findand(input)){//And found
        if(findhash(input) || findredir(input)){ //Should not contain hash or redirect
            return 5;
        }
        while(1){
            char *line = strsep(&input,"&&"); //Splitting the input with delimiter = "&&"
            if(line==NULL){//If no input is remaining then set the command to null and move forward
                commands[i] = NULL;
                break;
            }
            if(strcmp(line,"")==0){//If the line is itself empty then continue with the splitting meaning If the command is itself cannot be seperated by " " meaning the command is empty the simply continue as the next command might not be empty.
                continue;
            }
            // Now we have seperated the input and will contain 1 commands with its options like (ls -l)
            // Therefore we need to create token from the line of command
            int j=0;
            commands[i][j] = strsep(&line," ");
            while(commands[i][j]!=NULL){
                if(strcmp(commands[i][j]," ")==0 || strcmp(commands[i][j],"")==0){
                    j-=1;
                }
                j+=1;
                //creating tokens
                commands[i][j] = strsep(&line," ");
                }
            i++;
        }
        return 2;//If and is found
    }
    if(strcmp(input,"exit")==0){//If the command is to exit the shell
        return 3; //For Exit;
    }
    if(findredir(input)){//redirection found
        if(findand(input) || findhash(input)){ //Should not contain hash or and
            return 5;
        }
                while(1){
                    char *line = strsep(&input,">");//Splitting the input with delimiter = ">"
                    if(line==NULL){//If no input is remaining then set the command to null and move forward
                        commands[i] = NULL;
                        break;
                    }
                    if(strcmp(line,"")==0){//If the line is itself empty then continue with the splitting meaning If the command is itself cannot be seperated by " " meaning the command is empty the simply continue as the next command might not be empty.
                        continue;
                    }
                    // Now we have seperated the input and will contain 1 commands with its options like (ls -l)
                    // Therefore we need to create token from the line of command
                    int j=0;
                    commands[i][j] = strsep(&line," ");
                    while(commands[i][j]!=NULL){
                        if(strcmp(commands[i][j]," ")==0 || strcmp(commands[i][j],"")==0){
                            j-=1;
                        }
                        j+=1;
                        //creating Tokens of command
                        commands[i][j] = strsep(&line," ");
                        }
                    i++;
                }
        return 4;
    }
// If single command is there
    //Againg we create tokens
    int j=0;
    commands[0][j]=strsep(&input," ");
    while(commands[0][j]!=NULL){
        j+=1;
        //creating Tokens of command
        commands[0][j]=strsep(&input," ");
    }
    return 0;//If Single Command
}

//This Function is to check and execute commands that have cd in them
// If found then execute and alert the calling function else simply alert that not found
int cdcommand(char **command){
    if (strcmp(command[0], "cd") == 0)
    {
        if (chdir(command[1]))
        {
            printf("Error:\n");
        }
        return 1;
    }
    return 0;
}


void executeCommand(char **commands)
{
    if(cdcommand(commands)==0){//Checking for cd command if not found then do the following
        int fk = fork();
        if(fk<0){
            printf("fork failed\n");
        }
        if(fk==0){
            // Make the behavior of interrups as default
            signal(SIGINT,SIG_DFL);
            signal(SIGTSTP,SIG_DFL);
            if(execvp(commands[0],commands)==-1){//If execvp fails to execute the commands then exit the child
              printf("Shell: Incorrect command\n");
                exit(0);
            }
        }
        int wt = wait(NULL);//Wait for the command to execute
    }
}

void executeParallelCommands(char ***multiplecommands)
{
    // This function will run multiple commands in parallel
    int i=0;
    int pids[100];
    int cnt = 0;
    while(multiplecommands[i]){//While there are commands remaining do the following
        if(cdcommand(multiplecommands[i])==0){//Checking for cd command if not found then do the following
                int j;
                j = pids[i] = fork();//Remember the process ids of child as the parent will have to wait for all of them in the end before returning to callie
                if(j<0){
                    printf("fork Failed\n");
                }
                if(j==0){
                    // Make the behavior of interrups as default
                    signal(SIGINT,SIG_DFL);
                    signal(SIGTSTP,SIG_DFL);
                    if(execvp(multiplecommands[i][0],multiplecommands[i])==-1){//If execvp fails to execute the commands then exit the child
                      printf("Shell: Incorrect command\n");
                        exit(0);
                    }
                }
            }
        i++;
        cnt++;
        }
    //Now wait for all the parallel processes to end before returning to callie
    for(i=0;i<cnt;i++){
        int status;
        int wt = waitpid(pids[i],&status,0);
    }
    
}

void executeSequentialCommands(char ***multiplecommands)
{
    // This function will run multiple commands in parallel
    int i=0;
    while(multiplecommands[i]){//While there are commands remaining do the following
        if(cdcommand(multiplecommands[i])==0){//Checking for cd command if not found then do the following
            int j = fork();
            if(j<0){
                printf("fork Failed\n");
            }
            if(j==0){
                // Make the behavior of interrups as default
                signal(SIGINT,SIG_DFL);
                signal(SIGTSTP,SIG_DFL);
                    if(execvp(multiplecommands[i][0],multiplecommands[i])==-1){//If execvp fails to execute the commands then exit the child
                        printf("Shell: Incorrect command\n");
                        exit(0);
                    }
            }
            else{
                // Now wait for the associated child process to finish
                int wt = wait(NULL);
            }
        }
        i++;
        // Final wait
        int wt = wait(NULL);
    }
}

void executeCommandRedirection(char ***commands)
{
    // This function will run a single command with output redirected to an output file specificed by user
    int rc = fork();
    
    if(rc < 0){            // fork failed; exit
        exit(0);
    }
    else if(rc == 0) {        // child (new) process

        // ------- Redirecting STDOUT --------
        // Make the behavior of interrups as default
        signal(SIGINT,SIG_DFL);
        signal(SIGTSTP,SIG_DFL);
        //Closing the standard output file via the macro STDOUT_FILENO
        close(STDOUT_FILENO);
        //Choosing the file provided by the user and opening it
        char *file = commands[1][0];
        open(file, O_CREAT | O_WRONLY | O_APPEND);

        // -----------------------------------
        //Executing the commands
        execvp(commands[0][0], commands[0]);

    }
    else {
        //Waiting for the child process
        int rc_wait = wait(NULL);
    }
}


void start_code()
{
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    printf("%s$",cwd);
}

int main()
{
    // Initial declarations
    //Disabling the signal interuupts
    signal(SIGINT,SIG_IGN);
    signal(SIGTSTP,SIG_IGN);
    char *input;
    size_t size = 500;
    input = (char*)malloc(size*sizeof(char));
    char ***commands;
    commands= (char***)malloc(10*sizeof(char**));
    for(int i=0;i<10;i++){
        commands[i] = (char**)malloc(10*sizeof(char*));
    }
    // Printing the prompt in format - currentWorkingDirectory$
    start_code();
    while(1)    // This loop will keep your shell running until user exits.
    {
        
        getline(&input,&size,stdin);// accepting input with 'getline()'
        int ctr = 0;
        //The last character is \n therefore removing it
        while(input[ctr]!='\n'){
            ctr++;
        }
        input[ctr] = '\0';
        
        //Parsing the input provided by the user
        int condition = parseInput(input,commands);
        
        if(condition == 3)    // When user gives exit command.
        {
            printf("Exiting shell...\n");
            exit(0);
            break;
        }
        
        if(condition == 2){
            executeParallelCommands(commands);
        }// This function is invoked when user wants to run multiple commands in parallel (commands separated by &&)
        else if(condition == 1){
            executeSequentialCommands(commands);
        }// This function is invoked when user wants to run multiple commands sequentially (commands separated by ##)
        else if(condition == 4){
            executeCommandRedirection(commands);
        }// This function is invoked when user wants redirect output of a single command to and output file specificed by user
        else if(condition==5){
            // if any 2 of "##" , "&&" , ">" are found
//            printf("Shell: Incorrect command\n");
        }
        else{
            executeCommand(commands[0]);
        }// This function is invoked when user wants to run a single commands
        
        // Printing the prompt in format - currentWorkingDirectory$
        start_code();
        //Flushing the standard input
        fflush(stdin);
    }
    
    return 0;
}

