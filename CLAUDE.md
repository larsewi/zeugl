# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

zeugl is a C library that provides atomic file operations with minimal API changes. It provides drop-in replacements for `open()` and `close()` functions (`zopen()` and `zclose()`) while keeping standard I/O functions unchanged. The library uses temporary files and atomic rename operations to ensure file updates are atomic.

## Build Commands

The project supports two build systems: **Autotools** (default) and **CMake**.

### Autotools Build

#### Initial Setup
```bash
./bootstrap.sh                       # Generate configure script
./configure                          # Configure the build
./configure --enable-debug           # Configure with debug flags (adds -g -O0 -Werror -Wall -Wextra)
```

#### Build
```bash
make                                 # Build the library and CLI tool
make -j$(nproc)                      # Build with parallel compilation
```

#### Test
```bash
make check                           # Run the test suite
cd tests && ./testsuite              # Run tests directly
./testsuite -v                       # Run tests with verbose output
./testsuite -x                       # Run tests with shell tracing
./testsuite 5                        # Run specific test number
```

#### Format and Lint
```bash
make format                          # Format C code with clang-format and shell scripts with shfmt
```

#### Clean
```bash
make clean                           # Clean build artifacts
make distclean                       # Clean everything including configure artifacts
make super-clean                     # Git clean -fxd (removes all untracked files)
```

### CMake Build

#### Initial Setup
```bash
mkdir build && cd build              # Create build directory
cmake ..                             # Configure the build
cmake -DENABLE_DEBUG=ON ..           # Configure with debug flags (adds -g -O0 -Werror -Wall -Wextra)
```

#### Build
```bash
cmake --build .                      # Build the library and CLI tool
cmake --build . -j$(nproc)           # Build with parallel compilation
```

#### Install
```bash
cmake --install .                    # Install library and headers (may require sudo)
cmake --install . --prefix=/path     # Install to custom location
```

#### Clean
```bash
rm -rf build/                        # Clean build artifacts
```

## Code Architecture

### Core Library (`lib/`)
- **zeugl.h/c**: Main API providing `zopen()` and `zclose()` functions for atomic file operations
- **filecopy.h/c**: File copying utilities with locking support
- **whackamole.h/c**: Atomic file replacement using rename operations
- **logger.h/c**: Debug logging infrastructure
- **signals.h/c**: Install signal handlers for cleaning up temporary files

### Key Design Patterns

1. **Atomic Operations**: Files are modified by creating a temporary copy, modifying it, then atomically replacing the original using rename(2)

2. **File Locking**: Uses advisory locks (flock) to detect concurrent modifications and optionally retry or fail fast with Z_NOBLOCK flag

3. **Thread Safety**: Uses pthread mutexes when compiled with pthread support to maintain a thread-safe list of open files

4. **Error Handling**: All functions return negative values on error and set errno appropriately

### CLI Tool (`cli/`)
- **main.c**: Command-line interface to the zeugl library
- Usage: `zeugl [-f INPUT_FILE] [-c MODE] [-a] [-t] [-d] [-v] [-h] OUTPUT_FILE`
  - `-f`: Input file (default: stdin)
  - `-c`: Create file with specified mode (octal)
  - `-a`: Append mode
  - `-t`: Truncate file
  - `-d`: Debug output
  - `-v`: Print version information

### Testing (`tests/`)
- Uses GNU Autotest framework
- Test suite defined in `testsuite.at`
- Includes unit tests for file operations and multi-threaded/multi-process scenarios
- Static analysis tests for code formatting (clang-format, shfmt) and quality (cppcheck, shellcheck)

## Important Implementation Details

1. **Temporary Files**: Created with `.XXXXXX` suffix using mkstemp()
2. **File Modes**: Preserves original file permissions, excluding setuid bit
3. **Z_NOBLOCK Flag**: Prevents blocking on file locks and concurrent access detection
4. **Build Systems**: Supports both GNU Autotools (autoconf, automake, libtool) and CMake (3.12+)
