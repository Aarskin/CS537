#include "types.h"
#include "user.h"

int main(int argc, char* argv[])
{
	printf(1, "Hello World! - Userspace\n");
	settickets(); // "Hello World! - Kernelspace" atm
	exit();
}
