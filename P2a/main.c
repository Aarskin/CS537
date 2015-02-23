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
	char *pCmd[2];
	char *buf = (char*) malloc(512*sizeof(char));
	size_t buf_size = sizeof(buf);
	int te;
	char *bufT = (char*) malloc(512*sizeof(char));
	size_t bufT_size = sizeof(bufT);
	
	// Assume 1024 char max +1 for NULL (from spec)
	char*	cmd = (char*) malloc(1025 * sizeof(char));
	size_t	cmd_size = sizeof(cmd); // 1025 bytes (1/char)
	// Max possible words under assumption #4
	char* args[50];	
//char** args = (char**) malloc(512*sizeof(char*)); // Wasteful with memory atm *shrug*
	
	while(true) // Run forever (until ctrl+c, of course)
	{
		// Prompt
		printf("mysh> ");
		
		// Wait for user input, returns # chars read (-1 on error)
		stdin_num_bytes = getline(&cmd, &cmd_size, stdin);
	
		if(cmd[0] == '\n')
			continue;
		// Check getline return value 
		if(stdin_num_bytes == -1)
		{
			printf("Error!\n");	
			//perror(NULL);	
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

			//	while(!lastWord)
			//{				
				//args[index] = ParseWord(&cmd, &curChar, stdin_num_bytes, &lastWord);		
				
			//	index++;
			//}
			
			// Check for special features
			oRedirect = CheckOverwriteRedirect(args);
			aRedirect = CheckAppendRedirect(args);
			pip = CheckPipe(args);
			te = CheckTee(args);
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
		else // fork and then execvp command
		{
//call mytee////////////////////////////////////////////////////////////
			//int feedT[2];			

			if(te != -1)
			{
			
				//mytee( char** args, int fd[2]);
				mytee(args, bufT, bufT_size,  te);
				continue;
			}
			mode_t mode = S_IRUSR | S_IWUSR | S_IWGRP | S_IROTH;

				int feed[2];
				//if piping, fork to run first process 
				//this should output to file since we piped output to file
				if(pip != -1)
				{
					
					pCmd[0] = args[pip+1];					
					args[pip]=NULL;	
									
					int chk2 = pipe(feed);
					assert(chk2 != -1);
				
					int pid1 = fork();
					if(pid1 == -1)
						printf("Error!\n");
					//parent if pid2 != 0
					else if(pid1 != 0)
					{		   
						
						wait(NULL);
					}
					else{
						//close(1);
						dup2(feed[1], 1);
						close(feed[1]);
						
						int chk1 = execvp(args[0], args);
						assert(chk1 != -1);
						exit(0);
					}					
				}


			int pid = fork(); ///////////////////////////////////FORK

			// If fork returns 0, it's the child, -1 if error,
			// else returns child's PID if it's the parent
			if (pid == -1)
		printf("Error!\n");				
		//perror("Error forking.\n");    
			else if (pid != 0)
			{
				wait(NULL);	
			}
			else // It's the child process
			{
				// Handle special feature filestreams

				//handle append redirect
				if(aRedirect != -1)
				{
					file = open(args[aRedirect+1], O_CREAT | O_RDWR | O_APPEND | O_CLOEXEC, mode);
					args[aRedirect] = NULL; 
					assert(file != -1);
				
					int chk = dup2(file, 1);
					//assert(chk != -1);			    
if(chk == -1)
printf("Error!\n");
				
					
				
				}
//overwrite redirect
			if(oRedirect != -1)
			{
				file = open(args[oRedirect+1], O_CREAT | O_RDWR | O_TRUNC|O_CLOEXEC, mode);
				args[oRedirect] = NULL; // Terminate the 
				assert(file != -1);

				int chk = dup2(file, 1);
				//assert(chk != -1);
				
			    //what about redirects with no spaces around >
if(chk == -1)
				printf("Error!\n");
			}

				//parse redirected input
				if(pip != -1)
				{
					dup2(feed[0], 0);
					close(feed[0]);

					int i;
					getline(&buf, &buf_size, stdin);
					args[0] = pCmd[0];
					args[1] = strtok(buf, " \n");
					for(i = 2; i < 50; i++)
					{
						args[i] = strtok(NULL, " \n");						
					}													
				}
				//run process if no pipe, run second process if piping
				execvp(args[0], args);
				
				printf("Error!\n");
				//exit(0);
			}		
		}
		
	}
	free(buf);
	free(bufT);
	free(cmd);
	int i;
	for(i=0; i<50; i++)
		free(args[i]);	
	
	exit(0);
}


//executes a, pipes output to b and write to tee.txt
//returns modified args to be run by b
void mytee( char** args, char* buf, size_t buf_size, int te)
{
	int fd[2];
	int chk = pipe(fd);
	assert(chk != -1);
	char* tCmd = args[te+1];
	args[te] = NULL;
	mode_t mode = S_IRUSR | S_IWUSR | S_IWGRP | S_IROTH;
	   	int file = open("tee.txt", O_CREAT | O_RDWR | O_TRUNC | O_CLOEXEC,mode);


	int pid = fork();	
	if(pid == -1)	
printf("Error!\n");
		//perror(NULL);
	else if(pid != 0)
	{
		wait(NULL);
		close(fd[1]);
	}
	else
	{
		dup2(fd[1], 1);		
		close(fd[1]);

		int chk = execvp(args[0], args);
		assert(chk != -1);
		exit(0);
	}
/////////
	int pid0 = fork();
	if(pid0 == -1)
	{
	printf("Error!\n");
	//perror(NULL);		
	}
	else if(pid0 != 0)
	{		
		wait(NULL);
		close(file);
	}
	else
	{
		dup2(file, 0);
		close(file);
	}

//////
	int pid2 = fork();
	if(pid2 == -1)
		printf("Error!\n");
//		perror(NULL);
	else if(pid2 != 0)
	{
		close(fd[0]);
		wait(NULL);
	}
	else
	{
		dup2(fd[0], 0);
		close(fd[0]);
		
		//parse
		int i;
		getline(&buf, &buf_size, stdin);
		args[0] = tCmd;
		args[1] = strtok(buf, " \n");
		for(i = 2; i < 50; i++)
		{
			args[i] = strtok(NULL, " \n");						
		}
		execvp(args[0], args);
	}
}


int CheckTee(char** args)
{
	int i = 0;
	int sym = -1;	
	while(args[i] != NULL)
	{
		if(strncmp(args[i], "%", sizeof("%")) == 0)
		{
			sym = i;
			break;		
		}		
		++i;
	}	
	return sym;
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
