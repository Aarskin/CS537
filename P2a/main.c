#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "types.h"
#include "methods.h"

int main(int argc, char* argv[])
{
	int	stdin_num_bytes;
	
	// Assume 1024 char max +1 for NULL (from spec)
	char*	cmd = (char*) malloc(1025 * sizeof(char));
	size_t	cmd_size = sizeof(cmd); // 1025 bytes (1/char)
	
	while(true) // Run forever (until ctrl+c, of course)
	{
		// Prompt
		printf("mysh>");
		//printf("Sizeof(int): %lu\t", sizeof(int)); // 4 bytes
		
		// Wait for user input, returns # chars read (-1 on error)
		stdin_num_bytes = getline(&cmd, &cmd_size, stdin);
	
		// Check getline return value 
		if(stdin_num_bytes == -1)
		{
			printf("Error!\n");		
		}
		else // Parse cmd
		{
			bool lastWord = false;
			int curChar = 0;
			int index = 0;
			// Max possible words under assumption #4
			char* words[512]; // Wasteful with memory atm *shrug*
			
			
			while(!lastWord)
			{
				words[index] = ParseWord(&cmd, &curChar, stdin_num_bytes);
				index++;
			}
			
			
			//char* prog = 
			//char* args =
			
			//printf("The program is: %s\n", program);
			//printf("The arguments are: %s\n", args);
			printf(words[0]);
		}	
	}	
	
	exit(0);
}

char* ParseWord(char** cmd, int* pos, int cmd_size)
{
	char* word = *cmd;
	char* temp = word;
	
	// Tack characters on until we hit whitespace (or the end)
	while(*pos < cmd_size)
	{
		if(!isspace(**cmd)) // Advance to the next char
		{
			temp++;
			(*cmd)++;
			//*pos++;		
		}
		else
		{
			
			*temp = 0; // Null terminate this word
			return word;
		}
	}
	
	return word;
}
