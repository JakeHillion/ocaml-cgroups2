#define _GNU_SOURCE

#include "unity.h"

#include <fcntl.h>
#include <linux/sched.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <linux/wait.h>

#define STACK_SIZE (1024 * 1024)

static char *TMP_DIR = NULL;

void removeDirectory(char *dir) {
  char cmd[256];
  sprintf(cmd, "rm -r %s", dir);
  TEST_ASSERT_EQUAL(0, system(cmd));
}

void setUp(void) {
  const char tmp_dir[] = "/tmp/tmpdir-XXXXXX";

  TMP_DIR = malloc(sizeof(tmp_dir));
  strcpy(TMP_DIR, tmp_dir);

  TEST_ASSERT_NOT_EQUAL_MESSAGE(NULL, mkdtemp(TMP_DIR), "tmpdir failed");
}

void tearDown(void) {
  removeDirectory(TMP_DIR);
  free(TMP_DIR);
  TMP_DIR = NULL;
}

long clone3(struct clone_args *cl_args) {
  return syscall(SYS_clone3, cl_args, sizeof(struct clone_args));
}

void test_cloneNoFiles_writeToExistingFd_succeeds(void) {
  // PREPARE

  // ACT
  pid_t forkedChildPid;

  if ((forkedChildPid = fork()) == 0) {
    // child process - act but do not assert
    // all assertions will be on the return code

    // prepare fd
    int dirfd = open(TMP_DIR, O_DIRECTORY);
    if (dirfd < 0) {
      exit(1); // open dir failed
    }
    int filefd = openat(dirfd, "write", O_WRONLY | O_CREAT, 0700);
    if (filefd < 0) {
      exit(1); // open file failed
    }
    if (close(dirfd) != 0) {
      exit(7);
    }

    // clone
    int clonedChildPidFd;

    struct clone_args cl_args = {
        .flags = CLONE_PIDFD,
        .pidfd = (uint64_t)(&clonedChildPidFd),
        .child_tid = 0,
        .parent_tid = 0,
        .exit_signal = SIGCHLD,
        .stack = 0,
        .stack_size = 0,
        .tls = 0,
        .set_tid = 0,
        .set_tid_size = 0,
        .cgroup = 0,
    };

    long cloneResult = clone3(&cl_args);
    if (cloneResult == 0) {
      char buf[] = "hello world\n";
      if (write(filefd, buf, 13) == -1) {
        exit(9);
      }

      exit(0);
    } else if (cloneResult == -1) {
      exit(2); // clone failed
    }

    siginfo_t status;
    if (waitid(P_PIDFD, clonedChildPidFd, &status, WEXITED) == -1) {
      exit(3); // wait failed
    }

    if (status.si_status != 0) {
      exit(status.si_status); // return status
    }

    exit(0);
  }

  // ASSERT
  TEST_ASSERT_GREATER_THAN_MESSAGE(0, forkedChildPid, "fork failed");

  int status = 0;
  TEST_ASSERT_EQUAL_MESSAGE(forkedChildPid, waitpid(forkedChildPid, &status, 0),
                            "wait failed");
  TEST_ASSERT_EQUAL_MESSAGE(0, WEXITSTATUS(status), "return status non-zero");
}

void test_cloneFiles_writeToExistingFd_succeeds(void) {
  // PREPARE

  // ACT
  pid_t forkedChildPid;

  if ((forkedChildPid = fork()) == 0) {
    // child process - act but do not assert
    // all assertions will be on the return code

    // prepare fd
    int dirfd = open(TMP_DIR, O_DIRECTORY);
    if (dirfd < 0) {
      exit(1); // open dir failed
    }
    int filefd = openat(dirfd, "write", O_WRONLY | O_CREAT, 0700);
    if (filefd < 0) {
      exit(1); // open file failed
    }
    if (close(dirfd) != 0) {
      exit(7);
    }

    // clone
    int clonedChildPidFd;

    struct clone_args cl_args = {
        .flags = CLONE_PIDFD | CLONE_FILES,
        .pidfd = (uint64_t)(&clonedChildPidFd),
        .child_tid = 0,
        .parent_tid = 0,
        .exit_signal = SIGCHLD,
        .stack = 0,
        .stack_size = 0,
        .tls = 0,
        .set_tid = 0,
        .set_tid_size = 0,
        .cgroup = 0,
    };

    long cloneResult = clone3(&cl_args);
    if (cloneResult == 0) {
      char buf[] = "hello world\n";
      if (write(filefd, buf, 13) == -1) {
        exit(9);
      }

      exit(0);
    } else if (cloneResult == -1) {
      exit(2); // clone failed
    }

    siginfo_t status;
    if (waitid(P_PIDFD, clonedChildPidFd, &status, WEXITED) == -1) {
      exit(3); // wait failed
    }

    if (status.si_status != 0) {
      exit(status.si_status); // return status
    }

    exit(0);
  }

  // ASSERT
  TEST_ASSERT_GREATER_THAN_MESSAGE(0, forkedChildPid, "fork failed");

  int status = 0;
  TEST_ASSERT_EQUAL_MESSAGE(forkedChildPid, waitpid(forkedChildPid, &status, 0),
                            "wait failed");
  TEST_ASSERT_EQUAL_MESSAGE(0, WEXITSTATUS(status), "return status non-zero");
}

void test_cloneNoFiles_writeToNewFd_fails(void) { TEST_IGNORE_MESSAGE("TODO"); }

void test_cloneFiles_writeToNewFd_succeeds(void) {
  TEST_IGNORE_MESSAGE("TODO");
}
