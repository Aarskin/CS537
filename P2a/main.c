#include <stdlib.h>
#include <stdio.h>
#include "types.h"

int main(int argc, char* argv[])
{
	int stdin_num_bytes;
	size_t assumed_num_chars = 1024; // char == 1 byte
	char* cmd = (char*) malloc(assumed_num_chars + 1);
		
	while(true) // Run forever (until ctrl+c, of course)
	{
		// Prompt
		printf("mysh>");
		
		// Wait for user input
		stdin_num_bytes = getline(&cmd, &assumed_num_chars, stdin);
	
		// Check input
		if(stdin_num_bytes == -1)
		{
			printf("Error!\n");		
		}
		else // Parse cmd
		{
			
		}
	
	}	
	
	exit(0);
}
