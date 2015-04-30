#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "xfs.h"

int main(int argc, char* argv[])
{
	FILE* file;
	char* buff;
	struct superblock* super;
	size_t bmk, superblocksize;

	superblocksize = sizeof(struct superblock);
	file = fopen ("xfs.img" , "r+");
	if (file == NULL) perror(0);
	fseek(file, BSIZE, SEEK_SET); // seek to the superblock

	buff = malloc(superblocksize);
	if(buff == NULL) perror(0);
	
	bmk = fread(buff, superblocksize, 1, file);
	if (bmk != 1) perror(0);
	
	super = (struct superblock*)buff;
	printf("size:\t\t%d\n", super->size);
	printf("nblocks:\t%d\n", super->nblocks);
	printf("ninodes:\t%d\n", super->ninodes);	
	
	// terminate
	fclose (file);
	free (buff);
	
	return printf("Finished\n");
}
