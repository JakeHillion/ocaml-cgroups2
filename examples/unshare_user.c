#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

int main() {

	int fd;

	if (unshare(CLONE_NEWUSER))
		perror("unshare");
	if ((fd = open("/proc/self/uid_map", O_WRONLY)) == -1)
		perror("open");	
	if (write(fd, "0 0 4294967295\n", 15) == -1)
		perror("write");
	if (execl("/bin/bash", "/bin/bash", NULL))
		perror("execl");
}

