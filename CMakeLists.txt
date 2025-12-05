cmake_minimum_required(VERSION 3.12)
project(zeugl VERSION 0.0.1 LANGUAGES C)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Options
option(ENABLE_DEBUG "Enable debugging" OFF)

# Configuration
set(BUFFER_SIZE 65536 CACHE STRING "Buffer size used for file copying (default 64 KiB)")

# Find required packages
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Detect platform for immutable bit support
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(IMMUTABLE_IOCTL TRUE)
    set(IMMUTABLE_CHFLAGS FALSE)
    set(IMMUTABLE_WEAK FALSE)
elseif(CMAKE_SYSTEM_NAME MATCHES "FreeBSD|OpenBSD|NetBSD|Darwin")
    set(IMMUTABLE_IOCTL FALSE)
    set(IMMUTABLE_CHFLAGS TRUE)
    set(IMMUTABLE_WEAK FALSE)
else()
    set(IMMUTABLE_IOCTL FALSE)
    set(IMMUTABLE_CHFLAGS FALSE)
    set(IMMUTABLE_WEAK TRUE)
endif()

# Configure compile flags
if(ENABLE_DEBUG)
    add_compile_options(-g -O0 -Werror -Wall -Wextra -Wconversion -Wformat -Wshadow -Wcast-align -Wunused)
else()
    add_definitions(-DNDEBUG)
endif()

# Generate config.h
include(CheckIncludeFile)
include(CheckFunctionExists)
include(CheckSymbolExists)

check_include_file(fcntl.h HAVE_FCNTL_H)
check_include_file(unistd.h HAVE_UNISTD_H)
check_include_file(sys/file.h HAVE_SYS_FILE_H)
check_include_file(stdint.h HAVE_STDINT_H)
check_include_file(inttypes.h HAVE_INTTYPES_H)
check_include_file(linux/fs.h HAVE_LINUX_FS_H)
check_include_file(sys/ioctl.h HAVE_SYS_IOCTL_H)
check_include_file(stdbool.h HAVE_STDBOOL_H)

check_function_exists(strerror HAVE_STRERROR)
check_function_exists(stpcpy HAVE_STPCPY)
check_function_exists(strdup HAVE_STRDUP)
check_function_exists(strtoul HAVE_STRTOUL)
check_function_exists(chflags HAVE_CHFLAGS)
check_function_exists(malloc HAVE_MALLOC)
check_function_exists(lstat HAVE_LSTAT)

# Configure config.h
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/config.h
)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}
)

# Add subdirectories
add_subdirectory(lib)
add_subdirectory(cli)

# Install public header
install(FILES include/zeugl.h DESTINATION include)

# Enable testing
enable_testing()
