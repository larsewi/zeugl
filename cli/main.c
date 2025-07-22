#include "config.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "logger.h"
#include "zeugl.h"

#define PRINT_USAGE(prog)                                                      \
  fprintf(stderr, "Usage: %s [-f INPUT_FILE] [-d] [-v] [-h] OUTPUT_FILE\n",    \
          prog);

int main(int argc, char *argv[]) {
  const char *input_file = "-";

  int opt;
  while ((opt = getopt(argc, argv, "f:dvh")) != -1) {
    switch (opt) {
    case 'f':
      input_file = optarg;
      break;
    case 'd':
      LoggerEnable();
      break;
    case 'v':
      printf("%s\n", PACKAGE_STRING);
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
    fprintf(stderr, "Missing output file argument\n");
    PRINT_USAGE(argv[0]);
    return EXIT_FAILURE;
  }
  const char *output_file = argv[optind++];

  LOG_DEBUG("Input file '%s'", input_file);
  LOG_DEBUG("Output file '%s'", output_file);

  int fd = zopen(output_file);
  if (fd < 0) {
    LOG_DEBUG("Failed to open file '%s': zopen(): %s", output_file,
              strerror(errno));
    return EXIT_FAILURE;
  }

  zclose(fd);
  return EXIT_SUCCESS;
}
