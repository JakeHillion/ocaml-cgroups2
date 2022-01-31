// For privilege separation
#include <clone3.h>

#include <linux/wait.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

// This program
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>

#define BUF_SIZE 1024

int real_main(int, char **);
int request_handler(int);

int main(int argc, char **argv) {
  int pid_fd;

  struct clone_args cl_args = {
      .flags = CLONE_NEWIPC | CLONE_NEWNS | CLONE_NEWPID | CLONE_NEWUSER |
               CLONE_NEWUTS | CLONE_PIDFD,
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

    request_handler(client_fd);
  }

  return 0;
}

int request_handler(int client_fd) {
  char buf[BUF_SIZE];
  while (1) {
    int read = recv(client_fd, buf, BUF_SIZE, 0);
    if (read < 0) {
      perror("read");
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
