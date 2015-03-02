#include "types.h"
#include "pstat.h"
#include "user.h"


void p1()
{
	int i = 0;
	while(i<3)
	{
		i++;
		sleep(5);
	}
} 

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
/*
	settickets(20);
	struct pstat p[64];	
	//printf(1, "getpinfo returned: %d! - Kernelspace\n", getpinfo(p));
	
		//first child
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

	int i = 0;
	while(i < 5 )
	{
		getpinfo(p);	
		i++;
		sleep(1);
	}
	exit();
	*/
}



