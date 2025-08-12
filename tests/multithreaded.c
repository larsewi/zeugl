#include "config.h"

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <unistd.h>

#include "zeugl.h"

#define N_ARGS 3
#define DEV_RANDOM "/dev/random"
#define FILESIZE (10 * 1024 * 1024)
#define MIN(a, b) ((a <= b) ? a : b)

struct thread_data {
  int id;
  const char *filename;
  bool success;
};

void *start_routine(void *arg) {
  struct thread_data *thread_data = (struct thread_data *)arg;

  int src = open(DEV_RANDOM, O_RDONLY);
  if (src == -1) {
    fprintf(stderr, "[thread %d] Failed to open source file '%s': %s\n",
            thread_data->id, DEV_RANDOM, strerror(errno));
    return NULL;
  }
  printf("[thread %d] Opened source file '%s' (fd = %d) in read-only mode\n",
         thread_data->id, DEV_RANDOM, src);

  int dst = zopen(thread_data->filename, Z_CREATE | Z_APPEND, (mode_t)0644);
  if (dst == -1) {
    fprintf(stderr, "[thread %d] Failed to open source file '%s': %s\n",
            thread_data->id, thread_data->filename, strerror(errno));
    return NULL;
  }
  printf("[thread %d] Atomically opened source file '%s' (fd = %d)\n",
         thread_data->id, thread_data->filename, dst);

  off_t offset = lseek(dst, 0, SEEK_CUR);
  if (offset < 0) {
    fprintf(stderr,
            "[thread %d] Failed to get file offset from destination file '%s' "
            "(fd = %d) before copy: %s\n",
            thread_data->id, thread_data->filename, dst, strerror(errno));
    return NULL;
  }

  if (offset != 0 && offset != FILESIZE) {
    fprintf(stderr,
            "[thread %d] Bad file offset %" PRId64
            " from destination file '%s' (fd = %d) before copy: %s\n",
            thread_data->id, offset, thread_data->filename, dst,
            strerror(errno));
    return NULL;
  }
  printf("[thread %d] Good file offset %" PRId64
         " on zopened destination file '%s' (fd = %d) before copy\n",
         thread_data->id, offset, thread_data->filename, dst);

  offset = lseek(dst, 0, SEEK_SET);
  if (offset < 0) {
    fprintf(stderr,
            "[thread %d] Failed to seek to start of destination file '%s' (fd "
            "= %d) before copy: %s\n",
            thread_data->id, thread_data->filename, dst, strerror(errno));
    return NULL;
  }

  size_t tot_written = 0;
  char buffer[BUFSIZ];
  do {
    size_t n_read = 0;
    size_t tot_remaining = FILESIZE - tot_written;
    do {
      ssize_t ret =
          read(src, buffer + n_read, MIN(BUFSIZ - n_read, tot_remaining));
      if (ret < 0) {
        if (errno == EINTR) {
          continue;
        }
        fprintf(
            stderr,
            "[thread %d] Failed to read from source file '%s' (fd = %d): %s\n",
            thread_data->id, DEV_RANDOM, src, strerror(errno));
        return NULL;
      }

      n_read += (size_t)ret;
    } while (n_read < BUFSIZ);
    printf("[thread %d] Read %zu bytes from source file '%s' (fd = %d)",
           thread_data->id, n_read, DEV_RANDOM, src);

    size_t n_written = 0;
    do {
      ssize_t ret = write(dst, buffer + n_written, n_read - n_written);
      if (ret < 0) {
        if (errno == EINTR) {
          continue;
        }

        fprintf(stderr,
                "[thread %d] Failed to write to destination file '%s' (fd = "
                "%d): %s",
                thread_data->id, thread_data->filename, dst, strerror(errno));
        return NULL;
      }

      n_written += (size_t)ret;
    } while (n_written < n_read);

    tot_written += n_written;
  } while (tot_written < FILESIZE);

  if (zclose(dst, true) != 0) {
    fprintf(stderr,
            "[thread %d] Failed to atomically close destination file '%s' (fd "
            "= %d): %s\n",
            thread_data->id, thread_data->filename, dst, strerror(errno));
    return NULL;
  }
  printf("[thread %d] Closed destination file '%s' (fd = %d)\n",
         thread_data->id, thread_data->filename, dst);

  if (close(src) != 0) {
    fprintf(stderr,
            "[thread %d] Failed to close source file '%s' (fd = %d): %s\n",
            thread_data->id, DEV_RANDOM, src, strerror(errno));
    return NULL;
  }
  printf("[thread %d] Closed source file '%s' (fd = %d)\n", thread_data->id,
         DEV_RANDOM, dst);

  thread_data->success = true;
  return NULL;
}

int main(int argc, char *argv[]) {
  if (argc < N_ARGS) {
    fprintf(stderr,
            "[thread m] Bad argument count expected at least %d arguments, "
            "got %d\n",
            N_ARGS, argc);
    return EXIT_FAILURE;
  }

  const int num_threads = atoi(argv[1]);
  if (num_threads <= 0) {
    fprintf(stderr,
            "[thread m] Bad argument: Expected number of threads, got '%s'\n",
            argv[1]);
    return EXIT_FAILURE;
  }

  const char *fname = argv[2];

  pthread_t threads[num_threads];
  struct thread_data thread_data[num_threads];

  for (int i = 0; i < num_threads; i++) {
    thread_data[i].id = i;
    thread_data[i].filename = fname;
    thread_data[i].success = false;

    int ret = pthread_create(&threads[i], NULL, start_routine, &thread_data[i]);
    if (ret != 0) {
      fprintf(stderr, "[thread m] Failed to create thread: %s\n",
              strerror(ret));
      return EXIT_FAILURE;
    }
    printf("[thread m] Created thread %d\n", i);
  }

  for (int i = 0; i < num_threads; i++) {
    void *retval;
    int ret = pthread_join(threads[i], &retval);
    if (ret != 0) {
      fprintf(stderr, "[thread m] Failed to join thread %d: %s\n", i,
              strerror(ret));
      return EXIT_FAILURE;
    }
    printf("[thread m] Joined thread %d\n", i);

    if (!thread_data[i].success) {
      fprintf(stderr, "[thread m] Thread %d returned failure\n", i);
      return EXIT_FAILURE;
    }
  }

  int fd = open(argv[2], O_RDONLY);
  if (fd < 0) {
    fprintf(stderr, "[thread m] Failed to open file '%s': %s", fname,
            strerror(errno));
    return EXIT_FAILURE;
  }

  off_t offset = lseek(fd, 0, SEEK_END);
  if (offset < 0) {
    fprintf(stderr,
            "[thread m] Failed to get file offset from file '%s' (fd = %d) "
            "before copy: %s\n",
            fname, fd, strerror(errno));
    return EXIT_FAILURE;
  }

  if (offset != FILESIZE) {
    fprintf(stderr,
            "[thread m] Bad file offset %" PRId64
            " from file '%s' (fd = %d): %s\n",
            offset, fname, fd, strerror(errno));
    return EXIT_FAILURE;
  }
  printf("[thread m] Good file offset %" PRId64
         " on destination file '%s' (fd = %d) before copy\n",
         offset, fname, fd);

  close(fd);

  return EXIT_SUCCESS;
}
