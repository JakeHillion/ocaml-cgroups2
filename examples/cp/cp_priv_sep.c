// For privilege separation
#include <clone3.h>
#include <fcntl.h>
#include <linux/wait.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// This program
#include <stdio.h>

#define BUF_SIZE 4096

struct copy_args {
  char *src;
  char *dst;
};

int process_args(int argc, char **argv, struct copy_args *);
int do_copy(struct copy_args *args);

int main(int argc, char **argv) {
  pid_t child;

  // Spawn args processor
  int args_pid_fd;

  struct clone_args args_processor_cl_args = {
      .flags = CLONE_NEWIPC | CLONE_NEWNET | CLONE_NEWNS | CLONE_NEWPID |
               CLONE_NEWUSER | CLONE_NEWUTS | CLONE_PIDFD,
      .pidfd = (uint64_t)&args_pid_fd,
      .child_tid = (uint64_t)NULL,
      .parent_tid = (uint64_t)NULL,
      .exit_signal = SIGCHLD,
      .stack = (uint64_t)NULL,
      .stack_size = 0,
      .tls = (uint64_t)NULL,
  };

  int args_pipe_fds[2];
  if (pipe(args_pipe_fds) < 0) {
    perror("pipe");
    exit(-1);
  }

  if ((child = clone3(&args_processor_cl_args)) == 0) {
    char args[sizeof(struct copy_args)];

    int code = process_args(argc, argv, &args);
    if (code != 0) {
      exit(code);
    }

    write(args_pipe_fds[1], &args, sizeof(args));
    exit(0);
  } else if (child < 0) {
    perror("clone3");
    return -1;
  }

  // Wait for args processor
  // Implicit assertion that sizeof(args) < the pipe buffer size (won't block)
  siginfo_t status;
  if (waitid(P_PIDFD, args_pid_fd, &status, WEXITED) == -1) {
    perror("waitid");
    return -1;
  }

  if (status.si_status != 0) {
    return status.si_status;
  }

  char args[sizeof(struct copy_args)];
  read(args_pipe_fds[0], &args, sizeof(args));

  // Spawn copier (can be more separated)
  int copy_pid_fd;

  struct clone_args do_copy_cl_args = {
      .flags = CLONE_NEWIPC | CLONE_NEWNET | CLONE_NEWPID | CLONE_NEWUTS |
               CLONE_PIDFD,
      .pidfd = (uint64_t)&copy_pid_fd,
      .child_tid = (uint64_t)NULL,
      .parent_tid = (uint64_t)NULL,
      .exit_signal = SIGCHLD,
      .stack = (uint64_t)NULL,
      .stack_size = 0,
      .tls = (uint64_t)NULL,
  };

  if ((child = clone3(&do_copy_cl_args)) == 0) {
    int code = do_copy(&args);
    exit(code);
  } else if (child < 0) {
    perror("clone3");
    return -1;
  }

  if (waitid(P_PIDFD, copy_pid_fd, &status, WEXITED) == -1) {
    perror("waitid");
    return -1;
  }

  return status.si_status;
}

int process_args(int argc, char **argv, struct copy_args *args) {
  if (argc != 3) {
    fprintf(stderr, "usage: %s SRC DST\n", argv[0]);
    return -1;
  }

  args->src = argv[1];
  args->dst = argv[2];

  return 0;
}

int do_copy(struct copy_args *args) {
  fprintf(stderr, "src: `%s`; dst: `%s`\n", args->src, args->dst);
  int src_fd, dst_fd;

  if ((src_fd = open(args->src, O_RDONLY)) < 0) {
    perror("open");
    return 1;
  }
  if ((dst_fd = open(args->dst, O_WRONLY | O_CREAT | O_TRUNC)) < 0) {
    perror("open");
    return 1;
  }

  char buf[BUF_SIZE];
  while (1) {
    int read_bytes = read(src_fd, buf, BUF_SIZE);
    if (read_bytes < 0) {
      perror("read");
      return read_bytes;
    }

    if (!read_bytes) {
      fprintf(stderr, "copy complete\n");
      return 0;
    }

    if (write(dst_fd, buf, read_bytes) < 0) {
      perror("write");
      return -1;
    }
  }

  return 0;
}
