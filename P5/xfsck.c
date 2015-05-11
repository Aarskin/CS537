#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include "xfs.h"

FILE* file;
size_t bmk, superblocksize, dinodesize, direntsize, expected;
struct superblock* super;
struct dinode* inodes;
struct fsck_status* status;

struct cross_ref
{
	bool bitmap;
	bool inoderef;
};

void bitmap_dump(char* bmap)
{
	int i;
	
	for(i = 0; i < 1024; i++)
	{
		printf("block %d:\t%d\n", i, blockValid(i, bmap));	
	}
}

void inode_dump()
{
	int i, j;
	struct dinode* node;
	
	for(i = 0; i < 35; i++)
	{
		node = &inodes[i];
		printf("INODE # %d\t", i);

		if(node->type > 3 || node->type < 0)
			printf("*****");

		printf("Type:\t%hd\t", node->type);
		printf("Links:\t%hd\t", node->nlink);
		printf("Size:\t%d\t", node->size);
		printf("Blocks: | ");
		
		for(j = 0; j < 13; j++)
		{
			printf("%d | ", node->addrs[j]);
		}

		printf("\n");
	}
}

int main(int argc, char* argv[])
{
	char* buff;
	bool OK = false;	
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
	
	// How big do we expect this image to be?
	fseek(file, 0, SEEK_END);
	expected = ftell(file);
	expected = expected/BSIZE;
	
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
	size_t tmp = ftell(file); // where are we?
	fseek(file, tmp, SEEK_SET); 
	
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
	
	inode_dump();
	
	int i = 0;
	
	// Check it until we don't find any errors
	while(1)
	{
		// debugging output
		i++;
		printf("Pass: %d\n", i);
		
		status = fsck(super, inodes, buff);
		
		if(status->error_found && !status->error_corrected)
			break; // Game over, man. Game over.
		else if(!(status->error_found || status->error_corrected))
		{
			OK = true; // Don't do this anywhere but here
			break;
		}
	}
	
	
	
	// debug only ///////////////////////////////////////////////////////////////
	
	fflush(file);
	
	// Seek to the first inode (block 2)
	fseek(file, (IBLOCK(1)*BSIZE), SEEK_SET); 
	
	// Read in all the inodes
	buff = malloc(dinodesize*super->ninodes);
	if(buff == NULL) perror("DINODE!");
	bmk = fread(buff, dinodesize, super->ninodes, file);
	if(bmk != super->ninodes) perror("DINODE@");
	inodes = (struct dinode*)buff;
	
	inode_dump();
	
	/////////////////////////////////////////////////////////////////////////////

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

// Write a fix to the specified block number
void write_fix(int block_num, int offset, char* fix, size_t size)
{

	size_t pos = ftell(file); // save
	size_t loc = block_num*BSIZE+(dinodesize*offset);
	fseek(file, loc, SEEK_SET);
	assert(fwrite(fix, size, 1, file) == 1);
	
	printf("wrote: %s\t to %zd\n", fix, loc);
	
	fseek(file, pos, SEEK_SET);
}

void directoryCheck(struct dinode* inode, int inum)
{
	int i, j, block;
	size_t pos = ftell(file); // save
	size_t direntsize = sizeof(struct dirent);
	int numentries = inode->size/direntsize;
	char* dirbuff = malloc(direntsize*numentries);
	struct dirent* head;
	struct dirent* dir;
	
	// Loop over the data blocks (dirents)
	for(i = 0; i < NDIRECT+1; i++)
	{
		block = inode->addrs[i];
		
		if(block == 0)
			continue;
		
		// Seek to the array of DIRENT structs
		assert(fseek(file, block*BSIZE, SEEK_SET) == 0);
		assert(fread(dirbuff, direntsize, numentries, file) == numentries);
		head = (struct dirent*)dirbuff;
		
		printf("Directory @ block: %d\n", block);
		printf("-----------------\n");
		// Loop over the actual entries
		for(j = 0; j < numentries; j++)
		{
			dir = &head[j];
			bool error = false;
				
			if(j < 2) // Validate "." & ".."
			{				
				
				if(j == 1 && strncmp(dir->name, ".", sizeof(dir->name)) == 0)
					error = true;
				if(j == 2 && strncmp(dir->name, "..", sizeof(dir->name)) == 0)
					error = true;			
				if(inodes[dir->inum].type != 1)
					error = true;
					
				if(error) // wipe the inode
				{
					status->error_found = true;
					// Probably corrupted, wipe the inode that got us here, write-back
					// STILL NEED TO UPDATE BITMAP						
					memset(inode, 0, dinodesize);
					write_fix(IBLOCK(inum), inum%IPB, (char*)inode, dinodesize);
					status->error_corrected = true;
				}
			}
			
			if(dir->inum == 0)
				continue;			
				
			printf("%d: ", dir->inum);
			printf("%s\n", dir->name);
		}
		printf("-----------------\n");
	}
	
	// Transparent, put the file pos back where you found it
	fseek(file, pos, SEEK_SET);
}

struct fsck_status* fsck(struct superblock* super, struct dinode* inodes, char* bmap)
{
	int i, j;
	int refblocks = NDIRECT + 1;
	uint blocknum;
	uint bitblocks, usedblocks;
	struct dinode* node;
	struct cross_ref blockRefs[super->size];
	status = malloc(sizeof(struct fsck_status));
	
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
  	
  	if(super->size != expected)
  	{
  		status->error_found = true;
  		char* fix = malloc(superblocksize);
  		((struct superblock*)fix)->size = expected;
  		((struct superblock*)fix)->ninodes = super->ninodes;
  		((struct superblock*)fix)->nblocks = super->nblocks;
  		write_fix(1, 0, fix, superblocksize);
  		status->error_corrected = true;
  	}
  	
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
	for(i = 1; i < super->ninodes; i++)
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
		else if(node->type == 1)
			directoryCheck(node, i);
		
		// Link count sanity check
		// ???
		
		// Cross-ref blocks used by this inode with blocks marked used by bitmap
		for(j = 0; j < refblocks; j++)
		{
			blocknum = node->addrs[j];
			
			if(blocknum == 0) continue; // empty reference
			
			if(blocknum > super->size)
			{
				status->error_found = true;			
				memset(&inodes[i], 0, sizeof(inodes[i])); // Clear it
				status->error_corrected = true;
			}				
			
			blockRefs[blocknum].inoderef = true;
		} 
	}

	return status;
}
