#define _GNU_SOURCE

#include "unity.h"

#include <fcntl.h>
#include <linux/sched.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <linux/wait.h>

static char *TMP_DIR = NULL;

void removeDirectory(char *dir) {
  char cmd[256];
  sprintf(cmd, "rm -r %s", dir);
  TEST_ASSERT_EQUAL(0, system(cmd));
}

void setUp(void) {
  const char tmp_dir[] = "/var/tmp/tmpdir-XXXXXX";

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

void test_cloneMount_mount_doesNotPropagate(void) {
  // PREPARE

  // ACT
  pid_t forkedChildPid;

  if ((forkedChildPid = fork()) == 0) {
    // child process - act but do not assert
    // all assertions will be on the return code
    if (mount(TMP_DIR, TMP_DIR, NULL, MS_BIND | MS_PRIVATE, NULL) != 0) {
      exit(12); // bind mount failed
    }

    char *tmpDirMount = malloc(64);

    int clonedChildPidFd;

    struct clone_args cl_args = {
        .flags = CLONE_PIDFD | CLONE_NEWNS,
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
      if (mount(NULL, TMP_DIR, "tmpfs", 0, NULL) != 0) {
        exit(1); // mount failed
      }

      if (mount(NULL, TMP_DIR, NULL, MS_PRIVATE, NULL) != 0) {
        exit(10); // mount permission change failed
      }

      int dirfd;
      if ((dirfd = open(TMP_DIR, O_DIRECTORY)) < 0) {
        exit(1); // dir open failed
      }

      int filefd;
      if ((filefd = openat(dirfd, "touched", O_WRONLY | O_CREAT, 0700)) < 0) {
        exit(1); // file open failed
      }

      if (close(filefd) != 0 || close(dirfd) != 0) {
        exit(1); // close failed
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

    int dirfd;
    if ((dirfd = open(TMP_DIR, O_DIRECTORY)) < 0) {
      exit(1); // dir open failed
    }

    if (faccessat(dirfd, "touched", F_OK, 0) == 0) {
      exit(9); // file in foreign namespace mount exists
    }

    if (umount(TMP_DIR) != 0) {
      exit(11); // unmount failed
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
