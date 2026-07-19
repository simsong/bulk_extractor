## Building on Windows

be20_api is built as part of bulk_extractor. It has no separate checkout or
submodule initialization step.

Under MSYS2, install the UCRT64 toolchain and dependencies:

```sh
pacman -S \
  base-devel \
  mingw-w64-ucrt-x86_64-gcc \
  mingw-w64-ucrt-x86_64-make \
  mingw-w64-ucrt-x86_64-re2 \
  mingw-w64-ucrt-x86_64-abseil-cpp \
  mingw-w64-ucrt-x86_64-sqlite3 \
  mingw-w64-ucrt-x86_64-openssl \
  mingw-w64-ucrt-x86_64-expat
```

Run the parent build and tests through its Makefile:

```sh
./bootstrap.sh
./configure
make
make check
```
