// For privilege separation
#include <clone3.h>

#include <linux/wait.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

// This program
#include <stdint.h>
#include <stdio.h>

uint64_t fib(uint64_t i) {
  uint64_t a = 0;
  uint64_t b = 1;

  for (; i > 0; i--) {
    uint64_t old_b = b;
    b = b + a;
    a = old_b;
  }

  return a;
}

int real_main(int argc, char **argv);

int main(int argc, char **argv) {
  int pid_fd;

  struct clone_args cl_args = {
      .flags = CLONE_NEWIPC | CLONE_NEWNET | CLONE_NEWNS | CLONE_NEWPID |
               CLONE_NEWUSER | CLONE_NEWUTS | CLONE_PIDFD,
      .pidfd = (uint64_t)&pid_fd,
      .child_tid = (uint64_t)NULL,
      .parent_tid = (uint64_t)NULL,
      .exit_signal = SIGCHLD,
      .stack = (uint64_t)NULL,
      .stack_size = 0,
      .tls = (uint64_t)NULL,
  };

  pid_t child = clone3(&cl_args);
  if (child < 0) {
    perror("clone3");
    exit(-1);
  } else if (child == 0) {
    int code = real_main(argc, argv);
    exit(code);
  } else {
    siginfo_t status;
    if (waitid(P_PIDFD, pid_fd, &status, WEXITED) == -1) {
      perror("waitid");
      return -1;
    }

    exit(status.si_status);
  }
}

int real_main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "expected 1 argument\n");
    return -1;
  }

  uint64_t i = 0;
  if (sscanf(argv[1], "%lu", &i) != 1) {
    fprintf(stderr, "sscanf failed\n");
    return -1;
  }

  uint64_t fib_result = fib(i);
  printf("fib(%lu) = %lu\n", i, fib_result);
  return 0;
}
