#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "fs.h"

int main(int argc, char* argv[])
{
	FILE* file;
	FILE* inspect;
	long size;
	char* buff;
	size_t bmk;
	/*
	checkpoint chkpt;
	dirEnt dir;
	inode node;
	inodeMap map;
	*/	

	file = fopen ("fs.img" , "r+");
	inspect = fopen("inspect.txt", "w+");
	if (file == NULL) perror(0);

	// obtain file size:
	fseek (file , 0 , SEEK_END);
	size = ftell (file);
	rewind (file);

	// allocate memory to contain the whole file
	buff = (char*) malloc (sizeof(char)*size);
	if (buff == NULL) perror(0);
	
	// copy the file into the buff:
	bmk = fread (buff, sizeof(checkpoint), size, file);
	if (bmk != size) perror(0);

	// test write
	char* test = "Test";
	fwrite(test, sizeof(char), sizeof(test), inspect);

	// terminate
	fclose (file);
	free (buff);
	
	return printf("Finished\n");
}
