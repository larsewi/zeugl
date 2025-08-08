#include "config.h"

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "zeugl.h"

#define N_ARGS 3
#define DEV_RANDOM "/dev/random"
#define GIBIBYTE(x) (x * 1024 * 1024 * 1024)

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

  int dst = zopen(thread_data->filename, Z_CREATE, (mode_t)0644);
  if (dst == -1) {
    fprintf(stderr, "[thread %d] Failed to open source file '%s': %s\n",
            thread_data->id, thread_data->filename, strerror(errno));
    return NULL;
  }
  printf("[thread %d] Atomically opened source file '%s' (fd = %d)\n",
         thread_data->id, thread_data->filename, dst);

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
            "[thread main] Bad argument count expected at least %d arguments, "
            "got %d\n",
            N_ARGS, argc);
    return EXIT_FAILURE;
  }

  const int num_threads = atoi(argv[1]);
  if (num_threads <= 0) {
    fprintf(
        stderr,
        "[thread main] Bad argument: Expected number of threads, got '%s'\n",
        argv[1]);
    return EXIT_FAILURE;
  }

  pthread_t threads[num_threads];
  struct thread_data thread_data[num_threads];

  for (int i = 0; i < num_threads; i++) {
    thread_data[i].id = i;
    thread_data[i].filename = argv[2];
    thread_data[i].success = false;

    int ret = pthread_create(&threads[i], NULL, start_routine, &thread_data[i]);
    if (ret != 0) {
      fprintf(stderr, "[thread main] Failed to create thread: %s\n",
              strerror(ret));
      return EXIT_FAILURE;
    }
    printf("[thread main] Created thread %d\n", i);
  }

  for (int i = 0; i < num_threads; i++) {
    void *retval;
    int ret = pthread_join(threads[i], &retval);
    if (ret != 0) {
      fprintf(stderr, "[thread main] Failed to join thread %d: %s\n", i,
              strerror(ret));
      return EXIT_FAILURE;
    }
    printf("[thread main] Joined thread %d\n", i);

    if (!thread_data[i].success) {
      fprintf(stderr, "[thread main] Thread %d returned failure\n", i);
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
