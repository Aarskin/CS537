#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char* argv[])
{
	printf(1, "Hello World! - Userspace\n");
	printf(1, "settickets returned: %d! - Kernelspace\n", settickets(33));
	printf(1, "settickets returned: %d! - Kernelspace\n", settickets(9));
	printf(1, "settickets returned: %d! - Kernelspace\n", settickets(10));
	//printf(1, "settickets returned: %d! - Kernelspace\n", settickets(150));
	printf(1, "settickets returned: %d! - Kernelspace\n", settickets(151));
	printf(1, "getpinfo returned: %d! - Kernelspace\n", getpinfo());

	exit();
}
