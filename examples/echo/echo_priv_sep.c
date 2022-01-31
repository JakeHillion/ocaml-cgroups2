// For privilege separation
#include <clone3.h>

#include <linux/wait.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// This program
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>

#define BUF_SIZE 1024

int real_main(int, char **, int);
int request_handler(int);

int main(int argc, char **argv) {
  pid_t child;

  int num_pids = 2;
  int pid_fds[num_pids];

  // Create IPC
  int pipe_fds[2];
  if (pipe(pipe_fds) < 0) {
    perror("pipe");
    exit(-1);
  }

  // Spawn real_main
  // Use CLONE_FILES to have the opened fds passed up
  struct clone_args real_main_cl_args = {
      .flags = CLONE_NEWIPC | CLONE_NEWNS | CLONE_NEWPID | CLONE_NEWUSER |
               CLONE_NEWUTS | CLONE_PIDFD | CLONE_FILES,
      .pidfd = (uint64_t)&pid_fds[0],
      .child_tid = (uint64_t)NULL,
      .parent_tid = (uint64_t)NULL,
      .exit_signal = SIGCHLD,
      .stack = (uint64_t)NULL,
      .stack_size = 0,
      .tls = (uint64_t)NULL,
  };

  if ((child = clone3(&real_main_cl_args)) == 0) {
    int code = real_main(argc, argv, pipe_fds[1]);
    exit(code);
  } else if (child < 0) {
    perror("clone3");
    exit(-1);
  }

  // Spawn internal cloner (to maintain capabilities)
  // Use CLONE_FILES to pass the opened fds down
  struct clone_args process_generator_cl_args = {
      .flags = CLONE_FILES,
      .pidfd = (uint64_t)&pid_fds[1],
      .child_tid = (uint64_t)NULL,
      .parent_tid = (uint64_t)NULL,
      .exit_signal = SIGCHLD,
      .stack = (uint64_t)NULL,
      .stack_size = 0,
      .tls = (uint64_t)NULL,
  };

  if ((child = clone3(&process_generator_cl_args)) == 0) {
    int read_pipe = pipe_fds[0];

    int client_fd;
    while (1) {
      int bytes_read = read(read_pipe, &client_fd, sizeof(client_fd));
      if (bytes_read == 0) {
        break;
      } else if (bytes_read < 0) {
        perror("read");
        exit(-1);
      }

      fprintf(stderr, "got client_fd: %d\n", client_fd);

      // Do not use CLONE_FILES for default CoW behaviour
      struct clone_args request_cl_args = {
          .flags = CLONE_NEWIPC | CLONE_NEWNS | CLONE_NEWPID | CLONE_NEWUSER |
                   CLONE_NEWUTS | CLONE_PIDFD,
          .pidfd = (uint64_t)&pid_fds[0],
          .child_tid = (uint64_t)NULL,
          .parent_tid = (uint64_t)NULL,
          .exit_signal = SIGCHLD,
          .stack = (uint64_t)NULL,
          .stack_size = 0,
          .tls = (uint64_t)NULL,
      };

      if ((child = clone3(&request_cl_args)) == 0) {
        int code = request_handler(client_fd);
        exit(code);
      } else if (child < 0) {
        perror("clone3");
        exit(-1);
      }
    }

    exit(0);
  } else if (child < 0) {
    perror("clone3");
    exit(-1);
  }

  // Wait for child processes
  siginfo_t status;

  for (int i = 0; i < num_pids; i++) {
    if (waitid(P_PIDFD, pid_fds[i], &status, WEXITED) == -1) {
      perror("waitid");
      return -1;
    }
    if (status.si_status != 0) {
      return status.si_status;
    }
  }

  return 0;
}

int real_main(int argc, char **argv, int write_pipe) {
  if (argc != 2) {
    fprintf(stderr, "expected 1 argument\n");
    return -1;
  }

  int port = atoi(argv[1]);

  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    perror("socket");
  }

  int opt_val = 1;
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof opt_val);

  struct sockaddr_in server;

  server.sin_family = AF_INET;
  server.sin_port = htons(port);
  server.sin_addr.s_addr = htonl(INADDR_ANY);

  if (bind(server_fd, (struct sockaddr *)&server, sizeof(server)) < 0) {
    perror("bind");
  }

  if (listen(server_fd, 16) < 0) {
    perror("listen");
  }

  while (1) {
    struct sockaddr_in client;
    socklen_t client_len = sizeof(client);

    int client_fd = accept(server_fd, (struct sockaddr *)&client, &client_len);
    if (client_fd < 0) {
      perror("accept");
    }

    write(write_pipe, &client_fd, sizeof(client_fd));
    fprintf(stderr, "sent client_fd: %d\n", client_fd);
  }

  return 0;
}

int request_handler(int client_fd) {
  char buf[BUF_SIZE];
  while (1) {
    int read = recv(client_fd, buf, BUF_SIZE, 0);
    if (read < 0) {
      perror("recv");
      return read;
    }

    if (!read) {
      fprintf(stderr, "connection terminated\n");
      return 0;
    }

    if (send(client_fd, buf, read, 0) < 0) {
      perror("send");
      return -1;
    }
  }

  return 0;
}
