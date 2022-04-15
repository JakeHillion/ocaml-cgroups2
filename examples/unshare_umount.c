#define _GNU_SOURCE
#include <sched.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>

int main() {
	if (unshare(CLONE_NEWNS))
		perror("unshare");
	if (mount("none", "/", NULL, MS_REC|MS_PRIVATE, NULL))
		perror("mount");
	if (umount("/"))
		perror("umount");
}
