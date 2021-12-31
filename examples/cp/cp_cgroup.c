#define _GNU_SOURCE
#include <fcntl.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define IO_BLOCK_SIZE 4096

/*
 * Get size of regular file or error otherwise.
 */
off_t get_file_size(int fd) {
  struct stat st;

  if (fstat(fd, &st) < 0) {
    perror("fstat");
    return -1;
  }

  if (S_ISREG(st.st_mode)) {
    return st.st_size;
  } else {
    return -1;
  }
}

/*
 * Move self to a leaf node of the cgroup.
 */
int leaf_self() {}

/*
 * Perform the copy between the two file descriptors.
 */
int copy(int src_fd, int dst_fd) {
  off_t src_len;

  int passwd_fd;
  if ((passwd_fd = open("/home/jake/plot", O_RDONLY)) == -1) {
    perror("open 2");
    return 1;
  } else {
    // TODO: Debug this
    fprintf(stderr, "opened second file successfully\n");
  }

  if ((src_len = get_file_size(src_fd)) == -1) {
    return 1;
  }

  char buf[IO_BLOCK_SIZE];
  off_t blocks =
      (src_len / IO_BLOCK_SIZE) + ((src_len % IO_BLOCK_SIZE) > 0 ? 1 : 0);

  for (off_t i = 0; i < blocks; i++) {
    off_t len;
    if (i == blocks - 1 && src_len % IO_BLOCK_SIZE != 0) {
      len = src_len % IO_BLOCK_SIZE;
    } else {
      len = IO_BLOCK_SIZE;
    }

    if (read(src_fd, &buf, len) == -1) {
      perror("read");
      return -1;
    };
    if (write(dst_fd, &buf, len) == -1) {
      perror("write");
      return -1;
    }
  }

  return 0;
}

int main(int argc, char **argv) {
  if (argc != 3) {
    fprintf(stderr, "usage: %s <src> <dst>\n", argv[0]);
    return 1;
  }

  int src_fd, dst_fd;

  if ((src_fd = open(argv[1], O_RDONLY)) == -1) {
    perror("open");
    return 1;
  }
  if ((dst_fd = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0666)) == -1) {
    perror("open");
    return 1;
  }

  // all necessary files opened, drop filesystem
  if (unshare(CLONE_FS) == -1) {
    perror("unshare");
    return -1;
  }

  // fork and copy in a second process
  pid_t child_pid = fork();
  if (child_pid == -1) {
    perror("fork");
    return -1;
  } else if (child_pid == 0) {
    // pause(); // await cgroup setup
    raise(SIGSTOP); // await cgroup setup
    return copy(src_fd, dst_fd);
  }

  // setup a cgroup for the child

  // wait for the child to pause and restart it
  int status;
  if (waitpid(child_pid, &status, WUNTRACED | WSTOPPED) == -1) {
    perror("waitpid");
    return -1;
  }

  if (WIFSTOPPED(status)) {
    if (kill(child_pid, SIGCONT) == -1) {
      perror("kill");
      return -1;
    }
  } else {
    fprintf(stderr, "child behaved unexpectedly\n");
    return -1;
  }

  // wait for the child process to terminate
  if (waitpid(child_pid, &status, 0) == -1) {
    perror("waitpid");
    return -1;
  }

  if (WIFEXITED(status)) {
    return WEXITSTATUS(status);
  }

  return -1;
}
