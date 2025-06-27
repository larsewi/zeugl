#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "logger.h"
#include "zeugl.h"

#define PRINT_USAGE(prog)                                                      \
  fprintf(stderr, "Usage: %s [-d] [-v] [-h] SOURCE DEST\n", prog);

int main(int argc, char *argv[]) {
  int opt;
  while ((opt = getopt(argc, argv, "dvh")) != -1) {
    switch (opt) {
    case 'd':
      LoggerEnableDebug();
      break;
    case 'v':
      printf("%s %s\n", PACKAGE_NAME, zversion());
      return EXIT_SUCCESS;
    case 'h':
      PRINT_USAGE(argv[0]);
      return EXIT_SUCCESS;
    default:
      PRINT_USAGE(argv[0]);
      return EXIT_FAILURE;
    }
  }

  if (optind >= argc) {
    fprintf(stderr, "Missing source file operand\n");
    PRINT_USAGE(argv[0]);
    return EXIT_FAILURE;
  }
  __attribute__((unused)) const char *src = argv[argc++];

  if (optind >= argc) {
    fprintf(stderr, "Missing destination file operand after '%s'\n", src);
    PRINT_USAGE(argv[0]);
    return EXIT_FAILURE;
  }
  __attribute__((unused)) const char *dst = argv[argc++];

  return EXIT_SUCCESS;
}
