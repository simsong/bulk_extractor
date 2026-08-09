# bulk_extractor release procedure

This procedure defines how to prepare, validate, assemble, and publish a
versioned `bulk_extractor` release. Use it with the release issue and retain
the completed evidence there. Do not record case data, unredacted scan output,
credentials, or presigned URLs in GitHub.

## Release model

A normal pull request runs CI and may create short-lived test artifacts. It
must not publish release assets or use package-signing credentials. A release
PR prepares a versioned release: it updates the version in `configure.ac`,
release notes, package metadata, and any procedure changes.

Draft pull requests are planning/release-preparation records. Required CI jobs
skip them and run when the pull request is marked ready for review. GitHub still
records a skipped workflow run; Actions cannot filter `pull_request` events on
the draft field before a workflow starts. Pushes to `dev-release` do not run the
main-branch push workflows.

`configure.ac` is the sole authoritative package-version source. For a final
release, set `AC_INIT([BULK_EXTRACTOR], [X.Y.Z], ...)`: package metadata and
source-archive names derive from it. The leading `v` belongs only in the Git
tag, not in the final package version. Promote any development identifier (for
example, `v2.2.0alpha1`) to its final `X.Y.Z` value before tagging. Do not
duplicate the value in a workflow or release script.

After the release PR is merged and required checks are green, create a signed
annotated tag `vX.Y.Z` at the reviewed commit. The tag is immutable: never move
or reuse it. There are two supported ways to create the GitHub *draft* release:

1. **Automated draft:** Push the signed tag, or dispatch
   `.github/workflows/release.yml` for that tag. The workflow waits for the
   successful source-distribution and MinGW runs for the tagged commit,
   assembles the source archive, Windows executable, and `SHA256SUMS`, then
   creates a draft release.
2. **Manual draft:** Go to [Releases](https://github.com/simsong/bulk_extractor/releases),
   click **Draft a new release**, choose the existing signed tag, title it
   `BE vX.Y.Z`, generate or paste the reviewed notes, attach the verified
   artifacts and checksums, and click **Save draft**.

Creating or saving a draft release does **not** run tests. The automated path
waits for its tag-triggered tests before it creates the draft; the manual path
requires the release manager to confirm the recorded validation evidence first.
In either path, the release manager reviews the draft and explicitly clicks
**Publish release**. That click makes the GitHub release public.

Use a `release/X.Y` branch only when maintaining an established release line
while `main` continues with new development. Do not create a release branch for
an ordinary single-release cycle.

## Prerequisites

Before beginning, open a release issue and record:

- target version, tag, and reviewed commit SHA;
- the release manager and package-repository maintainers;
- approved large-image identifiers and checksums, without exposing case data;
- URLs for all build and validation jobs;
- every artifact filename, SHA-256 checksum, provenance, and publication URL.

Confirm that the release PR has a clean required-check status and that its
version matches the intended tag. Do not release from an unreviewed local
checkout or from a mutable branch reference.

## Build and validation gates

The release issue selects and records the required gates. Complete the baseline
gates below before GitHub publication; run the large-image and downstream
package gates when they are in that release's declared scope.

1. On a clean local macOS installation, bootstrap, configure, build, and run
   `make distcheck`. Record macOS version, architecture, compiler, commands,
   and logs.
2. On that Mac, scan an approved large disk image. Record the immutable image
   identifier/checksum, command line, elapsed time, peak resources, exit
   status, and output checksum/location. Never upload confidential image or
   output content to GitHub.
3. Run `make distcheck-ubuntu-container`. It builds a clean Ubuntu 22.04
   container and runs `make distcheck` for both `linux/arm64` and
   `linux/amd64` by default.
4. Run `make distcheck-fedora-container`. It builds a clean Fedora 44 container
   and runs `make distcheck` for both `linux/arm64` and `linux/amd64` by
   default. Together, `make distcheck-containers` runs both operating-system
   gates.
5. Run `.github/workflows/mingw.yml` and verify the Windows Unicode filename
   test against the exact `bulk_extractor64.exe` artifact. The current build
   uses `--disable-libewf`; it is neither signed nor E01-capable unless the
   workflow and its evidence are changed accordingly.
6. Run the AWS large-image validation described below, retaining its result
   archives, per-run text reports, and pass/fail evidence in the selected S3
   bucket.

## AWS large-image validation

`make release-aws-large-image` runs the interactive Bash launcher in
[`scripts/release_aws_large_image.sh`](../scripts/release_aws_large_image.sh).
It has no CloudFormation template or GitHub Actions workflow. The launcher
starts every combination of its `INSTANCE_TYPES` and `IMAGE_URLS` variables;
the defaults are 4-vCPU `m7i.xlarge` and 16-vCPU `m7i.4xlarge` against the
public ubnist1 and domexusers Digital Corpora downloads.

Supply a bucket, public subnet, SSH-enabled security group, and EC2 key pair:

```sh
make release-aws-large-image RESULT_BUCKET=be-release-results \
  SECURITY_GROUP_ID=sg-... SSH_KEY_NAME=be-release \
  SSH_PRIVATE_KEY=/secure/path/be-release.pem INSTANCE_PROFILE=be-release-runner
```

Each Ubuntu instance installs build prerequisites, downloads and builds the
selected source release, downloads its assigned disk image, scans it, and
uploads `BE{version}-{instance-type}-{image}-{utc-time}.zip` plus its matching
`.txt` report to the bucket. It then shuts down. The local launcher uses SSH to
tail each instance log and redraw a terminal progress table; when every
instance is stopped it prints the uploaded status and elapsed time for each.
Unless `SUBNET_ID` is supplied, the launcher selects a default subnet in the
account's default VPC.
The operator is responsible for an instance profile that permits only the
needed bucket writes and for terminating the stopped instances when retained
evidence is no longer needed.

## Artifact assembly

`make release` is deliberately an assembly gate, not a credential-bearing
publisher. It invokes `scripts/release.sh`, which preflights every required
input, requires `RELEASE_SOURCE_DIR` to name a Git worktree, creates a detached
temporary worktree at `HEAD`, and runs bootstrap,
macOS `make distcheck`, and container gates there. It captures logs and source
provenance, stages the source archive and supplied artifact inputs in
`release-artifacts/`, writes `SHA256SUMS` with `shasum` or `sha256sum`, then
removes the worktree. It refuses
to run from a checkout with tracked or staged changes, to overwrite an existing
artifact directory, or to proceed when an input is absent. The active checkout
is not built, configured, or cleaned by the release process.

`make release` also runs `make distcheck-containers`, so Finch is a release
prerequisite on macOS. On Apple Silicon, `arm64` runs natively and `amd64` uses
Finch's Linux emulation. The emulated run is slower and is compatibility
coverage rather than a performance result. Use
`CONTAINER_PLATFORMS=arm64` when diagnosing a native-only failure, or
`CONTAINER_PLATFORMS=amd64` when reproducing the emulated x86_64 result.

Finch is the default engine because it is available in this development
environment. The targets use the portable `build --platform` interface; a
developer who has started Colima and installed the Docker CLI may instead run
them with `CONTAINER_ENGINE=docker`. Use one active engine per run; do not
assume a stopped Colima profile can serve Finch commands.

Provide the artifact paths explicitly:

```sh
make release \
  RELEASE_WINDOWS_EXE=/path/to/bulk_extractor64.exe \
  RELEASE_DEB=/path/to/bulk_extractor_VERSION_ARCH.deb \
  RELEASE_RPM=/path/to/bulk_extractor-VERSION-RELEASE.ARCH.rpm \
  RELEASE_AWS_RESULT=/path/to/aws-large-image-summary.txt
```

To use a different empty staging directory, pass
`RELEASE_ARTIFACT_DIR=/absolute/path` to `make release`.

### GitHub source and Windows assembly

`scripts/assemble_github_release.py` is the testable assembly path for the
GitHub source archive and tested MinGW executable. It reads the version only
from `configure.ac`, verifies that the source archive embeds that same version,
and writes these files to an empty output directory:

- `bulk_extractor-X.Y.Z.tar.gz`
- `bulk_extractor64.exe`
- `SHA256SUMS`

To test assembly without a tag, GitHub credentials, or a release mutation,
provide artifacts created by the existing workflows (or equivalent local test
inputs) and use `--dry-run`:

```sh
python3 scripts/assemble_github_release.py \
  --source-archive /path/to/bulk_extractor-X.Y.Z.tar.gz \
  --windows-executable /path/to/bulk_extractor64.exe \
  --output-dir /tmp/bulk-extractor-release-test \
  --dry-run
```

For a tag-triggered release, the GitHub workflow supplies the repository and
tag. The script waits for the current source and MinGW workflow runs, downloads
their artifacts, verifies their names and version, and creates only a draft
release. It never publishes that release.

The staged set must contain:

| Artifact | Current producer | Required release evidence |
| --- | --- | --- |
| Source archive | `make distcheck` | Version and source-tag match |
| Windows `.exe` | MinGW workflow | Exact artifact passed Windows test |
| Debian `.deb` | [#622][deb-issue] | Clean package build and installed-package smoke test; direct-download artifact only |
| RPM/SRPM | [#623][rpm-issue] | Clean RPM build and installed-package smoke test; direct-download artifact only |
| AWS summary | [#624][aws-issue] | Redacted large-image build/scan result and cleanup evidence |
| `SHA256SUMS` | `make release` | Checksums verified before upload |

Before upload, verify each staged filename, version, checksum, and provenance
against the signed tag. A source archive or package built from any other commit
is a release failure.

## Publication

1. Create the draft by one of the two paths above. If the automated path
   created it, do not create a second manual draft; inspect and supplement that
   draft instead.
2. Check the release title (`BE vX.Y.Z`), notes, tag, commit, asset names,
   checksums, and links. Attach any verified artifacts and `SHA256SUMS` that
   are not already present.
3. Click **Publish release** when the selected GitHub-release gates are green.
   This publishes the GitHub release; it does not submit packages to external
   distribution archives or stores.
4. Publish the signed upstream source archive and its checksums. Distribution
   archives build their own binaries: do not submit a prebuilt `.deb` or RPM to
   Debian, Kali, Fedora, or openSUSE as an archive update.
5. Submit the Debian *source package* through an authorized Debian maintainer
   or sponsor and record the accepted source-package URL. Kali normally imports
   from Debian; when it carries packaging changes, submit a version-bump request
   or merge request against Kali's packaging repository using the same tagged
   upstream source. Ubuntu normally imports Debian packages into Universe before
   its Debian Import Freeze; after that point, record the required Launchpad
   sync request. Do not pursue Ubuntu's default third-party-source list for this
   free-software tool. The reproducible build and downstream tracking are in
   [#622][deb-issue].
6. Run `make release-rpm RELEASE_TAG=vVERSION`. It requires an annotated tag
   at `HEAD` and a completely clean checkout, then builds the tagged source in
   version-pinned Fedora and openSUSE Leap environments. It writes each
   distribution's binary RPM and SRPM to its subdirectory under
   `release-rpm-artifacts/` and verifies the installed binary in the same
   clean environment. These are GitHub
   direct-download/test artifacts only.
7. Submit source-level RPM packaging changes: for Fedora, determine whether a
   package already exists; otherwise file a new-package review at
   <https://bugzilla.redhat.com/enter_bug.cgi?product=Fedora&component=Package%20Review>.
   For an existing Fedora package, use its dist-git update workflow. For
   openSUSE, identify the live package repository and maintainer in
   <https://build.opensuse.org/>, fork that repository in openSUSE Gitea, and
   submit a pull request updating the spec, source reference, and changelog.
   Let the distribution build service build and archive the RPMs; record each
   accepted request URL and downstream build result in the release issue. The
   reproducible build and downstream tracking are in [#623][rpm-issue].
External package and store publication may follow the GitHub release. Record
their state and URLs in the release issue; do not represent an unsubmitted
downstream package as published.

The release may attach locally built `.deb`, RPM, and SRPM files to GitHub for
download and installation testing. Those files are not substitutes for the
source-package submissions above. Distribution archives apply their own signing
and build policies; keep any contributor or maintainer credentials outside this
repository and outside `make release`.

## Automation status and optional follow-up

The repository implementation for these release steps is complete:

- [#621][github-release-issue]: tag-driven draft GitHub Release creation,
  Windows artifact assembly, and checksums.
- [#622][deb-issue]: reproducible Debian source/binary package builds and
  release artifacts. Archive submission still requires an authorized Debian
  maintainer or sponsor.
- [#623][rpm-issue]: reproducible Fedora/openSUSE RPM and SRPM builds. Fedora
  and openSUSE submission still requires the relevant downstream account.

The following remain optional release-execution work, not prerequisites for
clicking **Publish release** unless the release issue makes them a gate:

- [#624][aws-issue]: budget-capped AWS large-image validation and secure
  reporting.
- [#626][snap-issue]: optional project-owned Snap Store publication for Ubuntu
  users; it does not replace the Debian-to-Ubuntu path. The strict-confinement
  interface model, local `make snap` entry point, amd64/arm64 install-test
  workflow, and protected annotated-tag stable publication are documented in
  [doc/snap.md](snap.md).

Once these are complete, a release manager should be able to direct Codex to
prepare a release PR, validate the tag, trigger the protected workflow, and
review the resulting draft release without handling package or cloud secrets.

[github-release-issue]: https://github.com/simsong/bulk_extractor/issues/621
[deb-issue]: https://github.com/simsong/bulk_extractor/issues/622
[rpm-issue]: https://github.com/simsong/bulk_extractor/issues/623
[aws-issue]: https://github.com/simsong/bulk_extractor/issues/624
[snap-issue]: https://github.com/simsong/bulk_extractor/issues/626
