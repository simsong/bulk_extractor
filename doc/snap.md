# Snap package

The project-maintained `bulk-extractor` Snap is a supplemental Ubuntu delivery
channel. The primary distribution path remains Debian source package, Ubuntu
Universe sync, and Kali downstream update.

## Normal image-file scans

The package uses strict confinement. After installation, connect `home` to scan
ordinary image files stored in a user's non-hidden home directory:

```sh
sudo snap install bulk-extractor
sudo snap connect bulk-extractor:home :home
bulk-extractor -o ~/bulk-extractor-output ~/image.dd
```

For image files stored on mounted removable media, explicitly connect
`removable-media`; it permits files under `/media`, `/run/media`, and `/mnt`:

```sh
sudo snap connect bulk-extractor:removable-media :removable-media
bulk-extractor -o ~/bulk-extractor-output /mnt/image.dd
```

## Raw devices

Raw-device scanning is deliberately not enabled by default. It uses the
super-privileged `block-devices` interface and therefore requires a Snap Store
declaration before publication. Once the Store has approved that declaration,
an administrator must explicitly connect the interface:

```sh
sudo snap connect bulk-extractor:block-devices :block-devices
sudo snap run bulk-extractor -o /mnt/bulk-extractor-output /dev/sdb
```

`block-devices` is requested with `allow-partitions: true`, so both disks and
their partition nodes can be read when the installed snapd supports that
attribute. The application opens input read-only; nevertheless, raw-device
selection remains an administrator responsibility.

## Release administration

Before a stable publication, the release manager must register
`bulk-extractor` under a project-owned Snapcraft account and add named project
collaborators. Store credentials are exported with only package access, push,
update, and release ACLs, scoped to this package and an expiry, and saved as
the protected `SNAPCRAFT_STORE_CREDENTIALS` secret for the `snap-store`
environment. They are never placed in the repository.

The `Snap build and test` workflow builds and install-tests amd64 and arm64
packages from a non-sensitive FAT32 image fixture. It publishes only an
annotated `vX.Y.Z` tag whose version matches `configure.ac`, after both
architecture jobs succeed. Record the store revision, architectures, and the
install-test workflow URLs in the release issue.
