#!/usr/bin/env python3
"""Assemble or publish the source and Windows assets for a tagged release."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import shutil
import subprocess
import sys
import tarfile
import tempfile
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Any


CONFIGURE_VERSION_RE = re.compile(r"^AC_INIT\(\[BULK_EXTRACTOR\],\[([^]]+)\]", re.MULTILINE)
SOURCE_WORKFLOW = "create-release-installer.yml"
WINDOWS_WORKFLOW = "mingw.yml"
WINDOWS_ARTIFACT = "bulk_extractor-windows-x86_64"
RUNS_KEY = "workflow_runs"
ID_KEY = "id"
STATUS_KEY = "status"
CONCLUSION_KEY = "conclusion"


@dataclass(frozen=True)
class ReleaseIdentity:
    version: str
    tag: str
    commit: str


def command(*args: str, capture_output: bool = False) -> subprocess.CompletedProcess[str]:
    return subprocess.run(args, check=True, text=True, capture_output=capture_output)


def configured_version(source_dir: Path) -> str:
    match = CONFIGURE_VERSION_RE.search((source_dir / "configure.ac").read_text())
    if match is None:
        raise ValueError("could not read BULK_EXTRACTOR version from configure.ac")
    return match.group(1)


def release_identity(source_dir: Path, tag: str | None, require_tag: bool) -> ReleaseIdentity:
    version = configured_version(source_dir)
    expected_tag = f"v{version}"
    if tag is not None and tag != expected_tag:
        raise ValueError(f"tag {tag} does not match configure.ac version {version}")
    actual_tag = tag or expected_tag
    if require_tag:
        tag_type = command("git", "-C", str(source_dir), "cat-file", "-t", f"refs/tags/{actual_tag}", capture_output=True).stdout.strip()
        if tag_type != "tag":
            raise ValueError(f"{actual_tag} must be an annotated tag")
        commit_ref = f"{actual_tag}^{{commit}}"
    else:
        commit_ref = "HEAD"
    commit = command("git", "-C", str(source_dir), "rev-parse", commit_ref, capture_output=True).stdout.strip()
    return ReleaseIdentity(version=version, tag=actual_tag, commit=commit)


def workflow_run(repo: str, workflow: str, commit: str, timeout: int) -> int:
    deadline = time.monotonic() + timeout
    endpoint = f"repos/{repo}/actions/workflows/{workflow}/runs?event=push&head_sha={commit}&per_page=100"
    while time.monotonic() < deadline:
        response = command("gh", "api", endpoint, capture_output=True)
        payload: dict[str, Any] = json.loads(response.stdout)
        runs: list[dict[str, Any]] = payload.get(RUNS_KEY, [])
        if runs:
            run = runs[0]
            status = run.get(STATUS_KEY)
            conclusion = run.get(CONCLUSION_KEY)
            if status == "completed":
                if conclusion == "success":
                    return int(run[ID_KEY])
                raise RuntimeError(f"{workflow} failed for {commit}: {conclusion}")
        time.sleep(15)
    raise TimeoutError(f"timed out waiting for {workflow} at {commit}")


def download_artifact(repo: str, run_id: int, artifact: str, destination: Path) -> Path:
    destination.mkdir(parents=True, exist_ok=True)
    command("gh", "run", "download", str(run_id), "--repo", repo, "--name", artifact, "--dir", str(destination))
    matches = list(destination.rglob("*"))
    files = [path for path in matches if path.is_file()]
    if len(files) != 1:
        raise RuntimeError(f"expected one file in downloaded {artifact} artifact, found {len(files)}")
    return files[0]


def archive_version(archive: Path) -> str:
    with tarfile.open(archive, "r:gz") as source:
        candidates = [member for member in source.getmembers() if member.name.endswith("/configure.ac")]
        if len(candidates) != 1:
            raise ValueError(f"{archive} does not contain exactly one configure.ac")
        extracted = source.extractfile(candidates[0])
        if extracted is None:
            raise ValueError(f"could not read configure.ac from {archive}")
        match = CONFIGURE_VERSION_RE.search(extracted.read().decode())
    if match is None:
        raise ValueError(f"could not read version from {archive}")
    return match.group(1)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def assemble(identity: ReleaseIdentity, source_archive: Path, windows_executable: Path, output_dir: Path) -> list[Path]:
    expected_archive = f"bulk_extractor-{identity.version}.tar.gz"
    if source_archive.name != expected_archive:
        raise ValueError(f"source archive must be named {expected_archive}")
    if archive_version(source_archive) != identity.version:
        raise ValueError("source archive configure.ac version does not match release tag")
    if not windows_executable.is_file() or windows_executable.stat().st_size == 0:
        raise ValueError("Windows executable must be a non-empty file")
    if output_dir.exists() and any(output_dir.iterdir()):
        raise ValueError(f"output directory must be empty: {output_dir}")
    output_dir.mkdir(parents=True, exist_ok=True)
    staged_archive = output_dir / expected_archive
    staged_windows = output_dir / "bulk_extractor64.exe"
    shutil.copy2(source_archive, staged_archive)
    shutil.copy2(windows_executable, staged_windows)
    checksums = output_dir / "SHA256SUMS"
    checksums.write_text("".join(f"{sha256(path)}  {path.name}\n" for path in (staged_archive, staged_windows)))
    return [staged_archive, staged_windows, checksums]


def publish_draft(repo: str, identity: ReleaseIdentity, assets: list[Path]) -> None:
    check = subprocess.run(("gh", "release", "view", identity.tag, "--repo", repo), text=True, capture_output=True)
    if check.returncode == 0:
        raise RuntimeError(f"release already exists for {identity.tag}")
    command(
        "gh", "release", "create", identity.tag, "--repo", repo, "--target", identity.commit,
        "--draft", "--title", f"bulk_extractor {identity.version}", "--generate-notes",
        *(str(asset) for asset in assets),
    )


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-dir", type=Path, default=Path.cwd())
    parser.add_argument("--tag", help="Annotated tag; defaults to v<version from configure.ac>.")
    parser.add_argument("--source-archive", type=Path)
    parser.add_argument("--windows-executable", type=Path)
    parser.add_argument("--repo", help="GitHub owner/repository used to retrieve workflow artifacts.")
    parser.add_argument("--wait-seconds", type=int, default=3600)
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--dry-run", action="store_true", help="Assemble files only; never create a GitHub Release.")
    parser.add_argument("--publish", action="store_true", help="Create a draft GitHub Release after assembly.")
    args = parser.parse_args()
    if args.dry_run and args.publish:
        parser.error("--dry-run and --publish are mutually exclusive")
    if (args.source_archive is None) != (args.windows_executable is None):
        parser.error("provide both local artifact paths or neither")
    if args.publish and args.repo is None:
        parser.error("--repo is required with --publish")
    if args.source_archive is None and args.repo is None:
        parser.error("--repo is required when artifacts are not supplied locally")
    identity = release_identity(args.source_dir, args.tag, args.publish or args.source_archive is None)
    temporary: tempfile.TemporaryDirectory[str] | None = None
    try:
        if args.source_archive is None:
            temporary = tempfile.TemporaryDirectory(prefix="bulk-extractor-release-")
            download_dir = Path(temporary.name)
            source_run = workflow_run(args.repo, SOURCE_WORKFLOW, identity.commit, args.wait_seconds)
            windows_run = workflow_run(args.repo, WINDOWS_WORKFLOW, identity.commit, args.wait_seconds)
            args.source_archive = download_artifact(args.repo, source_run, f"bulk_extractor-{identity.version}", download_dir / "source")
            args.windows_executable = download_artifact(args.repo, windows_run, WINDOWS_ARTIFACT, download_dir / "windows")
        assets = assemble(identity, args.source_archive, args.windows_executable, args.output_dir)
        if args.publish:
            publish_draft(args.repo, identity, assets)
        print(f"Release assets assembled in {args.output_dir}")
        return 0
    finally:
        if temporary is not None:
            temporary.cleanup()


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError, ValueError, subprocess.CalledProcessError, TimeoutError) as error:
        print(f"ERROR: {error}", file=sys.stderr)
        raise SystemExit(2)
