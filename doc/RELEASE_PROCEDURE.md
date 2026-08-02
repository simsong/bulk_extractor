# bulk_extractor release procedure

This procedure defines how to prepare, validate, assemble, and publish a
versioned `bulk_extractor` release. Use it with the release issue and retain
the completed evidence there. Do not record case data, unredacted scan output,
credentials, or presigned URLs in GitHub.

## Release model

A normal pull request runs CI and may create short-lived test artifacts. It
must not publish release assets or use package-signing credentials. A release
PR is the sole place to prepare a versioned release: it updates the version in
`configure.ac`, release notes, package metadata, and any procedure changes.

Draft pull requests are planning/release-preparation records. Required CI jobs
skip them and run when the pull request is marked ready for review. GitHub still
records a skipped workflow run; Actions cannot filter `pull_request` events on
the draft field before a workflow starts. Pushes to `dev-release` do not run the
main-branch push workflows.

After the release PR is merged and all required checks are green, create a
signed annotated tag named `vX.Y.Z` at the reviewed commit. The tag is the
immutable release identity; never move or reuse it. A protected tag-triggered
workflow should create a *draft* GitHub Release. The release manager reviews
the evidence below and explicitly publishes the draft.

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

Complete and record each gate before artifact publication.

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
6. Run the AWS large-image validation described below, then preserve only its
   redacted result summary, checksums, and pass/fail evidence.

## AWS large-image validation

The AWS validation infrastructure is tracked in [#624][aws-issue]. Until that
work exists, this gate is manual and must not be represented as automated.

The eventual CloudFormation or SAM implementation must:

- use a fixed instance type, maximum runtime, and automatic instance/volume
  cleanup;
- use encrypted S3, scoped IAM roles, and GitHub OIDC or presigned URLs rather
  than long-lived repository credentials;
- redact image and output data; post only an attested summary, logs, checksums,
  and pass/fail status to GitHub;
- have AWS Budget alerts and a separate runtime/cost guardrail. Budgets are
  alerts, not instantaneous hard spending caps, so the runtime guardrail is
  required to keep expected CPU costs at or below $10.

Confirm that the instance, volumes, and temporary S3 objects have been removed
before marking the gate complete.

## Artifact assembly

`make release` is deliberately an assembly gate, not a credential-bearing
publisher. It invokes `scripts/release.sh`, which preflights every required
input, creates a detached temporary Git worktree at `HEAD`, and runs bootstrap,
macOS `make distcheck`, and container gates there. It captures logs and source
provenance, stages the source archive and validated inputs in
`release-artifacts/`, writes `SHA256SUMS`, then removes the worktree. It refuses
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

The staged set must contain:

| Artifact | Current producer | Required release evidence |
| --- | --- | --- |
| Source archive | `make distcheck` | Version and source-tag match |
| Windows `.exe` | MinGW workflow | Exact artifact passed Windows test |
| Debian `.deb` | [#622][deb-issue] | Clean package build and installed-package smoke test |
| RPM/SRPM | [#623][rpm-issue] | Clean RPM build and installed-package smoke test |
| AWS summary | [#624][aws-issue] | Redacted large-image build/scan result and cleanup evidence |
| `SHA256SUMS` | `make release` | Checksums verified before upload |

Before upload, verify each staged filename, version, checksum, and provenance
against the signed tag. A source archive or package built from any other commit
is a release failure.

## Publication

1. Attach the staged artifacts and `SHA256SUMS` to the draft GitHub Release.
2. Check the release notes, tag, commit, asset names, checksums, and links.
3. Submit signed Debian artifacts to the selected Debian repository and record
   the resulting URL. The reproducible build and repository choice are tracked
   in [#622][deb-issue].
4. Submit signed RPM artifacts to the selected RPM repository and record the
   resulting URL. The reproducible build and repository choice are tracked in
   [#623][rpm-issue].
5. Publish the GitHub Release only after a release manager has reviewed every
   required gate and external package publication is either complete or clearly
   disclosed in the release notes.

## Automation roadmap

The following release-engineering issues close the remaining automation gaps:

- [#621][github-release-issue]: tag-driven draft GitHub Release creation,
  Windows artifact assembly, and checksums.
- [#622][deb-issue]: reproducible Debian builds, signing, and publication.
- [#623][rpm-issue]: reproducible RPM builds, signing, and publication.
- [#624][aws-issue]: budget-capped AWS large-image validation and secure
  reporting.

Once these are complete, a release manager should be able to direct Codex to
prepare a release PR, validate the tag, trigger the protected workflow, and
review the resulting draft release without handling package or cloud secrets.

[github-release-issue]: https://github.com/simsong/bulk_extractor/issues/621
[deb-issue]: https://github.com/simsong/bulk_extractor/issues/622
[rpm-issue]: https://github.com/simsong/bulk_extractor/issues/623
[aws-issue]: https://github.com/simsong/bulk_extractor/issues/624
