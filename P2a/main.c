#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include "types.h"
#include "methods.h"

int main(int argc, char* argv[])
{
	int	stdin_num_bytes;
	int	oRedirect;
	int	aRedirect;
	int pip;
	int file;
	
	// Assume 1024 char max +1 for NULL (from spec)
	char*	cmd = (char*) malloc(1025 * sizeof(char));
	size_t	cmd_size = sizeof(cmd); // 1025 bytes (1/char)
	// Max possible words under assumption #4
	char* args[512]; // Wasteful with memory atm *shrug*
	
	while(true) // Run forever (until ctrl+c, of course)
	{
		// Prompt
		printf("mysh> ");
		
		// Wait for user input, returns # chars read (-1 on error)
		stdin_num_bytes = getline(&cmd, &cmd_size, stdin);
	
		// Check getline return value 
		if(stdin_num_bytes == -1)
		{
			printf("Error!\n");		
		}
		else // Parse words out of cmd
		{
			bool lastWord = false;
			int curChar = 0;
			int index = 0;			
			
			while(!lastWord)
			{
			/*
				if(index == 0)
					command = ParseWord(&cmd, &curChar, stdin_num_bytes, &lastWord);
				else
					args[index-1] = ParseWord(&cmd, &curChar, stdin_num_bytes, &lastWord);				
					*/
					
				args[index] = ParseWord(&cmd, &curChar, stdin_num_bytes, &lastWord);				
				
				index++;
			}
			
			// Check for special features
			oRedirect = CheckOverwriteRedirect(args);
			aRedirect = CheckAppendRedirect(args);
			pip = CheckPipe(args);
		}
		
		if(strncmp(args[0], "exit", sizeof("exit")) == 0) 
		{
			exit(0); // Bail out		
		}
		else if(strncmp(args[0], "cd", sizeof("cd")) == 0)
		{
			int errChk = chdir(args[1]);
			if(errChk == -1)
				fprintf(stderr, "Error!\n");
		}
		else if(strncmp(args[0], "pwd", sizeof("pwd")) == 0)
		{
			char* cwd = getcwd(NULL, 0);
			printf("%s\n", cwd);
		}			
		else // execvp command
		{
			int pid = fork();
			mode_t mode = S_IRUSR | S_IWUSR | S_IWGRP | S_IROTH;
			// Handle special feature filestreams
			if(oRedirect != -1)
			{
				file = open(args[oRedirect+1], O_CREAT | O_RDWR | O_TRUNC, mode);
				args[oRedirect] = NULL; // Terminate the 
				assert(file != -1);
				// PICK UP HERE!!
				//dup2 copies fd fine but output is not printed to file the second time this is called. Weird.
				int chk = dup2( STDOUT_FILENO, file);
				assert(chk != -1);
				
			    //what about redirects with no spaces around >

				perror(NULL);
				
			}
			//handle append redirect
			if(aRedirect != -1)
			{
				file = open(args[aRedirect+1], O_CREAT | O_RDWR | O_APPEND, mode);
				args[aRedirect] = NULL; 
				assert(file != -1);
				
				int chk = dup2(file, STDOUT_FILENO);
				assert(chk != -1);			    
				
				perror(NULL);
				
			}
			//handle pipes 
			//modify to handle TWO pipes
			if(pip != -1)
			{				
				
				//create a new fd to store output from first command
				//file = open(args[pip+1], O_CREAT | O_RDWR, mode);
				//int feed[2] = {file, STDOUT_FILENO};
				//store stdout to fd file
				//int chk = pipe(feed);
				//assert(chk != -1);

				args[pip] = NULL; 
				//assert(file != -1);
				
			}

			// If fork returns 0, it's the child, -1 if error,
			// else returns child's PID if it's the parent
			if (pid == -1)
				perror("Error forking.\n");    
			else if (pid != 0)
				wait(NULL);		
			else // It's the child process
			{
				if(oRedirect != -1)
					setvbuf(stdout, NULL, _IONBF, 0);
				//if piping, fork again to run first process 
				//this should output to file since we piped output to file
				if(pip != -1){
										
					int pid1 = fork();
					int feed[2] = {STDIN_FILENO, STDOUT_FILENO};
					int chk2 = pipe(feed);
					assert(chk2 != -1);

					if(pid1 == -1)
						perror("Forking error 2.\n ");
					//parent if pid2 != 0
					else if(pid1 != 0)
						wait(NULL);
					else{

					int chk1 = execvp(args[0], args);
					assert(chk1 != -1);

					//replace args with second command
					args[0] = args[pip+1];
					int chk = write(STDIN_FILENO, args[1], strlen(args[1]));
					assert(chk != -1);

					}
					
				}

				//run process if no pipe, run second process if piping
				execvp(args[0], args);
				
				printf("execvp failed!\n");
				exit(0);
			}		
		}
		
	}	
	
	exit(0);
}


int CheckOverwriteRedirect(char** args)
{
	int i = 0;
	int sym = -1;
	
	while(args[i] != NULL)
	{
		if(strncmp(args[i], ">", sizeof(">")) == 0)
		{
			sym = i;
			break;		
		}
		
		++i;
	}
	
	return sym;
}

int CheckAppendRedirect(char** args)
{
	int i = 0;
	int sym = -1;
	
	while(args[i] != NULL)
	{
		if(strncmp(args[i], ">>", sizeof(">>")) == 0)
		{
			sym = i;
			break;		
		}
		
		++i;
	}
	
	return sym;
}

int CheckPipe(char** args)
{
	int i = 0;
	int sym = -1;
	
	while(args[i] != NULL)
	{
		if(strncmp(args[i], "|", sizeof("|")) == 0)
		{
			sym = i;
			break;		
		}
		
		++i;
	}
	
	return sym;
}

char* ParseWord(char** cmd, int* pos, int cmd_size, bool* last)
{
	char* word = (char*) malloc(sizeof(*cmd));
	char* temp = word;
	int i = 0;
	
	// Tack characters on until we hit whitespace (or the end)
	while(*pos < cmd_size)
	{
		char curChar = *(*cmd + *pos);
		
		if(!isspace(curChar))
		{
			*temp = curChar; // Track
			
			temp++; // Advance
			++(*pos);
		}
		else
		{
			// Advance pos to start of the next word
			while(isspace(*(&curChar + i)))
			{
				++i;
				++(*pos);
			}
			
			
			*temp = 0; // Null terminate this word
			return word;
		}
	}
	
	if(*pos >= cmd_size) 
	{
		*last = true;
		return NULL;	
	}
	
	return word;
}
