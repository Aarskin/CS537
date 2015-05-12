#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>
#include "lfs.h"

#define BSIZE 4096

FILE* img;

int main(int argc, char* argv[])
{
	if(argc < 2)
	{
		fprintf(stderr, "Usage: lfsreader <cat, ls> <abs path> <img file>");
		exit(0);
	}
	
	assert(fopen(argv[3], "r") != 0);
	
	
	exit(0);
}
