//COP4610
//Project 1 Starter Code
//example code for initial parsing

//*** if any problems are found with this code,
//*** please report them to the TA


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <fcntl.h>

/////////////////////

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
typedef struct
{
	char** tokens;
	int numTokens;
} instruction;

typedef struct{
	int queue;
	int pid;
	char** cmd;
} process;

//parsing helper functions
void addToken(instruction* instr_ptr, char* tok);
void printTokens(instruction* instr_ptr);
void clearInstruction(instruction* instr_ptr);
void addNull(instruction* instr_ptr);

//environmental variables
char* expandEnvVar(char* token);

//file path resolution
char* convertFilePath(char* relPath);
int isAbsPath(char* filePath);
int isDirectory(const char*);

//command path resolution
char* expandCommand(char* command);

//process execution
void execution(char**);
process exe_bg(char**);

// I/O resolution
void IO(char** x, int y);

// Pipe
void piping( char** p, int q);

int main() {
	char* token = NULL;
	char* temp = NULL;

	instruction instr;
	instr.tokens = NULL;
	instr.numTokens = 0;

    int executeControl=0;
	int insCount = 0;

	while (1) {

		char* envUser = expandEnvVar("$USER");
		char* envMachine = expandEnvVar("$MACHINE");
		char* envPwd = expandEnvVar("$PWD");
		printf("%s@%s:%s>", envUser, envMachine, envPwd);
		
		// loop reads character sequences separated by whitespace
		do {
			//increment instruction count
			insCount++;
			//scans for next token and allocates token var to size of scanned token
			scanf("%ms", &token);
			temp = (char*)malloc((strlen(token) + 1) * sizeof(char));

			int i;
			int start = 0;
			for (i = 0; i < strlen(token); i++) {
				//pull out special characters and make them into a separate token in the instruction
				if (token[i] == '|' || token[i] == '>' || token[i] == '<' || token[i] == '&') {
					if (i-start > 0) {
						memcpy(temp, token + start, i - start);
						temp[i-start] = '\0';
						addToken(&instr, temp);
					}

					char specialChar[2];
					specialChar[0] = token[i];
					specialChar[1] = '\0';

					addToken(&instr,specialChar);

					start = i + 1;
				}
			}

			if (start < strlen(token)) {
				memcpy(temp, token + start, strlen(token) - start);
				temp[i-start] = '\0';
				addToken(&instr, temp);
			}

			//free and reset variables
			free(token);
			free(temp);

			token = NULL;
			temp = NULL;
		} while ('\n' != getchar());    //until end of line is reached

		addNull(&instr);
 		/* ------------------------------------------ END OF SAMPLE CODE ----------------------------------------- */

		int i;

		process* processes = (process*)malloc(100 * sizeof(process));
		int numProc = 0;

		// ENVIRONMENTAL VARIABLES
		for (i=0; i<instr.numTokens-1; i++){
			if (strlen(instr.tokens[i])){
				if (instr.tokens[i][0] == '$'){
					char* envVar = expandEnvVar(instr.tokens[i]);
					
					// variable is found, replace with its value
					if (envVar != NULL){
						char* temp = malloc(strlen(envVar)+1);
						strcpy(temp, envVar);
						instr.tokens[i] = realloc(temp, strlen(temp)+1);
						free(envVar);

					// variable not found, replace with nothing
					} else {
						char* temp = "\0";
						strcpy(instr.tokens[i], temp);
					}
				}
            }
		}

		// FILE PATH RESOLUTION
		if (!isAbsPath(instr.tokens[1])){
			char* absPath = convertFilePath(instr.tokens[1]);
			instr.tokens[1] = realloc(absPath, strlen(absPath)+1);
		}

		// BUILT INS
		// exit
		if (!strcmp(instr.tokens[0], "exit")){
			////////////////////////////////////////
			//WAIT FOR BACKGROUND COMMANDS TO FINISH
			////////////////////////////////////////
			printf("Exiting now!\n");
			printf("\tCommands executed: %d\n", insCount);
			return 0;
		}
		// change directory
		else if (!strcmp(instr.tokens[0], "cd")){
			char* path = (char*)malloc(100 * sizeof(char));
			if (instr.numTokens <= 2){
				strcpy(path, expandEnvVar("$HOME"));
			}
			else{
				char* isPath = strstr(instr.tokens[1], "/");
				if (isPath == NULL){
					char* commPath = malloc(strlen(envPwd) + strlen(instr.tokens[1]) + 1);
					strcpy(commPath, envPwd);
					strcat(commPath, "/");
					strcat(commPath, instr.tokens[1]);
					instr.tokens[1] = realloc(commPath, strlen(commPath) + 1);
				}
				strcpy(path, instr.tokens[1]);
			}
			if (instr.numTokens >= 4){
				printf("Error: Too many arguments\n");
			}
			else{
				
				if (isDirectory(path)){
					if (!chdir(path)){
						setenv("PWD", path, 1);
					}
				} else {
					printf("Invalid Directory: %s\n", instr.tokens[1]);
				}
			}
		}
		// echo
		else if (!strcmp(instr.tokens[0], "echo")){
			int i = 1;
			for( ; i < instr.numTokens-1; i++){
				if (instr.tokens[i] != NULL){
					printf("%s ", instr.tokens[i]);
				}
			}
			printf("\n");
		}
	
		// EXTERNAL/INTERNAL COMMANDS
		else {
			// path resolution
			char* absPath = expandCommand(instr.tokens[0]);
			bool isBg = false;
			if (absPath != NULL){
				instr.tokens[0] = realloc(absPath, strlen(absPath)+1);
				char** newTokens = malloc(instr.numTokens * sizeof(char*));	//stores command without &
				int newCount = 0;
				//handling for background processes
            	for (i=0; i < instr.numTokens-1; i++){
					if (strcmp(instr.tokens[i], "&")){	//finds only tokens that aren't & and adds them to newTokens
						newTokens[i] = strdup(instr.tokens[i]);
						newCount++;
					}
					else{
						isBg = true;	//found an &, meaning the command is to be run in bg
					}
        		}
				if (isBg){
            		printf("BG EXE\n");
					printf("NewCount: %d\n", newCount);
					for (i=0; i < newCount-1; i++){
						printf("%s ", newTokens[i]);
						printf("\n");
					}
            		processes[numProc] = exe_bg(newTokens); //executes command in the background
					numProc++;
        		}

        /// these added lines of codes take care ot the control flow execution
                for (i=0; i<instr.numTokens-1; i++){
			        if (strlen(instr.tokens[i])){
				         //////// check for the output redirection
                         if(instr.tokens[i][0] == '>'|| instr.tokens[i][0]=='<')
                        {
                            IO(instr.tokens, instr.numTokens); //pass the character array to the IO function
                             executeControl++;
                                break; // leave because we won't do the other instructions down there
                        }

                         //////// check for pipe processe
                        if(instr.tokens[i][0] == '|')
                        {
                          piping(instr.tokens, instr.numTokens); //pass the character array to the IO function
                          executeControl++;
                             break; // leave because we won't do the other instructions down there
                        }
                
                    }
                }
                // if there was any execution before, regular execution won't happen
                 if(executeControl==0)
            	        execution(instr.tokens); //executes command normally

				isBg = false;
			}
            
            
			else
				printf("Unknown command: %s\n", instr.tokens[0]);
		}


		while (!waitpid(-1, NULL, WNOHANG)){

		}
		
		for (i = 0; i <= numProc-1; i++){
			printf("[%d]+\t[%s]\n", processes[i].queue, processes[i].cmd[0]);
		}
		
		free(envUser);
		free(envMachine);
		free(envPwd);
        executeControl=0; // need to reset this value for the future commands 
		clearInstruction(&instr);
	}

	return 0;
}

// takes $ENVVAR and returns its representation
char* expandEnvVar(char* token){
	char* fullToken = NULL;
	char* expandedVar = NULL;
	if (strlen(token)>0){
		if (token[0]=='$'){
			fullToken = malloc(strlen(token));
			strcpy(fullToken, token+1);
			char* temp = getenv(fullToken);
			if (temp != NULL){
				expandedVar = malloc(strlen(temp)+1);
				strcpy(expandedVar, temp);
				free(fullToken);
				return expandedVar;
			}
			else{
				printf("Unrecognized environmental variable: %s", fullToken);
				free(fullToken);
				return NULL;
			}
		}
		else{
			return NULL;
		}		
	} else {
		printf("Unrecognized environmental variable: %s", token);
		return NULL;
	}
}

// converts relative file path to absolute file path
char* convertFilePath(char* relPath){
	
	int size = strlen(relPath) * 30;
	char* homePath = expandEnvVar("$HOME");
	char* pwdPath = expandEnvVar("$PWD");
	char* absPath = malloc(size);
	char* tempPath = malloc(size);
	char** subPaths = malloc(size * sizeof(char*));
	strcpy(tempPath, relPath);
	
	// holds pointer to last directory in $PWD
	char* trailingDir = strrchr(pwdPath, '/');
	
	// Same as $PWD, minus the last directory
	char* parentPath = malloc(strlen(pwdPath)+2);
	
	// If only one directory in PWD
	if (&pwdPath[0] == trailingDir){
		strcpy(parentPath, "/");	
	} else {
		strncpy(parentPath, pwdPath, trailingDir-pwdPath);
		char* nullChr = "\0";
		strcat(parentPath, nullChr);
	}
	
	// Splits file path into directories/files, adds them to subPaths	
	int i = 0;
	char* temp = strtok(tempPath, "/");
	while (temp != NULL){	
		subPaths[i] = malloc(strlen(temp)+1);
		strcpy(subPaths[i], temp);
		temp = strtok(NULL, "/");
		i++;
	}
	
	// Reloops subPaths to eliminate relative symbols
	int j = 0;
	for (j=0; j < i; j++){
		if (j == 0){
			if (!strcmp(subPaths[j], "~")){
				strcpy(absPath, homePath);
			} else if (!strcmp(subPaths[j], ".")){
				strcpy(absPath, pwdPath);
			} else if (!strcmp(subPaths[j], "..")){
				strcpy(absPath, parentPath);
			}
		} else {
			if (!strcmp(subPaths[j], ".")){
				continue;
			} else {
				strcat(absPath, "/");
				strcat(absPath, subPaths[j]);
			}
		}
		free(subPaths[j]);
	}
	free(homePath);
	free(pwdPath);
	free(parentPath);
	free(subPaths);
	free(tempPath);
	return absPath;
}	

// returns 1 if the file path cannot be expanded further
int isAbsPath(char* filePath){
	if (filePath == NULL)
		return 1;
	if(!strcmp(filePath, "..")){
		return 0;
	} else if(!strcmp(filePath, ".")){
		return 0;
	} else if(!strcmp(filePath, "~")){
		return 0;
	}
	char* temp = malloc(strlen(filePath)+1);
	strcpy(temp, filePath);
	char* parentCheck = strstr(temp, "../");
	char* currentCheck = strstr(temp, "./");
	char* homeCheck = strstr(temp, "~/");
	free(temp);
	if (parentCheck != NULL || currentCheck != NULL || homeCheck != NULL){
		return 0;
	} else {
		return 1;
	}
}
// Returns first absolute path to command if it exists, NULL otherwise
char* expandCommand(char* command){
	
	char* commandPath = NULL;
	char* envVar = expandEnvVar("$PATH");
	char* rawPath = malloc(strlen(envVar)+1);
	strcpy(rawPath, envVar);
	
	// Splits $PATH into subpaths, expands if necessary, stores each subpath in pathArr
	char** pathArr = calloc(strlen(rawPath)+1, sizeof(char*));
	char* subPath = strtok(rawPath, ":");
	int i=0;
	while (subPath != NULL){
		pathArr[i] = malloc(strlen(subPath)+1);
		strcpy(pathArr[i], subPath);
		int abs = isAbsPath(pathArr[i]);
		if (abs == 0){
			char* temp = convertFilePath(pathArr[i]);
			pathArr[i] = realloc(temp, strlen(temp)+1);
		}
		subPath = strtok(NULL, ":");
		i++;
	}
	// Concats the command onto each path and tests for access
	// breaks the loop and returns the absolute path if found, NULL otherwise
	i--;
	for (; i>=0; i--){
		char* temp = malloc(strlen(pathArr[i]) + strlen(command) + 3);
		strcpy(temp, pathArr[i]);
		strcat(temp, "/");
		strcat(temp, command);
		if(access(temp, F_OK) != -1) {
    			commandPath = malloc(strlen(temp)+1);
			memcpy(commandPath, temp, strlen(temp)+1); 
			free(temp);
			free(pathArr[i]);
   			break;
		}
		free(temp);
		free(pathArr[i]);
	}
	free(envVar);
	free(pathArr);
	free(rawPath);
	return commandPath;	
}

// this function gets the command and executes it
void execution (char** command)
{
    int status; 
    __pid_t child = fork();
    
    // Error
    if (child == -1){
        printf("Error in forking\n");
	exit(1);
    }
    // Child
    else if(child == 0) {
	 execv(command[0], command);
         printf("Problem executing: %s\n", command[0]);
         exit(1);
    }
    // Parent
    else{
        printf("%s\n",command[0]);
           //control back to the shell program
           waitpid(child, &status, 0);     
    }
}

//executes background processes
process exe_bg (char** command)
{
    int status; 
    __pid_t child = fork();
	process newProcess;
	newProcess.queue = 0;
	newProcess.pid = (int)child;
	newProcess.cmd = command;
	
    // Error
    if (child == -1){
        printf("Error in forking\n");
	exit(1);
    }
    // Child
    else if(child == 0) {
		setpgid(0, 0);
	 execv(command[0], command);
         printf("Problem executing: %s\n", command[0]);
         exit(1);
    }
    // Parent
    else{
		printf("[0]\t[%d]\n", (int)child);
        //control back to the shell program
        waitpid(child, &status, WNOHANG);   
    }
	return newProcess;
}

//Checks whether a path is a valid directory, credit to:
//https://codeforwin.org/2018/03/c-program-check-file-or-directory-exists-not.html
int isDirectory(const char* path){	
	struct stat stats;
	stat(path, &stats);

    // Check for file existence
    if (S_ISDIR(stats.st_mode))
        return 1;

    return 0;
}

//reallocates instruction array to hold another token
//allocates for new token within instruction array
void addToken(instruction* instr_ptr, char* tok)
{
	//extend token array to accomodate an additional token
	if (instr_ptr->numTokens == 0)
		instr_ptr->tokens = (char**) malloc(sizeof(char*));
	else
		instr_ptr->tokens = (char**) realloc(instr_ptr->tokens, (instr_ptr->numTokens+1) * sizeof(char*));

	//allocate char array for new token in new slot
	instr_ptr->tokens[instr_ptr->numTokens] = (char *)malloc((strlen(tok)+1) * sizeof(char));
	strcpy(instr_ptr->tokens[instr_ptr->numTokens], tok);

	instr_ptr->numTokens++;
}

void addNull(instruction* instr_ptr)
{
	//extend token array to accomodate an additional token
	if (instr_ptr->numTokens == 0)
		instr_ptr->tokens = (char**)malloc(sizeof(char*));
	else
		instr_ptr->tokens = (char**)realloc(instr_ptr->tokens, (instr_ptr->numTokens+1) * sizeof(char*));

	instr_ptr->tokens[instr_ptr->numTokens] = (char*) NULL;
	instr_ptr->numTokens++;
}

void printTokens(instruction* instr_ptr)
{
	int i;
	printf("Tokens:\n");
	for (i = 0; i < instr_ptr->numTokens; i++) {
		if ((instr_ptr->tokens)[i] != NULL)
			printf("%s\n", (instr_ptr->tokens)[i]);
	}
}

void clearInstruction(instruction* instr_ptr)
{
	int i;
	for (i = 0; i < instr_ptr->numTokens; i++){
		free(instr_ptr->tokens[i]);
	}

	free(instr_ptr->tokens);

	instr_ptr->tokens = NULL;
	instr_ptr->numTokens = 0;
}

///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
void IO(char* argv[], int length)
{
 //////////This bloc cpies the relevant information from the command for execution
 int tSize=length-1; // one is for the null
   char* cmd[tSize-1];
     int i=0, j=0;
     int IO;
    char * out= ">";
    char* in="<";

      for(i; i<tSize; i++)
        {
            if(strcmp(argv[i],in)==0) 
            {
                IO=0; // that means Input redirection
            }

            else if(strcmp(argv[i], out)==0)
            {
                IO=1; // this means Output redirection
            } 

             else
                {
                    cmd[j]=argv[i];
                    j++;
                }
        }
        i=0;
        
  
  if(IO==1) // output redirection
  {  
      int status;
    int fd=open(cmd[tSize-2], O_CREAT | O_APPEND | O_WRONLY);
    pid_t child=fork();
     if(child==0)
     {
         //Child
         close(STDIN_FILENO);
         dup2(fd,1); // only dup2 would work
         close(fd);
         //Executeprocess
         execv(cmd[0], cmd);
         //printf("Command not found \" %s \" \n", what[0]);
         

         }
         else
         {
             //Parent
            close(fd);
            waitpid(child, &status, 0); 
             //exit(1);
         }
         printf("%s\n",cmd[tSize-2]);
  }


  else // input redirection
  {
      int fd= open(cmd[tSize-2],O_WRONLY);
       pid_t child=fork();
       int status;
     if(child==0)
     {
         //Child
         close(STDIN_FILENO);
          dup2(fd,0); // only dup2 would work
         close(fd);
         
         //Executeprocess
         execv(cmd[0], cmd);
         printf("Command not found \n");
         

         }
         else
         {
             //Parent
             close(fd);
              waitpid(child, &status, 0);
             //exit(1);
         }
        
        
  }
  
}


/////////////////////////////////////
/////////////////////////////////////
 void piping( char* argv[], int length)
{

    
   //Pipe stuff
    int toPipe[2];
    pipe(toPipe);

 int j=0; 
 int k=0;
 char* p="|";
 int status;
 int secondPart=0;
  int tSize=length-1;
 char* first[tSize/2]; // first part of the pipe 
 char * rest[tSize/2]; // remaining of the pipe

 int i; // for the for loop
    
    for(i=0; i<tSize; i++)
        {
            if(strcmp(argv[i],p)==0) 
            {
                    secondPart=1;
            }
            else
            {
                if(secondPart==1)
                {
                    rest[k]=argv[i];
                    k++;
                    continue;
                }
                first[j]=argv[i];
                    j++;
            }
         }   



    char* pgm="/usr/bin/"; 
    size_t lng= strlen(pgm);
    char* cmd = malloc(lng+strlen(rest[0])+1); // allocating memory for the new  string 
    strcpy(cmd,pgm);
    int index;
    for(index=0; index<strlen(rest[0]); index++)
    {
        cmd[index+lng]=rest[0][index];
    }
    
    

    char *cmd1= first[0]; // first element in the array must not be a path for execv
  

  pid_t child= fork();
     if(child==0)     
     {
         
    
                 dup2(toPipe[1],1);
                 close(toPipe[0]);
                 execv(cmd1, first);

             }

pid_t Gchild= fork();
        if(Gchild==0)
            
            {   
                dup2(toPipe[0],0);
                close(toPipe[1]);
                execv(cmd, rest);
                        
            }
         
         else
         {   
                close(toPipe[0]);
                close(toPipe[1]);
              waitpid(child, &status,0);
              waitpid(Gchild, &status,0);
         }
        
        
        
        
}