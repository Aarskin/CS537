#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char* argv[])
{
	printf(1, "Hello World! - Userspace\n");
	printf(1, "High %d! - Kernelspace\n", settickets());
	exit();
}
