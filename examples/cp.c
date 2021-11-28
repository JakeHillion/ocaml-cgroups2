#include <fcntl.h>
#include <linux/fs.h>
#include <linux/io_uring.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/uio.h>
#include <unistd.h>

#define QUEUE_DEPTH 16
#define IO_BLOCK_SIZE 4096

/*
 * io_uring structures
 */
struct submission_ring {
  unsigned *head;
  unsigned *tail;
  unsigned *ring_mask;
  unsigned *ring_entries;
  unsigned *flags;
  unsigned *array;
};

struct completion_ring {
  unsigned *head;
  unsigned *tail;
  unsigned *ring_mask;
  unsigned *ring_entries;
  struct io_uring_cqe *cqes;
};

struct ring {
  int fd;
  struct submission_ring sq_ring;
  struct io_uring_sqe *sqes;
  struct completion_ring cq_ring;
};

/*
 * File info struct
 */
struct file {
  int fd;
  off_t size;
  struct iovec iovecs[];
};

/*
 * Generate syscall wrappers that are still not in stdlib
 */
int io_uring_setup(unsigned entries, struct io_uring_params *p) {
  return (int)syscall(__NR_io_uring_setup, entries, p);
}

int io_uring_enter(int ring_fd, unsigned int to_submit,
                   unsigned int min_complete, unsigned int flags) {
  return (int)syscall(__NR_io_uring_enter, ring_fd, to_submit, min_complete,
                      flags, NULL, 0);
}

/*
 * Setup a ring.
 */
int init_uring(struct ring *r) {
  struct submission_ring *sring = &r->sq_ring;
  struct completion_ring *cring = &r->cq_ring;

  struct io_uring_params p;
  void *sq_ptr, *cq_ptr;

  memset(&p, 0, sizeof(p));
  r->fd = io_uring_setup(QUEUE_DEPTH, &p);
  if (r->fd < 0) {
    perror("io_uring_setup");
    return 1;
  }

  int submission_ring_size = p.sq_off.array + p.sq_entries * sizeof(void *);
  int completion_ring_size =
      p.cq_off.cqes + p.cq_entries * sizeof(struct io_uring_cqe);

  if (p.features & IORING_FEAT_SINGLE_MMAP) {
    if (completion_ring_size > submission_ring_size) {
      submission_ring_size = completion_ring_size;
    }
    completion_ring_size = submission_ring_size;
  } else {
    fprintf(
        stderr,
        "requires unified queue mapping, needs kernel version 5.4 or above\n");
    return 1;
  }

  sq_ptr = mmap(0, submission_ring_size, PROT_READ | PROT_WRITE,
                MAP_SHARED | MAP_POPULATE, r->fd, IORING_OFF_SQ_RING);
  if (sq_ptr == MAP_FAILED) {
    perror("mmap");
    return 1;
  }
  cq_ptr = sq_ptr;

  sring->head = sq_ptr + p.sq_off.head;
  sring->tail = sq_ptr + p.sq_off.tail;
  sring->ring_mask = sq_ptr + p.sq_off.ring_mask;
  sring->ring_entries = sq_ptr + p.sq_off.ring_entries;
  sring->flags = sq_ptr + p.sq_off.flags;
  sring->array = sq_ptr + p.sq_off.array;

  cring->head = cq_ptr + p.cq_off.head;
  cring->tail = cq_ptr + p.cq_off.tail;
  cring->ring_mask = cq_ptr + p.cq_off.ring_mask;
  cring->ring_entries = cq_ptr + p.cq_off.ring_entries;
  cring->cqes = cq_ptr + p.cq_off.cqes;

  r->sqes = mmap(0, p.sq_entries * sizeof(struct io_uring_sqe),
                 PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE, r->fd,
                 IORING_OFF_SQES);
  if (r->sqes == MAP_FAILED) {
    perror("mmap");
    return 1;
  }

  return 0;
}

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
 * Open file and prepare a file struct.
 */
int prep_file(struct file **f, char *path, int flags) {
  int fd = open(path, flags);
  if (fd == -1) {
    perror("open");
    return -1;
  }

  off_t size = get_file_size(fd);
  if (size == -1) {
    return -1;
  }

  off_t blocks = size / IO_BLOCK_SIZE;
  if (size % IO_BLOCK_SIZE)
    blocks++;

  *f = malloc(sizeof(struct file) + sizeof(struct iovec) * blocks);
  if (!*f) {
    fprintf(stderr, "malloc failed\n");
    return -1;
  }

  return 0;
}

int main(int argc, char **argv) {
  if (argc < 3) {
    fprintf(stderr, "usage: %s <src> <dst>\n", argv[0]);
    return 1;
  }

  struct ring r;
  memset(&r, 0, sizeof(r));

  if (init_uring(&r)) {
    fprintf(stderr, "uring setup failed\n");
    return 1;
  }

  struct file *src, *dst;

  if (prep_file(&src, argv[1], O_RDONLY) == -1) {
    return 1;
  }
  if (prep_file(&dst, argv[2], O_WRONLY | O_CREAT | O_TRUNC) == -1) {
    return 1;
  }

  return 0;
}
