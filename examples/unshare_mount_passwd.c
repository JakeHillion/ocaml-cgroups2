#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#define BUF_SIZE 1024

void print_file(int fd);

int main() {
	int fd;
	
	if ((fd = open("/etc/passwd", O_RDONLY)) < 0)
		perror("open");
	print_file(fd);
	if (close(fd))
		perror("close");
	
	if (unshare(CLONE_NEWNS))
		perror("unshare");
	printf("----- unshared -----\n");
	
	if ((fd = open("/etc/passwd", O_RDONLY)) < 0)
		perror("open");
	print_file(fd);
	if (close(fd))
		perror("close");
}

void print_file(int fd) {
	char buf[BUF_SIZE];
	int bytes_read;

	while ((bytes_read = read(fd, buf, BUF_SIZE)) > 0)
		if (write(1, buf, bytes_read) < 0)
			perror("write");

	if (bytes_read == -1)
		perror("read");
}

