# zeugl

The `zeugl` project provides an open-source library that brings atomic file
operations to your codebase with minimal API changes. The library provides
drop-in replacements for `open()` and `close()` while keeping standard I/O
functions like `read()` and `write()` unchanged, allowing developers to achieve
atomicity with just a few line modifications.
