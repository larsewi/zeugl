#include "config.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "filecopy.h"
#include "logger.h"
#include "zeugl.h"

#define USAGE_STRING                                                           \
  "Usage: %s [-f INPUT_FILE] [-c MODE] [-a] [-t] [-b] [-d] [-v] [-h] "         \
  "OUTPUT_FILE\n"

typedef struct {
  const char *opt;
  const char *arg;
  const char *desc;
} option;

static const option options[] = {
    {"f", "FILENAME", "input file ('-' for stdin)"},
    {"c", "MODE", "create file if it does not exist"},
    {"a", NULL, "position file offset at end-of-file"},
    {"t", NULL, "truncate file"},
    {"b", NULL, "don't block on advisory locks or if interrupted"},
    {"d", NULL, "enable debug logging (noop if compiled with NDEBUG=1)"},
    {"v", NULL, "print version name and exit"},
    {"h", NULL, "print this help menu"},
};

int main(int argc, char *argv[]) {
  const char *input_fname = "-";
  int flags = 0;
  mode_t mode = 0;

  char opt_str[BUFSIZ];
  int opt_str_cursor = 0;
  char usage_str[BUFSIZ];
  int usage_str_cursor;

  int ret = snprintf(usage_str, sizeof(usage_str), "Usage: %s", argv[0]);
  assert((ret >= 0) && (ret < sizeof(usage_str)));
  usage_str_cursor += ret;

  for (int i = 0; i < sizeof(options) / sizeof(option); i++) {
    /* Create option string for getopt() */
    if (options[i].arg == NULL) {
        ret = snprintf(opt_str + opt_str_cursor, sizeof(opt_str) - opt_str_cursor, " [-%s]", options[i].opt);
    }
    else {
        ret = snprintf(opt_str + opt_str_cursor, sizeof(opt_str) - opt_str_cursor, " [-%s %s]", options[i].opt, options[i].arg);
    }
    assert((ret >= 0) && (ret < sizeof(opt_str)));
    opt_str_cursor += ret;

    opt_str[opt_str_cursor++] = *options[i].opt;
    if (options[i].arg != NULL) {
      opt_str[opt_str_cursor++] = ':';
    }
    opt_str[opt_str_cursor] = '\0';

    /* Create usage string */
    ret = snprintf(usage_str + usage_str_cursor, sizeof(usage_str) - usage_str_cursor, "\t-%s  %s\n", options[i].opt, options[i].desc);
  }

  int opt;
  while ((opt = getopt(argc, argv, opt_str)) != -1) {
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
    case 'b':
      flags |= Z_NOBLOCK;
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
      printf(USAGE_STRING "\n", argv[0]);
      return EXIT_SUCCESS;
    default:
      fprintf(stderr, USAGE_STRING, argv[0]);
      return EXIT_FAILURE;
    }
  }

  if (optind >= argc) {
    fprintf(stderr, "Missing output file argument\n");
    fprintf(stderr, USAGE_STRING, argv[0]);
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
  }

  return exit_code;
}
