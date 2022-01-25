#include <clone3.h>
#include <sys/syscall.h>
#include <unistd.h>

long clone3(struct clone_args *cl_args) {
  return syscall(SYS_clone3, cl_args, sizeof(struct clone_args));
}
