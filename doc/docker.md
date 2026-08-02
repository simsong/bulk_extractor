# Container image

`Dockerfile` builds a Linux container image for scanning regular image files.
It builds the project's default configuration with libewf disabled, matching the
Debian Bookworm compatibility configuration in CI. It does not include
Lightgrep and does not claim to be a privileged raw-device appliance.

Build from a source checkout:

```
docker build --tag bulk-extractor:local .
```

Scan a regular image with an immutable input mount and a writable output mount:

```
docker run --rm \
  --mount type=bind,source="$PWD/input",target=/input,readonly \
  --mount type=bind,source="$PWD/output",target=/output \
  bulk-extractor:local -o /output /input/image.raw
```

The runtime process is the unprivileged `bulk_extractor` user. Do not add
`--privileged` or device mounts for ordinary image files. Raw-device scanning
has host-specific access, consistency, and disclosure risks and is not part of
this image's supported interface.

The image supplements, rather than replaces, the native macOS, Ubuntu, MinGW,
and Windows-runtime CI jobs. To validate the image locally, build it and run a
small checked-in or analyst-provided regular image with the invocation above.
