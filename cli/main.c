#include "config.h"

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "filecopy.h"
#include "logger.h"
#include "zeugl.h"

#define PRINT_USAGE(prog)                                                      \
  fprintf(stderr,                                                              \
          "Usage: %s [-f INPUT_FILE] [-c MODE] [-a] [-t] [-i] [-d] [-v] [-h] " \
          "OUTPUT_FILE\n",                                                     \
          prog)

int main(int argc, char *argv[]) {
  const char *input_fname = "-";
  int flags = 0;
  mode_t mode = 0;

  int opt;
  while ((opt = getopt(argc, argv, "f:c:atidvh")) != -1) {
    switch (opt) {
    case 'f':
      input_fname = optarg;
      break;
    case 'c': {
      flags |= Z_CREATE;
      char *endptr = NULL;
      errno = 0;
      unsigned long ret = strtoul(optarg, &endptr, 8);
      if (errno != 0) {
        LOG_DEBUG("Failed to parse mode string '%s': %s", endptr,
                  strerror(errno));
        return EXIT_FAILURE;
      }
      if ((*optarg == '\0') || (*endptr != '\0') || (ret > 0777)) {
        LOG_DEBUG("Failed to parse mode string '%s': Bad argument", endptr);
        return EXIT_FAILURE;
      }
      mode = (mode_t)ret;
    } break;
    case 'a':
      flags |= Z_APPEND;
      break;
    case 't':
      flags |= Z_TRUNCATE;
      break;
    case 'i':
      flags |= Z_IMMUTABLE;
      break;
    case 'd':
#if !NDEBUG
      LoggerEnable();
#endif
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
  const char *output_fname = argv[optind++];

  int output_fd = zopen(output_fname, flags, mode);
  if (output_fd < 0) {
    LOG_DEBUG("Failed to begin transaction for output file '%s': %s",
              output_fname, strerror(errno));
    return EXIT_FAILURE;
  }
  LOG_DEBUG("Began transaction for output file '%s' (fd = %d)", output_fname,
            output_fd);

  int exit_code = EXIT_FAILURE;
  bool commit_transaction = false;
  bool input_is_stdin = strcmp(input_fname, "-") == 0;
  int input_fd = STDIN_FILENO;

  if (input_is_stdin) {
    LOG_DEBUG("Using stdin (fd = %d) as input file", input_fd);
  } else {
    input_fd = open(input_fname, O_RDONLY);
    if (input_fd < 0) {
      LOG_DEBUG("Failed to open input file '%s': %s", input_fname,
                strerror(errno));
      goto FAIL;
    }
    LOG_DEBUG("Opened input file '%s' (fd = %d)", input_fname, input_fd);
  }

  if (!filecopy(input_fd, output_fd)) {
    LOG_DEBUG("Failed to write content from input file '%s' (fd = %d) to "
              "output file '%s' (fd = %d)",
              input_fname, input_fd, output_fname, output_fd);
    goto FAIL;
  }
  LOG_DEBUG("Successfully wrote content from input file '%s' (fd = %d) to "
            "output file '%s' (fd = %d)",
            input_fname, input_fd, output_fname, output_fd);

  exit_code = EXIT_SUCCESS;
  commit_transaction = true;

FAIL:

  if (!input_is_stdin && (input_fd != -1)) {
    if (close(input_fd) == 0) {
      LOG_DEBUG("Closed input file '%s' (fd = %d)", input_fname, input_fd);
    } else {
      LOG_DEBUG("Failed to close input file '%s' (fd = %d): %s", input_fname,
                input_fd, strerror(errno));
    }
  }

  if (zclose(output_fd, commit_transaction) == 0) {
    LOG_DEBUG("Successfully %s transaction for output file '%s' (fd = %d)",
              commit_transaction ? "committed" : "aborted", output_fname,
              output_fd);
  } else {
    LOG_DEBUG("Failed to %s transaction for file '%s' (fd = %d): %s",
              commit_transaction ? "commit" : "abort", output_fname, output_fd,
              strerror(errno));
    return EXIT_FAILURE;
  }

  return exit_code;
}
