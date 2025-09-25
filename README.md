# zeugl

<img src="icon.svg" alt="zeugl icon" width="100" height="100">

A C library that provides atomic file operations with minimal API changes.

## What is zeugl?

zeugl is a lightweight C library that makes file operations atomic by providing
drop-in replacements for standard file operations. Instead of directly modifying
files (which can lead to corruption), zeugl ensures all file updates are atomic
- they either complete fully or don't happen at all.

The library provides drop-in replacements for `open()` and `close()` while
keeping standard I/O functions like `read()` and `write()` unchanged, allowing
developers to achieve atomicity with just a few line modifications.

## Key Features

- **Atomic File Operations**: All file modifications are atomic using temporary
  files and rename operations
- **Drop-in Replacement**: Simple API with `zopen()` and `zclose()` replacing
  `open()` and `close()`
- **Standard I/O Compatible**: Works seamlessly with `read()`, `write()`, and
  other standard I/O functions
- **Concurrent Access Protection**: Built-in file locking to detect and handle
  concurrent modifications
- **Thread-Safe**: Maintains thread safety when compiled with pthread support
- **Signal Handling**: Automatic cleanup of temporary files on interruption

## How It Works

1. When you open a file with `zopen()`, zeugl returns a file descriptor to a
   temporary copy
2. All modifications happen on the temporary copy
3. When you call `zclose()`, the temporary file atomically replaces the original
4. If there is a raise between two processes, its guaranteed that one (and only
   one) process gets to replace the file
5. If the process is interrupted, temporary files are automatically cleaned up

## Quick Start

### Building

```bash
./bootstrap.sh
./configure
make
make check
sudo make install
```

### Basic Usage

```c
#include <zeugl.h>

// Start atomic file transaction
int fd = zopen("config.txt", flags, mode);
// Use fd with standard I/O functions
write(fd, buffer, size);
// Commit/abort file transaction
zclose(fd, true);  // Atomic replacement happens here
```

### Command Line Tool

The CLI tool is mainly used for testing, but can be used for updating files
atomically on the command line or in shell scripts.

```bash
# Atomically update a file
echo "new content" | zeugl -c 644 output.txt
```

## Contributing

Contributions are welcome!
