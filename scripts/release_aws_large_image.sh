#!/usr/bin/env bash
# Provision, collect, and remove the release large-image validation stack.
set -euo pipefail

usage() {
    cat <<'EOF'
Usage: make release-aws-large-image \
  AWS_LARGE_IMAGE_STACK=bulk-extractor-release-2-2-0 \
  AWS_LARGE_IMAGE_RESULT=/absolute/path/summary.txt \
  AWS_LARGE_IMAGE_INPUT_BUCKET=approved-bucket \
  AWS_LARGE_IMAGE_INPUT_KEY=approved-image \
  AWS_LARGE_IMAGE_INPUT_SHA256=... \
  AWS_LARGE_IMAGE_AVAILABILITY_ZONE=us-east-1a \
  AWS_LARGE_IMAGE_SUBNET_ID=subnet-... AWS_LARGE_IMAGE_VPC_ID=vpc-... \
  AWS_LARGE_IMAGE_SOURCE_URL=https://github.com/.../bulk_extractor-X.tar.gz \
  AWS_LARGE_IMAGE_SOURCE_SHA256=... \
  AWS_LARGE_IMAGE_BUDGET_EMAIL=release-manager@example.org

The target creates one c6i.2xlarge instance for at most eight hours, retrieves
only a redacted summary, and deletes the stack, volume, bucket, and logs.
EOF
}

[[ ${1:-} != --help ]] || { usage; exit 0; }
template=${AWS_LARGE_IMAGE_TEMPLATE:?make must set AWS_LARGE_IMAGE_TEMPLATE}
stack=${AWS_LARGE_IMAGE_STACK:?set AWS_LARGE_IMAGE_STACK to a new stack name}
result=${AWS_LARGE_IMAGE_RESULT:?set AWS_LARGE_IMAGE_RESULT to a local summary path}
required=(AWS_LARGE_IMAGE_INPUT_BUCKET AWS_LARGE_IMAGE_INPUT_KEY AWS_LARGE_IMAGE_INPUT_SHA256 AWS_LARGE_IMAGE_SOURCE_URL AWS_LARGE_IMAGE_SOURCE_SHA256 AWS_LARGE_IMAGE_BUDGET_EMAIL AWS_LARGE_IMAGE_AVAILABILITY_ZONE AWS_LARGE_IMAGE_SUBNET_ID AWS_LARGE_IMAGE_VPC_ID)
command -v aws >/dev/null || { echo "ERROR: AWS CLI is required" >&2; exit 2; }
[[ -r $template ]] || { echo "ERROR: template not readable: $template" >&2; exit 2; }
for variable in "${required[@]}"; do
    [[ -n ${!variable:-} ]] || { echo "ERROR: $variable is required" >&2; exit 2; }
done
[[ $AWS_LARGE_IMAGE_INPUT_SHA256 =~ ^[[:xdigit:]]{64}$ ]] || { echo "ERROR: input checksum must be SHA-256" >&2; exit 2; }
[[ $AWS_LARGE_IMAGE_SOURCE_SHA256 =~ ^[[:xdigit:]]{64}$ ]] || { echo "ERROR: source checksum must be SHA-256" >&2; exit 2; }
[[ $AWS_LARGE_IMAGE_SOURCE_URL == https://* ]] || { echo "ERROR: source URL must use HTTPS" >&2; exit 2; }
if aws cloudformation describe-stacks --stack-name "$stack" >/dev/null 2>&1; then
    echo "ERROR: refusing to reuse existing stack $stack" >&2
    exit 2
fi

stack_created=false
result_bucket=
summary_key=
cleanup() {
    [[ $stack_created == true ]] || return
    if [[ -n $result_bucket && -n $summary_key ]]; then
        aws s3 rm "s3://$result_bucket/$summary_key" --only-show-errors || true
    fi
    aws cloudformation delete-stack --stack-name "$stack" || true
    aws cloudformation wait stack-delete-complete --stack-name "$stack" || true
}
trap cleanup EXIT

stack_created=true
aws cloudformation deploy --stack-name "$stack" --template-file "$template" \
    --capabilities CAPABILITY_NAMED_IAM \
    --parameter-overrides \
        InputBucket="$AWS_LARGE_IMAGE_INPUT_BUCKET" \
        InputKey="$AWS_LARGE_IMAGE_INPUT_KEY" \
        InputSha256="$AWS_LARGE_IMAGE_INPUT_SHA256" \
        AvailabilityZone="$AWS_LARGE_IMAGE_AVAILABILITY_ZONE" \
        SubnetId="$AWS_LARGE_IMAGE_SUBNET_ID" \
        VpcId="$AWS_LARGE_IMAGE_VPC_ID" \
        SourceArchiveUrl="$AWS_LARGE_IMAGE_SOURCE_URL" \
        SourceArchiveSha256="$AWS_LARGE_IMAGE_SOURCE_SHA256" \
        BudgetAlertEmail="$AWS_LARGE_IMAGE_BUDGET_EMAIL"
result_bucket=$(aws cloudformation describe-stacks --stack-name "$stack" --query "Stacks[0].Outputs[?OutputKey=='ResultBucket'].OutputValue | [0]" --output text)
summary_key=$(aws cloudformation describe-stacks --stack-name "$stack" --query "Stacks[0].Outputs[?OutputKey=='SummaryKey'].OutputValue | [0]" --output text)
mkdir -p "$(dirname "$result")"
aws s3 cp "s3://$result_bucket/$summary_key" "$result" --only-show-errors
grep -qx 'status=PASS' "$result" || { echo "ERROR: large-image validation failed" >&2; exit 1; }
echo "AWS large-image validation completed; redacted summary: $result"
