#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "types.h"
#include "methods.h"

int main(int argc, char* argv[])
{
	int	stdin_num_bytes;
	int	oRedirect;
	int	aRedirect;
	
	// Assume 1024 char max +1 for NULL (from spec)
	char*	cmd = (char*) malloc(1025 * sizeof(char));
	size_t	cmd_size = sizeof(cmd); // 1025 bytes (1/char)
	// Max possible words under assumption #4
	char* args[512]; // Wasteful with memory atm *shrug*
	
	while(true) // Run forever (until ctrl+c, of course)
	{
		// Prompt
		printf("mysh>");
		
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
			
			// Handle special feature filestreams
			if(oRedirect != -1)
			{
				int file = open(args[oRedirect+1], O_CREAT | O_RDWR);
				args[oRedirect] = NULL; // Terminate the 
				
				// PICK UP HERE!!
				int chk = dup2(1, file);
				perror(NULL);
				
			}

			// If fork returns 0, it's the child, -1 if error,
			// else returns child's PID if it's the parent
			if (pid == -1)
				perror("Error forking.\n");    
			else if (pid != 0)
				wait(NULL);		
			else // It's the child process
			{
				
				execvp(args[0], args);
				
				printf("execvp failed!\n");
				exit(0);
			}		
		}
		
	}	
	
	exit(0);
}

/*
void chdir(char * newdir){
	int errChk = chdir(newdir );
		if(errChk == -1)
			fprintf(stderr, "Error!\n");
}

*/

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
