#define CAML_NAME_SPACE
#include <caml/mlvalues.h>
#include <caml/memory.h>
#include <caml/callback.h>

#include <unistd.h>
#include <stdio.h>
#include <linux/sched.h>
#include <sys/syscall.h>

#ifndef CLONE_NEWCGROUP /* Added in Linux 4.6 */
#define CLONE_NEWCGROUP 0x02000000
#endif

#ifndef CLONE_CLEAR_SIGHAND /* Added in Linux 5.5 */
#define CLONE_CLEAR_SIGHAND 0x100000000ULL
#endif

#ifndef CLONE_NEWTIME /* Added in Linux 5.6 */
#define CLONE_NEWTIME 0x00000080
#endif

/*
type clone_flag =
  | CLONE_CHILD_CLEARTID
  | CLONE_CHILD_SETTID
  | CLONE_CLEAR_SIGHAND
  | CLONE_FILES
  | CLONE_FS
  | CLONE_IO
  | CLONE_NEWCGROUP
  | CLONE_NEWIPC
  | CLONE_NEWNET
  | CLONE_NEWNS
  | CLONE_NEWPID
  | CLONE_NEWUSER
  | CLONE_NEWUTS
  | CLONE_PARENT
  | CLONE_PARENT_SETTID
  | CLONE_PIDFD
  | CLONE_PTRACE
  | CLONE_SETTLS
  | CLONE_SIGHAND
  | CLONE_SYSVSEM
  | CLONE_THREAD
  | CLONE_UNTRACED
  | CLONE_VFORK
  | CLONE_VM
  | CLONE_NEWTIME
*/

unsigned long long caml_clone_flag(value val)
{
      switch (Long_val(val))
      {
      case 0:
            return CLONE_CHILD_CLEARTID;
      case 1:
            return CLONE_CHILD_SETTID;
      case 2:
            return CLONE_CLEAR_SIGHAND;
      case 3:
            return CLONE_FILES;
      case 4:
            return CLONE_FS;
      case 5:
            return CLONE_IO;
      case 6:
            return CLONE_NEWCGROUP;
      case 7:
            return CLONE_NEWIPC;
      case 8:
            return CLONE_NEWNET;
      case 9:
            return CLONE_NEWNS;
      case 10:
            return CLONE_NEWPID;
      case 11:
            return CLONE_NEWUSER;
      case 12:
            return CLONE_NEWUTS;
      case 13:
            return CLONE_PARENT;
      case 14:
            return CLONE_PARENT_SETTID;
      case 15:
            return CLONE_PIDFD;
      case 16:
            return CLONE_PTRACE;
      case 17:
            return CLONE_SETTLS;
      case 18:
            return CLONE_SIGHAND;
      case 19:
            return CLONE_SYSVSEM;
      case 20:
            return CLONE_THREAD;
      case 21:
            return CLONE_UNTRACED;
      case 22:
            return CLONE_VFORK;
      case 23:
            return CLONE_VM;
      case 24:
            return CLONE_NEWTIME;
      }

      return 0;
}

/*
type clone_args =
  { flags: int  (** Flags for the clone call *)
  ; pidfd: pid_t ref option  (** Pointer to where to store the pidfd *)
  ; child_tid: pid_t ref option
        (** Where to place the child thread ID in the child's memory *)
  ; parent_tid: pid_t ref option
        (** Where to place the child thread ID in the parent's memory *)
  ; exit_signal: int
        (** Signal to deliver to parent on child's termination *)
  ; stack: bytes ref option
        (** Stack for the child if the parent and child share memory *)
  ; tls: int ref option  (** Location of new thread local storage *)
  ; set_tid: pid_t list option
        (** Optional list of specific pids for one or more of the namespaces *)
  }
*/

value caml_clone3(value caml_cl_args, value fn)
{
      CAMLparam2(caml_cl_args, fn);

      // extract arguments
      unsigned long long flags = 0;

      value tail = Field(caml_cl_args, 0);
      while (!Is_none(tail))
      {
            flags |= caml_clone_flag(Field(tail, 0));
            tail = Field(tail, 1);
      }

      int pidfd;           // output
      pid_t child_tid;     // output
      pid_t parent_tid;    // output
      int exit_signal;     // input
      void *stack;         // input
      size_t stack_size;   // input
      void *tls;           // input
      pid_t *set_tid;      // input
      size_t set_tid_size; // input
      int cgroup;          // input

      exit_signal = Int_val(Field(caml_cl_args, 4));

      value stack_val = Field(caml_cl_args, 5);
      if (Is_some(stack_val))
      {
            stack = Bytes_val(Some_val(stack_val));
            stack_size = caml_string_length(Some_val(stack_val));
      }
      else
      {
            stack = NULL;
            stack_size = 0;
      }
      // TODO: set_tid

      struct clone_args cl_args = {
          .flags = (uint64_t)flags,
          .pidfd = (uint64_t)&pidfd,
          .child_tid = (uint64_t)&child_tid,
          .parent_tid = (uint64_t)&parent_tid,
          .exit_signal = (uint64_t)exit_signal,
          .stack = (uint64_t)stack,
          .stack_size = (uint64_t)stack_size,
          .tls = (uint64_t)0,
          // .set_tid = set_tid,
          // .set_tid_size = set_tid_size,
          // .cgroup = cgroup,
      };

      // clone
      pid_t clone_result = syscall(SYS_clone3, &cl_args, sizeof(struct clone_args));
      if (clone_result == 0)
      {
            // run the callback
            // value v = caml_callback(fn, Val_none);
            // if (Is_exception_result(v))
            // {
            //       exit(1);
            // }
            // else
            // {
            //       exit(0);
            // }

            char *newargv[] = {"/bin/sh", NULL};
            char *newenviron[] = {NULL};

            execve(newargv[0], newargv, newenviron);
            perror("execve");
            return 1;
      }
      else if (clone_result < 0)
      {
            // handle error
            perror("clone");
            exit(-1);
      }

      // write back results to caml refs
      if (Is_some(Field(caml_cl_args, 1)))
      {
            Store_field(caml_cl_args, 1, Val_int(pidfd));
      }

      CAMLreturn(Val_int(clone_result));
}
