#include "types.h"
#include "pstat.h"
#include "user.h"

int main(int argc, char* argv[])
{
	
	
	
	struct pstat p[64];
	
	settickets(20);
	
		int pid = fork();
		if(pid < 0)
			printf(1, "Fork failed");
		else if(pid > 0) //parent
			wait();
		else //child
		{				
			p1();
			settickets(100);			
		}
	
	
	getpinfo(p);	
	exit();
}
