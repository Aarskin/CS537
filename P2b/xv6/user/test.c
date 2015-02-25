#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char* argv[])
{
	printf(1, "Hello World! - Userspace\n");
	printf(1, "settickets %d! - Kernelspace\n", settickets());
	printf(1, "getpinfo %d! - Kernelspace\n", getpinfo());
	exit();
}
