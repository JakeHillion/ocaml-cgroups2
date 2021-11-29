#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/uio.h>
#include <unistd.h>

#include <liburing.h>
#include <linux/fs.h>

#define QUEUE_DEPTH 16
#define IO_BLOCK_SIZE 4096
#define BUFFER_NUM_BLOCKS 8

/*
 * Data struct for SQEs and CQEs.
 */
struct io_data {
  bool read;
  off_t offset;
  int len;
};

/*
 * Get size of regular file or error otherwise.
 */
off_t get_file_size(int fd) {
  struct stat st;

  if (fstat(fd, &st) < 0) {
    perror("fstat");
    return -1;
  }

  if (S_ISREG(st.st_mode)) {
    return st.st_size;
  } else {
    return -1;
  }
}

/*
 * Submit a linked pair of read/write requests
 */
int prep_read_write_pair(struct io_uring *ring, int src_fd, int dst_fd,
                         off_t offset, void *buf, int len) {
  struct io_uring_sqe *sqe;
  struct io_data *data;

  sqe = io_uring_get_sqe(ring);
  if (!sqe)
    return 1;

  io_uring_prep_read(sqe, src_fd, buf, len, offset);

  data = malloc(sizeof(*data));
  data->read = 1;
  data->len = len;
  data->offset = offset;

  sqe->flags |= IOSQE_IO_LINK; // add as a dependency of the write
  io_uring_sqe_set_data(sqe, data);

  sqe = io_uring_get_sqe(ring);
  if (!sqe) {
    io_uring_submit_and_wait(ring, 1);
    sqe = io_uring_get_sqe(ring);
    if (!sqe)
      return 2;
  }
  io_uring_prep_write(sqe, dst_fd, buf, len, offset);

  data = malloc(sizeof(*data));
  data->read = 0;
  data->len = len;
  data->offset = offset;
  io_uring_sqe_set_data(sqe, data);

  return 0;
}

/*
 * Await and handle completion entries.
 */
int await_cqes(struct io_uring *ring, unsigned int wait_nr) {
  struct io_uring_cqe *cqe;
  struct io_data *data;

  io_uring_submit(ring);

  for (int i = 0; i < wait_nr; i++) {
    int ret;
    if ((ret = io_uring_wait_cqe(ring, &cqe))) {
      fprintf(stderr, "io_uring_wait_cqe: %d\n", ret);
      return 1;
    }
  }

  return 0;
}

int main(int argc, char **argv) {
  // argument validation
  if (argc < 3) {
    fprintf(stderr, "usage: %s <src> <dst>\n", argv[0]);
    return 1;
  }

  // load files
  int src_fd, dst_fd;
  off_t src_len;

  if ((src_fd = open(argv[1], O_RDONLY)) == -1) {
    perror("open");
    return 1;
  }
  if ((dst_fd = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC)) == -1) {
    perror("open");
    return 1;
  }
  if ((src_len = get_file_size(src_fd)) == -1) {
    return 1;
  }

  // prepare uring
  struct io_uring ring;
  io_uring_queue_init(QUEUE_DEPTH, &ring, 0);

  // perform copy
  void *buf;
  if ((buf = mmap(0, IO_BLOCK_SIZE * BUFFER_NUM_BLOCKS, PROT_READ | PROT_WRITE,
                  MAP_SHARED | MAP_ANONYMOUS, -1, 0)) == MAP_FAILED) {
    perror("mmap");
    return 1;
  }

  off_t blocks =
      (src_len / IO_BLOCK_SIZE) + ((src_len % IO_BLOCK_SIZE) > 0 ? 1 : 0);

  for (off_t i = 0; i < blocks; i++) {
    int section = i % BUFFER_NUM_BLOCKS;
    void *base = buf + (IO_BLOCK_SIZE * section);

    off_t len;
    if (i == blocks - 1) {
      len = src_len % IO_BLOCK_SIZE;
    } else {
      len = IO_BLOCK_SIZE;
    }

    prep_read_write_pair(&ring, src_fd, dst_fd, i * IO_BLOCK_SIZE, base, len);
    if (await_cqes(&ring, 2)) {
      return 1;
    }
  }

  return 0;
}
