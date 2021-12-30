#define _GNU_SOURCE

#include "unity.h"

#include <fcntl.h>
#include <linux/sched.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <linux/wait.h>

#define CHILD_STACK_SIZE (8 * 1024 * 1024)

#define FILE_DEFAULT_NS ("/tmp/file_exists_0148c163-5919-4651")
#define FILE_NEW_NS ("/tmp/file_exists_fd0fd491-9816-442d")

void createFile(const char *fname) {
  int fd = open(fname, O_RDONLY | O_CREAT, 0644);
  TEST_ASSERT_GREATER_THAN_MESSAGE(0, fd, "open failed");
  TEST_ASSERT_EQUAL_MESSAGE(0, close(fd), "close failed");
}

void deleteFileIfExists(const char *fname) {
  if (access(fname, F_OK) == 0) {
    TEST_ASSERT_EQUAL_MESSAGE(0, unlink(fname), "unlink failed");
  }
}

void setUp(void) { createFile(FILE_DEFAULT_NS); }

void tearDown(void) { deleteFileIfExists(FILE_DEFAULT_NS); }

long clone3(struct clone_args *cl_args) {
  return syscall(SYS_clone3, cl_args, sizeof(struct clone_args));
}

void test_cloneFs_findsFileWhenFsCloned_succeeds(void) {
  // PREPARE

  // ACT
  pid_t forkedChildPid;

  if ((forkedChildPid = fork()) == 0) {
    // child process - act but do not assert
    // all assertions will be on the return code
    int clonedChildPidFd;

    struct clone_args cl_args = {
        .flags = CLONE_PIDFD | CLONE_FS,
        .pidfd = (uint64_t)(&clonedChildPidFd),
        .child_tid = 0,
        .parent_tid = 0,
        .exit_signal = 0,
        .stack = 0,
        .stack_size = 0,
        .tls = 0,
        .set_tid = 0,
        .set_tid_size = 0,
        .cgroup = 0,
    };

    long cloneResult = clone3(&cl_args);
    if (cloneResult == 0) {
      printf("child\n");
      sleep(30);

      exit(0);
    } else if (cloneResult == -1) {
      exit(1); // clone failed
    }

    sleep(30);
    siginfo_t status;
    printf("clonedChildPidFd: %d\n", clonedChildPidFd);
    if (waitid(P_PIDFD, clonedChildPidFd, &status, WEXITED) == -1) {
      perror("waitid");
      exit(2); // wait failed
    }

    // MUST exit this process and not return
    // else the test runner will be duplicated

    exit(status.si_status); // propogate return status
  }

  // ASSERT
  TEST_ASSERT_GREATER_THAN_MESSAGE(0, forkedChildPid, "fork failed");

  int status = 0;
  TEST_ASSERT_EQUAL_MESSAGE(forkedChildPid, waitpid(forkedChildPid, &status, 0),
                            "wait failed");
  TEST_ASSERT_EQUAL_MESSAGE(0, WEXITSTATUS(status), "return status non-zero");
}
