#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char *argv[]) {
	int fd = open(argv[1], 0);
	char buf[256];

	int x = getfilename(fd, buf, 256);
	
	if(x==0) {
		printf(1, "XV6_TEST_OUTPUT Open filename: %s\n", buf);
	} else {
		printf(2, "oh no\n");
	}
	close(fd);
	exit();
}
