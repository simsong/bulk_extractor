#!/usr/bin/env bash
# Assemble a release in an isolated worktree. Invoked only by `make release`.
set -euo pipefail

source_dir=${RELEASE_SOURCE_DIR:?make release must set RELEASE_SOURCE_DIR}
artifact_dir=${RELEASE_ARTIFACT_DIR:?make release must set RELEASE_ARTIFACT_DIR}
required_artifacts=(RELEASE_WINDOWS_EXE RELEASE_DEB RELEASE_RPM RELEASE_AWS_RESULT)

for variable in "${required_artifacts[@]}"; do
    value=${!variable:-}
    if [[ -z $value || ! -s $value ]]; then
        printf 'ERROR: %s must name a non-empty validated artifact\n' "$variable" >&2
        exit 2
    fi
done

if [[ -e $artifact_dir ]]; then
    printf 'ERROR: release artifact directory already exists: %s\n' "$artifact_dir" >&2
    exit 2
fi

release_root=$(mktemp -d "${TMPDIR:-/tmp}/bulk-extractor-release.XXXXXX")
release_src="$release_root/source"
log_dir="$artifact_dir/logs"

cleanup() {
    git -C "$source_dir" worktree remove --force "$release_src" 2>/dev/null || true
    rm -rf "$release_root"
}
trap cleanup EXIT

run_logged() {
    local name=$1
    shift
    printf '==> %s\n' "$name"
    "$@" 2>&1 | tee "$log_dir/$name.log"
}

git -C "$source_dir" diff --quiet || {
    printf 'ERROR: tracked changes in the source checkout; commit them before release\n' >&2
    exit 2
}
git -C "$source_dir" diff --cached --quiet || {
    printf 'ERROR: staged changes in the source checkout; commit them before release\n' >&2
    exit 2
}

mkdir -p "$log_dir"
git -C "$source_dir" worktree add --detach "$release_src" HEAD

run_logged bootstrap bash -c 'cd "$1" && bash bootstrap.sh' _ "$release_src"
run_logged configure bash -c 'cd "$1" && ./configure --quiet' _ "$release_src"
run_logged macos-distcheck make -C "$release_src" distcheck
run_logged linux-container-distcheck make -C "$release_src" distcheck-containers

archive=$(find "$release_src" -maxdepth 1 -type f -name 'bulk_extractor-*.tar.gz' -print -quit)
if [[ -z $archive ]]; then
    printf 'ERROR: distcheck did not produce a source archive\n' >&2
    exit 1
fi

cp "$archive" "$artifact_dir/"
cp "$RELEASE_WINDOWS_EXE" "$artifact_dir/bulk_extractor64.exe"
cp "$RELEASE_DEB" "$artifact_dir/"
cp "$RELEASE_RPM" "$artifact_dir/"
cp "$RELEASE_AWS_RESULT" "$artifact_dir/"

{
    printf 'source_commit=%s\n' "$(git -C "$release_src" rev-parse HEAD)"
    printf 'created_utc=%s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)"
    printf 'artifact_directory=%s\n' "$artifact_dir"
} > "$artifact_dir/RELEASE_METADATA.txt"

(cd "$artifact_dir" && shasum -a 256 \
    "$(basename "$archive")" bulk_extractor64.exe \
    "$(basename "$RELEASE_DEB")" "$(basename "$RELEASE_RPM")" \
    "$(basename "$RELEASE_AWS_RESULT")" RELEASE_METADATA.txt > SHA256SUMS)

printf 'Release artifacts staged in %s\n' "$artifact_dir"
