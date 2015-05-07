#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include "xfs.h"

struct cross_ref
{
	bool bitmap;
	bool inoderef;
};

size_t bmk, superblocksize, dinodesize, direntsize;

int main(int argc, char* argv[])
{
	FILE* file;
	char* buff;
	bool OK = false;
	struct superblock* super;
	struct dinode* inodes;	
	struct fsck_status* status;
	
	if(argc != 2)
	{
		fprintf(stderr, "Usage: myfsck <image file>...\n");
		exit(1);
	}

	superblocksize = sizeof(struct superblock);
	dinodesize = sizeof(struct dinode);
	direntsize = sizeof(struct dirent);
	
	// Open the image
	file = fopen (argv[1], "r+");
	if (file == NULL) 
	{
		perror("File Error");
		exit(1);
	}
	
	// Seek to the superblock (block 1)
	fseek(file, BSIZE, SEEK_SET); 
	
	// Read the superblock
	buff = malloc(superblocksize);
	if(buff == NULL) perror("SUPER!");
	bmk = fread(buff, superblocksize, 1, file);
	if (bmk != 1) perror("SUPER@");
	super = (struct superblock*)buff;

	// Seek to the first inode (block 2)
	fseek(file, (IBLOCK(1)*BSIZE), SEEK_SET); 
	
	// Read in all the inodes
	buff = malloc(dinodesize*super->ninodes);
	if(buff == NULL) perror("DINODE!");
	bmk = fread(buff, dinodesize, super->ninodes, file);
	if(bmk != super->ninodes) perror("DINODE@");
	inodes = (struct dinode*)buff;
	
	// Seek to the bitmap (block ?)
	fseek(file, 28*BSIZE, SEEK_SET);
	
	// Bitmap should be the block just after the inodes (SEEK_CUR)
	buff = malloc(128);
	if(buff == NULL) perror("BITMAP!");
	bmk = fread(buff, 128, 1, file);
	if(bmk != 1) perror("BITMAP@");
	
	// Check it until we don't find any errors
	while(1)
	{
		status = fsck(super, inodes, buff);
		
		if(status->error_found && !status->error_corrected)
			break; // Game over, man. Game over.
		else if(!(status->error_found || status->error_corrected))
		{
			OK = true; // Don't do this anywhere but here
			break;
		}
	}

	// Free it
	fclose (file);
	free (buff);

	// Done
	//printf("Finished: ");
	
	if(!OK)
		printf("Error\n");

	exit(0);		
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
	sa = (block % 8);
	
	// This will be 0 unless we masked off a 1 - indicating a valid block
	if((bmap[index] & (1 << sa)) > 0)
		return true;
	
	return false;
}

void bitmap_dump(char* bmap)
{
	int i;
	
	for(i = 0; i < 1024; i++)
	{
		printf("block %d:\t%d\n", i, blockValid(i, bmap));	
	}
}

void inodes_dump(struct dinode* inodes)
{
	int i, j;
	struct dinode* node;
	
	for(i = 0; i < 200; i++)
	{
		printf("\n");
		node = &inodes[i];
		printf("INODE # %d\n", i);

		if(node->type > 3 || node->type < 0)
			printf("---------------------------------------------------------------------------------------------->");

		printf("Type:\t%hd\n", node->type);
		printf("Links:\t%hd\n", node->nlink);
		printf("Size:\t%d\n", node->size);
		printf("Blocks: | ");
		
		for(j = 0; j < 13; j++)
		{
			printf("%d | ", node->addrs[j]);
		}

		printf("\n");
	}
}

struct fsck_status* fsck(struct superblock* super, struct dinode* inodes, char* bmap)
{
	int i;
	uint bitblocks, usedblocks;
	struct dinode* node;
	struct cross_ref blockRefs[super->size];
	struct fsck_status* status = malloc(sizeof(struct fsck_status));
	
	// debug only
	//bitmap_dump(bmap);
	//inodes_dump(inodes);
	
	// Initialize
	status->error_found = false;
	status->error_corrected = false;
	
	// Raw size computations (bytes)
	uint fs_size = super->size*BSIZE;
	uint blank_and_super_size = 2*BSIZE;
	uint inode_size = super->ninodes*dinodesize;
	uint bitmap_size = super->size/8;
	uint data_size = super->nblocks*BSIZE;
	uint all_together_now = blank_and_super_size + inode_size + bitmap_size + data_size;
	
	
	// Make sure the superblock makes sense *************************************
	if(fs_size < all_together_now)
	{
		status->error_found = true;
		return status;
	}
	
	bitblocks = super->size/(512*8) + 1;
  	usedblocks = super->ninodes / IPB + 3 + bitblocks;
  	
  	if(super->nblocks + usedblocks != super->size)
  	{
  		status->error_found = true;
  		return status;
  	}
  	// **************************************************************************
	
	// Overhead for crossreferencing bitmap with inodes
	for(i = 0; i < super->size; i++)
	{
		blockRefs[i].bitmap = blockValid(i, bmap); // What's the bitmap think now?
		blockRefs[i].inoderef = false; // Initialize
	}
	
	// "When you encounter a bad inode, the only thing you can do is clear it."
	for(i = 0; i < super->ninodes; i++)
	{
		node = &inodes[i];
		
		// Size sanity check
		if(node->size > data_size)
		{
			status->error_found = true;			
			memset(&inodes[i], 0, sizeof(inodes[i])); // Clear it			
			status->error_corrected = true;
		}
		
		// Type sanity check
		if(	node->type != 0 
			&& node->type != 1 
			&& node->type != 2 
			&& node->type != 3)
		{
			status->error_found = true;			
			memset(&inodes[i], 0, sizeof(inodes[i])); // Clear it			
			status->error_corrected = true;
		}
		
		// Link count sanity check
		// ???
	}

	return status;
}
