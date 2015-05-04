#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "xfs.h"

int main(int argc, char* argv[])
{
	FILE* file;
	FILE* view;
	char* buff;
	struct superblock* super;
	struct dinode* inodes;
	size_t bmk, superblocksize, dinodesize, direntsize;

	superblocksize = sizeof(struct superblock);
	dinodesize = sizeof(struct dinode);
	direntsize = sizeof(struct dirent);
	
	// Open the image
	file = fopen ("xfs.img", "r+");
	view  = fopen ("view.txt", "w+");
	if (file == NULL) perror(0);
	
	// Seek to the superblock (block 1)
	fseek(file, BSIZE, SEEK_SET); 
	
	// Read the superblock
	buff = malloc(superblocksize);
	if(buff == NULL) perror("SUPER!");
	bmk = fread(buff, superblocksize, 1, file);
	if (bmk != 1) perror("SUPER@");
	super = (struct superblock*)buff;

	// Seek to the first inode (block 2)
	fseek(file, (IBLOCK(1)*BSIZE) + dinodesize, SEEK_SET); 
	
	// Read in all the inodes
	buff = malloc(dinodesize*super->ninodes);
	if(buff == NULL) perror("DINODE!");
	bmk = fread(buff, dinodesize, super->ninodes, file);
	if(bmk != super->ninodes) perror("DINODE@");
	inodes = (struct dinode*)buff;
	
	// Seek to the bitmap (block 28)
	fseek(file, 28*BSIZE, SEEK_SET);
	
	// Bitmap should be the block just after the inodes (SEEK_CUR)
	buff = malloc(BSIZE);
	if(buff == NULL) perror("BITMAP!");
	bmk = fread(buff, BSIZE, 1, file);
	if(bmk != 1) perror("BITMAP@");
	
	// Check it
	fsck(super, inodes, buff);

	// Free it
	fclose (file);
	free (buff);

	// Done	
	return printf("Finished\n");
}

bool blockValid(int block, char* bmap)
{
	int index, sa;
	
	// Each index represents one char, == one byte, == 8 bits
	// Need the to index to the proper set of 8 bits to check
	index = block / 8;
	
	// Determine the shift amount into the 8 bits. 7 because if the modulo is 0
	// we want to mask off the MSB. E.g. block 16 should be the MSB of index 2,
	// but the modulo is 0, which would give us the LSB
	sa = 7 - (block % 8);
	
	// This will be 0 unless we masked off a 1 - indicating a valid block
	if((bmap[index] & (1 << sa)) > 0)
		return true;
	
	return false;
}

int fsck(struct superblock* super, struct dinode* inodes, char* bmap)
{
	int i = 0;
	
	for(i = 0; i < 1024; i++)
	{
		printf("Block %d:\t%d\n", i, blockValid(i, bmap));
	}

	return i;
}
