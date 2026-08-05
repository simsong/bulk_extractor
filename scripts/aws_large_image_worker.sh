#!/usr/bin/env bash
# Run inside the disposable release-validation instance. Do not export scan output.
set -euo pipefail

result_file=${RESULT_FILE:?}
scratch_directory=${SCRATCH_DIRECTORY:?}
source_directory=${SOURCE_DIRECTORY:?}
status=FAIL
reason=not_started
output_files=0
started_utc=$(date -u +%Y-%m-%dT%H:%M:%SZ)

write_summary() {
    cat > "$result_file" <<EOF
status=$status
reason=$reason
started_utc=$started_utc
finished_utc=$(date -u +%Y-%m-%dT%H:%M:%SZ)
instance_type=c6i.2xlarge
max_runtime_minutes=$MAX_RUNTIME_MINUTES
source_sha256=$SOURCE_ARCHIVE_SHA256
approved_input_sha256=$INPUT_SHA256
output_file_count=$output_files
EOF
}

cleanup() {
    rm -rf "$scratch_directory/scan-output" "$scratch_directory/input.img" "$source_directory"
    write_summary
}
trap cleanup EXIT

fail() {
    reason=$1
    exit 1
}

input_encryption=$(aws s3api head-object --bucket "$INPUT_BUCKET" --key "$INPUT_KEY" \
    --query ServerSideEncryption --output text) || fail input_metadata_unavailable
[[ $input_encryption == AES256 ]] || fail input_must_use_s3_managed_encryption

aws s3 cp "s3://$INPUT_BUCKET/$INPUT_KEY" "$scratch_directory/input.img" --only-show-errors || fail input_download_failed
if ! actual_input_sha256=$(sha256sum "$scratch_directory/input.img" | awk '{print $1}'); then
    fail input_checksum_unavailable
fi
[[ ${actual_input_sha256,,} == ${INPUT_SHA256,,} ]] || fail input_checksum_mismatch
cd "$source_directory" || fail source_directory_missing
./configure --quiet --disable-libewf || fail configure_failed
make -j2 || fail build_failed
mkdir -p "$scratch_directory/scan-output"
./src/bulk_extractor -q -o "$scratch_directory/scan-output" "$scratch_directory/input.img" || fail scan_failed
output_files=$(find "$scratch_directory/scan-output" -type f | wc -l | tr -d ' ')
status=PASS
reason=validated
