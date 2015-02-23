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
	int	oRedirect;
	int	aRedirect;
	int pip;
	int file;
	char *pCmd[2];
	
	// Assume 1024 char max +1 for NULL (from spec)
	char*	cmd = (char*) malloc(1025 * sizeof(char));
	size_t	cmd_size = sizeof(cmd); // 1025 bytes (1/char)
	// Max possible words under assumption #4
	char* args[512];	
	//char** args = (char**) malloc(512*sizeof(char*)); // Wasteful with memory atm *shrug*
	
	while(true) // Run forever (until ctrl+c, of course)
	{
		// Prompt
		printf("mysh> ");
		
		ReadCommand(args, cmd, cmd_size); // into args
		oRedirect = CheckOverwriteRedirect(args);
		aRedirect = CheckAppendRedirect(args);
		pip = CheckPipe(args);
		
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
		else // fork and then execvp command
		{			
			mode_t mode = S_IRUSR | S_IWUSR | S_IWGRP | S_IROTH;
			// Handle special feature filestreams
			if(oRedirect != -1)
			{
				file = open(args[oRedirect+1], O_CREAT | O_RDWR | O_TRUNC, mode);
				//FILE* out = fopen(args[oRedirect+1], "w+");
				args[oRedirect] = NULL; // Terminate the arglist
				//perror(NULL);
				//assert(file != -1);
				//dup2 copies fd fine but output is not printed to file the second time this is called. Weird.
				int chk = dup2(file, STDOUT_FILENO);
				assert(chk != -1);

				perror(NULL);
				
			}
			//handle append redirect
			if(aRedirect != -1)
			{
				file = open(args[aRedirect+1], O_CREAT | O_RDWR | O_APPEND, mode);
				args[aRedirect] = NULL; 
				assert(file != -1);
				
				int chk = dup2(file, STDOUT_FILENO);
				perror(NULL);
				assert(chk != -1);			    
				
				
			}
			int feed[2];
			//if piping, fork to run first process 
			//this should output to file since we piped output to file
			if(pip != -1){ 
					
				pCmd[0] = args[pip+1];					
				args[pip]=NULL;	
									
				int chk2 = pipe(feed);	assert(chk2 != -1);
				int pid1 = fork();
				if(pid1 == -1)
					perror("Forking error 2.\n ");
				//parent if pid2 != 0
				else if(pid1 != 0)
				{
					wait(NULL);
					
					int i;
					dup2(feed[0], 0);
					close(feed[0]);
					getline(&cmd, &cmd_size, stdin); // stdin is the contents of the pipe!
					
					args[0] = pCmd[0]; // replace the old arguments for the second command
					args[1] = strtok(cmd, " \n");
					for(i = 2; i < 20; i++)
					{
						args[i] = strtok(NULL, " \n");						
					}
				}
				else{
					dup2(feed[1], 1);
					close(feed[1]);
					int chk1 = execvp(args[0], args);
					assert(chk1 != -1);
				}					
			}

			int pid = fork(); ///////////////////////////////////FORK

			// If fork returns 0, it's the child, -1 if error,
			// else returns child's PID if it's the parent
			if (pid == -1)
				perror("Error forking.\n");    
			else if (pid != 0)
				wait(NULL);	
			else // It's the child process
			{
				//run process if no pipe, run second process if piping
				execvp(args[0], args);
				printf("Error!\n");
			}		
		}		
	}
	free(args);
	free(cmd);
	
	exit(0);
}

void ReadCommand(char** args, char* cmd, size_t cmd_size)
{
	
	// Wait for user input, returns # chars read (-1 on error)
	int stdin_num_bytes = getline(&cmd, &cmd_size, stdin);

	// Check getline return value 
	if(stdin_num_bytes == -1)
	{
		printf("Error!\n");	
		perror(NULL);	
	}
	else // Parse words out of cmd
	{
		//bool lastWord = false;
		//int curChar = 0;
		int index = 0;	
					
		args[0] = strtok(cmd, " \n");
		
		for(index = 1; index < 50; index++)
		{
			args[index] = (char *) malloc(sizeof(char*));
			args[index] = strtok(NULL, " \n");
			//break;
			//args[index+1]=NULL;	
		}
		args[index+1]=NULL;

		//	while(!lastWord)
		//{				
		//args[index] = ParseWord(&cmd, &curChar, stdin_num_bytes, &lastWord);		
			
		//	index++;
		//}
		
		// Check for special features
		
	}
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

/*
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
*/
