#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <zeugl.h>

static void signal_handler(__attribute__((unused)) int sig) {
  printf("Old signal handler is called!\n");
  creat("hello-from-chained-signal-handler", (mode_t)0644);
}

int main(int argc, const char *argv[]) {
  printf("Testing cleanup handlers...\n");

  /* Install a signal handler, to make sure new signal handlers are chained */
  struct sigaction sa = {0};
  sa.sa_handler = signal_handler;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGTERM, &sa, NULL);

  /* Create a test file that will be left open */
  int fd = zopen("test_file.txt", Z_CREATE | Z_TRUNCATE, (mode_t)0644);
  if (fd < 0) {
    perror("zopen failed");
    return 1;
  }

  /* Write some data */
  const char *data = "This is test data\n";
  write(fd, data, strlen(data));

  printf("Created temporary file (fd=%d)\n", fd);
  printf("Check for .XXXXXX files: ls -la test_file.txt*\n");

  if (argc > 1 && strcmp(argv[1], "signal") == 0) {
    printf("Sending SIGTERM to self in 2 seconds...\n");
    sleep(2);
    kill(getpid(), SIGTERM);
  } else if (argc > 1 && strcmp(argv[1], "abort") == 0) {
    printf("Aborting in 2 seconds...\n");
    sleep(2);
    abort();
  } else {
    printf("Exiting normally in 2 seconds...\n");
    sleep(2);
    /* Exit without calling zclose - cleanup should handle it */
    exit(0);
  }

  return 0;
}
