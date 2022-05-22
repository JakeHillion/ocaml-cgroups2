#include <fcntl.h>
#include <linux/types.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

int main() {
  int dirfd;

  if ((dirfd = open("/tmp", __O_DIRECTORY)) < 0)
    perror("opendir");
  if (openat(dirfd, "filethatdoesexist", O_RDONLY) < 0)
    perror("openat0");
  if (chroot("/tmp"))
    perror("chroot");
  if (open("/etc/passwd", O_RDONLY) < 0)
    perror("open");
  if (openat(dirfd, "../etc/passwd", O_RDONLY) < 0)
    perror("openat1");
  if (openat(dirfd, "filethatdoesexist", O_RDONLY) < 0)
    perror("openat2");
}

