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

void setUp(void) {
  static char *initialDir = NULL;
  if (initialDir == NULL) {
    initialDir = get_current_dir_name();
  }

  TEST_ASSERT_NOT_EQUAL_MESSAGE(NULL, initialDir,
                                "get working directory failed");
  TEST_ASSERT_EQUAL_MESSAGE(0, chdir(initialDir),
                            "returning to working directory failed");
}

void tearDown(void) {}

long clone3(struct clone_args *cl_args) {
  return syscall(SYS_clone3, cl_args, sizeof(struct clone_args));
}

void test_unshareNoFsFromFork_chdir_doesNotPropagate(void) {
  // PREPARE
  char tmpDir[] = "/tmp/tmpdir-XXXXXX";
  TEST_ASSERT_NOT_EQUAL_MESSAGE(NULL, mkdtemp(tmpDir), "tmpdir failed");

  // ACT
  pid_t forkedChildPid;

  if ((forkedChildPid = fork()) == 0) {
    // child process - act but do not assert
    // all assertions will be on the return code
    pid_t clonedChildPid;

    if ((clonedChildPid = fork()) == 0) {
      if (unshare(0) == -1) {
        exit(1); // unshare failed
      }

      if (chdir(tmpDir) != 0) {
        exit(2); // chdir failed
      }

      exit(0);
    } else if (clonedChildPid == -1) {
      exit(3); // fork failed
    }

    siginfo_t status;
    if (waitid(P_PID, clonedChildPid, &status, WEXITED) == -1) {
      exit(4); // wait failed
    }

    if (status.si_status != 0) {
      exit(status.si_status); // return status
    }

    char *cwd = get_current_dir_name();
    int directory_moved = strcmp(cwd, tmpDir);
    free(cwd);

    if (directory_moved == 0) {
      exit(5); // chdir did propagate
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

void test_unshareFsFromFork_chdir_doesNotPropagate(void) {
  // PREPARE
  char tmpDir[] = "/tmp/tmpdir-XXXXXX";
  TEST_ASSERT_NOT_EQUAL_MESSAGE(NULL, mkdtemp(tmpDir), "tmpdir failed");

  // ACT
  pid_t forkedChildPid;

  if ((forkedChildPid = fork()) == 0) {
    // child process - act but do not assert
    // all assertions will be on the return code
    pid_t clonedChildPid;

    if ((clonedChildPid = fork()) == 0) {
      if (unshare(CLONE_FS) == -1) {
        exit(1); // unshare failed
      }

      if (chdir(tmpDir) != 0) {
        exit(2); // chdir failed
      }

      exit(0);
    } else if (clonedChildPid == -1) {
      exit(3); // fork failed
    }

    siginfo_t status;
    if (waitid(P_PID, clonedChildPid, &status, WEXITED) == -1) {
      exit(4); // wait failed
    }

    if (status.si_status != 0) {
      exit(status.si_status); // return status
    }

    char *cwd = get_current_dir_name();
    int directory_moved = strcmp(cwd, tmpDir);
    free(cwd);

    if (directory_moved == 0) {
      exit(5); // chdir did propagate
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

void test_unshareNoFsFromClone_chdir_doesPropagate(void) {
  // PREPARE
  char tmpDir[] = "/tmp/tmpdir-XXXXXX";
  TEST_ASSERT_NOT_EQUAL(NULL, mkdtemp(tmpDir));

  char subDirA[21];
  TEST_ASSERT_GREATER_THAN(0, sprintf(subDirA, "%s/a", tmpDir));
  TEST_ASSERT_EQUAL(0, mkdir(subDirA, 0700));

  char subDirB[21];
  TEST_ASSERT_GREATER_THAN(0, sprintf(subDirB, "%s/b", tmpDir));
  TEST_ASSERT_EQUAL(0, mkdir(subDirB, 0700));

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
      if (chdir(subDirA) != 0) {
        exit(1); // chdir failed
      }

      if (unshare(0) == -1) {
        exit(1); // unshare failed
      }

      if (chdir(subDirB) != 0) {
        exit(1); // chdir failed
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

    char *cwd = get_current_dir_name();
    int directory_moved = strcmp(cwd, subDirB);
    free(cwd);

    if (directory_moved != 0) {
      exit(4); // chdir propagated unexpectedly
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

void test_unshareFsFromClone_chdir_doesNotPropagate(void) {
  // PREPARE
  char tmpDir[] = "/tmp/tmpdir-XXXXXX";
  TEST_ASSERT_NOT_EQUAL(NULL, mkdtemp(tmpDir));

  char subDirA[21];
  TEST_ASSERT_GREATER_THAN(0, sprintf(subDirA, "%s/a", tmpDir));
  TEST_ASSERT_EQUAL(0, mkdir(subDirA, 0700));

  char subDirB[21];
  TEST_ASSERT_GREATER_THAN(0, sprintf(subDirB, "%s/b", tmpDir));
  TEST_ASSERT_EQUAL(0, mkdir(subDirB, 0700));

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
      if (chdir(subDirA) != 0) {
        exit(1); // chdir failed
      }

      if (unshare(CLONE_FS) == -1) {
        exit(1); // unshare failed
      }

      if (chdir(subDirB) != 0) {
        exit(1); // chdir failed
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

    char *cwd = get_current_dir_name();
    int directory_moved = strcmp(cwd, subDirA);
    free(cwd);

    if (directory_moved != 0) {
      exit(4); // chdir propagated unexpectedly
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
