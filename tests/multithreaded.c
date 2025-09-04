#include "config.h"

#include <asm-generic/errno-base.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <unistd.h>

#include "zeugl.h"

#define DEV_RANDOM "/dev/random"
#define MIN(a, b) ((a <= b) ? a : b)
#define LOG_DEBUG(tid, ...) _log_message(tid, __FILE__, __LINE__, __VA_ARGS__)

struct thread_data {
  int id;
  int num_bytes;
  const char *filename;
  bool success;
};

static const char *FILENAME;
static size_t NUM_BYTES;

static void _log_message(int thread_id, const char *file, int line,
                         const char *format, ...) {
  char msg[1024];
  va_list ap;

  va_start(ap, format);

#ifdef NDEBUG
  __attribute__((unused))
#endif
  int ret = vsnprintf(msg, sizeof(msg), format, ap);
  assert(ret >= 0 && (size_t)ret < sizeof(msg));

  va_end(ap);

  ret = printf("[Thread %d][%s:%d] Debug: %s\n", thread_id, file, line, msg);
  assert(ret >= 0);
}

static off_t get_file_size(int fd) {
  /* Check current offset */
  off_t cur_offset = lseek(fd, 0, SEEK_CUR);
  if (cur_offset < 0) {
    return cur_offset;
  }

  /* Check end-of-file offset */
  off_t end_offset = lseek(fd, 0, SEEK_END);
  if (end_offset < 0) {
    return end_offset;
  }

  /* Seek back to original offset */
  if (lseek(fd, cur_offset, SEEK_SET) < 0) {
    return cur_offset;
  }

  return end_offset;
}

static bool file_rand_fill(int dst, size_t num_bytes) {
  int src = open(DEV_RANDOM, O_RDONLY);
  if (src == -1) {
    return false;
  }

  size_t tot_written = 0;
  char buffer[BUFSIZ];
  do {
    size_t n_read = 0;
    size_t tot_remaining = num_bytes - tot_written;
    do {
      ssize_t ret =
          read(src, buffer + n_read, MIN(BUFSIZ - n_read, tot_remaining));
      if (ret < 0) {
        if (errno == EINTR) {
          continue;
        }
        close(src);
        return false;
      }

      n_read += (size_t)ret;
    } while (n_read < BUFSIZ && n_read < num_bytes);

    size_t n_written = 0;
    do {
      ssize_t ret = write(dst, buffer + n_written, n_read - n_written);
      if (ret < 0) {
        if (errno == EINTR) {
          continue;
        }
        close(src);
        return false;
      }

      n_written += (size_t)ret;
    } while (n_written < n_read);

    tot_written += n_written;
  } while (tot_written < num_bytes);

  close(src);
  return true;
}

void *start_routine(void *arg) {
  struct thread_data *thread_data = (struct thread_data *)arg;

  int fd = zopen(FILENAME, 0);
  if (fd == -1) {
    LOG_DEBUG(thread_data->id, "Failed to open file '%s': %s", FILENAME,
              strerror(errno));
    return NULL;
  }
  LOG_DEBUG(thread_data->id, "Opened file '%s'", FILENAME);

  off_t offset = get_file_size(fd);
  if (offset < 0) {
    LOG_DEBUG(thread_data->id, "Failed to get size of file '%s': %s", FILENAME,
              strerror(errno));
    return NULL;
  }
  if ((size_t)offset != NUM_BYTES) {
    LOG_DEBUG(thread_data->id,
              "Race condition detected based on file size for file '%s': "
              "Expected %d, got %" PRId64,
              FILENAME, NUM_BYTES, offset);
    return NULL;
  }

  if (!file_rand_fill(fd, NUM_BYTES)) {
    LOG_DEBUG(thread_data->id,
              "Failed to fill file '%s' with %zu bytes of random data: %s",
              FILENAME, NUM_BYTES, strerror(errno));
    return NULL;
  }
  LOG_DEBUG(thread_data->id, "Filled file '%s' with %zu bytes of random data",
            FILENAME, NUM_BYTES);

  if (zclose(fd, true) != 0) {
    LOG_DEBUG(thread_data->id, "Failed to close file '%s': %s", FILENAME,
              strerror(errno));
    return NULL;
  }
  LOG_DEBUG(thread_data->id, "Closed file '%s'", FILENAME);

  /* You should be able to read the file without getting interrupted by other
   * threads/processes */
  fd = open(FILENAME, O_RDONLY);
  if (fd < 0) {
    LOG_DEBUG(thread_data->id, "Failed to open file '%s': %s", FILENAME,
              strerror(errno));
    return NULL;
  }

  offset = get_file_size(fd);
  if (offset < 0) {
    LOG_DEBUG(thread_data->id, "Failed to get size of file '%s': %s", FILENAME,
              strerror(errno));
    return NULL;
  } else if ((size_t)offset != NUM_BYTES) {
    LOG_DEBUG(thread_data->id,
              "Race condition detected based on file size for file '%s': "
              "Expected %d, got %" PRId64,
              FILENAME, NUM_BYTES, offset);
    return NULL;
  }

  close(fd);

  thread_data->success = true;
  return NULL;
}

int main(int argc, char *argv[]) {
  if (argc < 1) {
    LOG_DEBUG(0, "Missing required argument NUM_THREADS");
    return EXIT_FAILURE;
  }

  const int num_threads = atoi(argv[1]);
  if (num_threads <= 0) {
    LOG_DEBUG(0, "Bad argument: Expected number of threads, got '%s'", argv[1]);
    return EXIT_FAILURE;
  }

  if (argc < 2) {
    LOG_DEBUG(0, "Missing required argument NUM_BYTES");
    return EXIT_FAILURE;
  }

  int ret = atoi(argv[2]);
  if (ret <= 0) {
    LOG_DEBUG(0, "Bad argument: Expected number of bytes, got '%s'", argv[2]);
    return EXIT_FAILURE;
  }
  NUM_BYTES = (size_t)ret;

  if (argc < 3) {
    LOG_DEBUG(0, "Missing required argument FILENAME");
    return EXIT_FAILURE;
  }

  FILENAME = argv[3];

  int fd = open(FILENAME, O_WRONLY | O_CREAT | O_TRUNC, (mode_t)0644);
  if (fd < 0) {
    LOG_DEBUG(0, "Failed to open file '%s': %s", FILENAME, strerror(errno));
    return EXIT_FAILURE;
  }

  if (!file_rand_fill(fd, NUM_BYTES)) {
    LOG_DEBUG(0, "Failed to fill file '%s' with %zu bytes of random data: %s",
              FILENAME, NUM_BYTES, strerror(errno));
    return EXIT_FAILURE;
  }
  LOG_DEBUG(0, "Filled file '%s' with %zu bytes of random data", FILENAME,
            NUM_BYTES);
  close(fd);

  pthread_t threads[num_threads];
  struct thread_data thread_data[num_threads];

  for (int i = 0; i < num_threads; i++) {
    thread_data[i].id = i + 1;
    thread_data[i].success = false;

    int ret = pthread_create(&threads[i], NULL, start_routine, &thread_data[i]);
    if (ret != 0) {
      LOG_DEBUG(0, "Failed to create thread: %s", strerror(ret));
      return EXIT_FAILURE;
    }
    LOG_DEBUG(0, "Created thread %d", thread_data[i].id);
  }

  for (int i = 0; i < num_threads; i++) {
    void *retval;
    int ret = pthread_join(threads[i], &retval);
    if (ret != 0) {
      LOG_DEBUG(0, "Failed to join thread %d: %s", thread_data[i].id,
                strerror(ret));
      return EXIT_FAILURE;
    }
    LOG_DEBUG(0, "Joined thread %d", thread_data[i].id);

    if (!thread_data[i].success) {
      LOG_DEBUG(0, "Thread %d returned failure", thread_data[i].id);
      return EXIT_FAILURE;
    }
  }

  fd = open(FILENAME, O_RDONLY);
  if (fd < 0) {
    LOG_DEBUG(0, "Failed to open file '%s': %s", FILENAME, strerror(errno));
    return EXIT_FAILURE;
  }

  off_t offset = get_file_size(fd);
  if (offset < 0) {
    LOG_DEBUG(0, "Failed to get size of file '%s': %s", FILENAME,
              strerror(errno));
    return EXIT_FAILURE;
  }
  if ((size_t)offset != NUM_BYTES) {
    LOG_DEBUG(0,
              "Race condition detected based on file size for file '%s': "
              "Expected %zu, got %" PRId64,
              FILENAME, NUM_BYTES, offset);
    return EXIT_FAILURE;
  }

  close(fd);
  return EXIT_SUCCESS;
}
