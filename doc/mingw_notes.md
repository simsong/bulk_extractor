# Windows MinGW build and raw-device support

## CI-built Windows executable

The supported Windows executable is a CI artifact built by
[`.github/workflows/mingw.yml`](../.github/workflows/mingw.yml), not by a
native Windows developer build or an installer. The workflow runs on pushes to
`main`, manually dispatched runs, and relevant non-draft pull requests;
documentation-only pull requests are excluded.

On Ubuntu 24.04, the workflow installs the x86_64 MinGW-w64 POSIX toolchain,
checks out a pinned vcpkg revision, builds static Expat, RE2, and Abseil, and
builds a checksum-pinned static libewf release. It then configures an
out-of-tree cross-build with:

```sh
bash bootstrap.sh
mkdir build-mingw && cd build-mingw
PKG_CONFIG="pkg-config --static" \
PKG_CONFIG_LIBDIR="$target_root/lib/pkgconfig" \
CPPFLAGS="-I$target_root/include" \
LDFLAGS="-L$target_root/lib" \
  ../configure --host=x86_64-w64-mingw32 --quiet
grep -q '^#define HAVE_LIBEWF 1' config.h
make -j2 LDFLAGS="-L$target_root/lib -all-static"
```

The resulting PE executable is copied to `bulk_extractor64.exe` and uploaded
as the `bulk_extractor-windows-x86_64` artifact. The workflow verifies that it
is a PE32+ executable larger than 1 MiB and imports only Windows system DLLs;
it rejects MinGW, RE2, Abseil, Expat, zlib, and GNU crypto runtime DLLs.

A Windows runner downloads that exact artifact and verifies that it can:

- recursively scan a directory containing a Unicode filename;
- scan the checked-in E01 fixture and create its report;
- scan the checked-in IP fixture and report its expected IPv4 address; and
- scan an attached disposable FAT32 VHD as a raw drive-letter volume and
  recover a known email address.

The artifact includes static libewf support but is not signed and is not a
release installer. Keep the import check and Windows runtime tests in the same
pull request as any Windows-build change.

## Raw-device input

The Windows executable accepts these raw-device input forms:

- `C:` as a shorthand for `\\.\C:`;
- `\\.\PhysicalDriveN` for a physical disk;
- `\\.\X:` for a drive-letter volume; and
- `\\?\Volume{GUID}` for a named volume, without the trailing backslash used
  for a mounted-volume directory.

Run from an elevated command prompt and use a separate output directory.
bulk_extractor opens the device read-only with shared read/write access. It
does not lock, mount, modify, or write the device. The reader uses
`CreateFileW`, obtains the precise length with `IOCTL_DISK_GET_LENGTH_INFO`,
and uses offset-based `ReadFile` calls; it does not use
`std::filesystem::file_size` or calculated disk geometry.

A volume is not interchangeable with its physical disk: it can have distinct
boundaries or span disks. Windows may require `FSCTL_ALLOW_EXTENDED_DASD_IO`
to read the final sectors of a volume. bulk_extractor does not issue that
control code, so it reports a short read at that boundary rather than bypassing
the operating system's protection.

The CI VHD test covers a drive-letter volume. Before a release, also validate
the physical-drive and volume-GUID forms from an elevated Windows system using
disposable media with known size and known bytes near both ends.
