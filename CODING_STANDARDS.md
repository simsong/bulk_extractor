# Coding Standards and Guide

## Coding Standards
Unless otherwise specified, we follow the (Google C++ Coding Standards)[https://google.github.io/styleguide/cppguide.html] and the (LLVM Coding Standards)[https://llvm.org/docs/CodingStandards.html].

Here are the exceptions:

* We use C++ exceptions to communicate low memory conditions up through the stack.
* Max line width is 120 characters
* C++ filenames end '.cpp' rather than '.cc' (although we may change this).
* External C++ headers end '.hpp' if that is the way they are distributed

We format all of the text using clang-format.

## Coding Guide

1. We try to use the sbuf_t as the the fundamental unit of memory allocation. This allows us no-copy creation of substrings, tracking of the source of every piece of memory from the original disk image (or file), and how the memory was decoded.