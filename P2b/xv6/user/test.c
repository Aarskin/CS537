#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char* argv[])
{
	printf(1, "Hello World! - Userspace\n");
	printf(1, "settickets returned: %d! - Kernelspace\n", settickets(35));
	printf(1, "getpinfo returned: %d! - Kernelspace\n", getpinfo());
	exit();
}
