#!/usr/bin/env bash
# Launch and monitor the AWS release-performance matrix. No CloudFormation or
# GitHub Actions resources are involved.
set -euo pipefail

AWS_REGION=${AWS_REGION:-us-east-1}
INSTANCE_TYPES=(${INSTANCE_TYPES:-m7i.xlarge m7i.4xlarge}) # 4 and 16 vCPUs
IMAGE_URLS=(${IMAGE_URLS:-
https://downloads.digitalcorpora.org/corpora/drives/nps-2009-ubnist1/ubnist1.gen3.raw
https://downloads.digitalcorpora.org/corpora/drives/nps-2009-domexusers/nps-2009-domexusers.E01})
RESULT_BUCKET=${RESULT_BUCKET:-}
BE_VERSION=${BE_VERSION:-2.2.0}
BE_RELEASE_URL=${BE_RELEASE_URL:-https://github.com/simsong/bulk_extractor/releases/download/v${BE_VERSION}/bulk_extractor-${BE_VERSION}.tar.gz}
AMI_ID=${AMI_ID:-}
SUBNET_ID=${SUBNET_ID:-}
SECURITY_GROUP_ID=${SECURITY_GROUP_ID:-}
SSH_KEY_NAME=${SSH_KEY_NAME:-}
SSH_PRIVATE_KEY=${SSH_PRIVATE_KEY:-}
INSTANCE_PROFILE=${INSTANCE_PROFILE:-}
SSH_USER=${SSH_USER:-ubuntu}
POLL_SECONDS=${POLL_SECONDS:-15}
LOG_PATH=/var/log/bulk-extractor-release.log

usage() {
    cat <<'EOF'
Usage: make release-aws-large-image RESULT_BUCKET=example-results \
  SECURITY_GROUP_ID=sg-... SSH_KEY_NAME=my-key \
  SSH_PRIVATE_KEY=/path/to/my-key.pem INSTANCE_PROFILE=be-release-runner

Launches every INSTANCE_TYPES x IMAGE_URLS combination. Defaults are
m7i.xlarge (4 vCPUs), m7i.4xlarge (16 vCPUs), and the public ubnist1 and
domexusers Digital Corpora images. Override the whitespace-separated lists,
AWS_REGION, RESULT_BUCKET, BE_VERSION, BE_RELEASE_URL, or AMI_ID as needed.

Without SUBNET_ID, the script chooses a default subnet in the account's default
VPC; it must assign public IPv4 addresses. The security group must permit SSH
from this operator. The caller's IAM policy must allow EC2 launch/describe and
S3 writes to RESULT_BUCKET; INSTANCE_PROFILE must grant the instances S3
PutObject permission on that bucket. This script leaves
instances stopped after completion; terminate them when their evidence is no
longer needed.
EOF
}

[[ ${1:-} != --help ]] || { usage; exit 0; }
for command in aws ssh tput; do command -v "$command" >/dev/null || { echo "ERROR: $command is required" >&2; exit 2; }; done
for value in RESULT_BUCKET SECURITY_GROUP_ID SSH_KEY_NAME SSH_PRIVATE_KEY INSTANCE_PROFILE; do
    [[ -n ${!value} ]] || { echo "ERROR: $value is required; see --help" >&2; exit 2; }
done
[[ -r $SSH_PRIVATE_KEY ]] || { echo "ERROR: SSH_PRIVATE_KEY is not readable: $SSH_PRIVATE_KEY" >&2; exit 2; }
if [[ -z $SUBNET_ID ]]; then
    default_vpc=$(aws ec2 describe-vpcs --region "$AWS_REGION" --filters Name=is-default,Values=true \
        --query 'Vpcs[0].VpcId' --output text)
    [[ $default_vpc != None ]] || { echo "ERROR: no default VPC; set SUBNET_ID" >&2; exit 2; }
    SUBNET_ID=$(aws ec2 describe-subnets --region "$AWS_REGION" --filters "Name=vpc-id,Values=$default_vpc" \
        Name=default-for-az,Values=true --query 'Subnets[0].SubnetId' --output text)
    [[ $SUBNET_ID != None ]] || { echo "ERROR: default VPC has no default subnet; set SUBNET_ID" >&2; exit 2; }
    echo "Using default subnet $SUBNET_ID in $default_vpc"
fi
aws ec2 describe-subnets --region "$AWS_REGION" --subnet-ids "$SUBNET_ID" >/dev/null
aws s3api head-bucket --bucket "$RESULT_BUCKET" >/dev/null
if [[ -z $AMI_ID ]]; then
    AMI_ID=$(aws ssm get-parameter --region "$AWS_REGION" \
        --name /aws/service/canonical/ubuntu/server/24.04/stable/current/amd64/hvm/ebs-gp3/ami-id \
        --query Parameter.Value --output text)
fi

declare -a INSTANCE_IDS INSTANCE_TYPES_BY_ID IMAGE_NAMES START_TIMES RESULT_KEYS
declare -A COMPLETE ELAPSED
RUN_TIMESTAMP=$(date -u +%Y-%m-%dT%H:%M:%SZ)

shell_quote() { printf '%q' "$1"; }

write_user_data() {
    local destination=$1 instance_type=$2 image_url=$3 image_name=$4 result_key=$5
    {
        printf '#!/usr/bin/env bash\nset -Eeuo pipefail\n'
        printf 'INSTANCE_TYPE=%s\n' "$(shell_quote "$instance_type")"
        printf 'IMAGE_URL=%s\n' "$(shell_quote "$image_url")"
        printf 'IMAGE_NAME=%s\n' "$(shell_quote "$image_name")"
        printf 'RESULT_BUCKET=%s\n' "$(shell_quote "$RESULT_BUCKET")"
        printf 'RESULT_KEY=%s\n' "$(shell_quote "$result_key")"
        printf 'BE_VERSION=%s\n' "$(shell_quote "$BE_VERSION")"
        printf 'BE_RELEASE_URL=%s\n' "$(shell_quote "$BE_RELEASE_URL")"
        cat <<'EOF'
LOG=/var/log/bulk-extractor-release.log
WORK=/opt/bulk-extractor-release
STATUS=FAIL
REASON=not_started
STARTED=$(date -u +%Y-%m-%dT%H:%M:%SZ)
SECONDS=0
mkdir -p "$WORK"
exec > >(tee -a "$LOG") 2>&1
finish() {
    local archive=
    set +e
    if [[ $STATUS == PASS ]]; then
        archive="${RESULT_KEY%.txt}.zip"
        (cd "$WORK" && zip -qr "$archive" output)
        aws s3 cp "$WORK/$archive" "s3://$RESULT_BUCKET/$archive" --only-show-errors
    fi
    cat > "$WORK/result.txt" <<RESULT
status=$STATUS
reason=$REASON
instance_type=$INSTANCE_TYPE
image_url=$IMAGE_URL
started_utc=$STARTED
finished_utc=$(date -u +%Y-%m-%dT%H:%M:%SZ)
elapsed_seconds=$SECONDS
archive=${archive##*/}
RESULT
    aws s3 cp "$WORK/result.txt" "s3://$RESULT_BUCKET/$RESULT_KEY" --only-show-errors || true
    sync
    shutdown -h now
}
trap finish EXIT
echo "BE_STAGE=installing"
export DEBIAN_FRONTEND=noninteractive
apt-get update
apt-get install -y awscli build-essential pkg-config libxml2-dev libssl-dev libewf-dev \
    libafflib-dev libboost-all-dev libexpat1-dev libcurl4-openssl-dev libsqlite3-dev \
    zlib1g-dev liblzma-dev flex bison zip curl ca-certificates
echo "BE_STAGE=building"
cd "$WORK"
curl --fail --location --retry 3 "$BE_RELEASE_URL" -o source.tar.gz
tar -xzf source.tar.gz
source_directory=$(find . -maxdepth 1 -type d -name 'bulk_extractor-*' | head -n 1)
[[ -n $source_directory ]] || { REASON=source_unpack_failed; exit 1; }
cd "$source_directory"
./configure --quiet
make -j"$(nproc)"
make install
echo "BE_STAGE=downloading"
cd "$WORK"
curl --fail --location --retry 3 "$IMAGE_URL" -o "$IMAGE_NAME"
echo "BE_STAGE=scanning"
set +e
bulk_extractor -q -o "$WORK/output" "$WORK/$IMAGE_NAME" 2>&1 | tee -a "$LOG"
scan_status=${PIPESTATUS[0]}
set -e
[[ $scan_status -eq 0 ]] || { REASON=bulk_extractor_failed; exit 1; }
STATUS=PASS
REASON=completed
echo "BE_STAGE=uploading"
EOF
    } > "$destination"
}

launch() {
    local instance_type=$1 image_url=$2 image_name result_base user_data instance_id
    image_name=${image_url##*/}
    image_name=${image_name%.gz}
    image_name=${image_name//[^A-Za-z0-9._-]/_}
    result_base="BE${BE_VERSION}-${instance_type}-${image_name}-${RUN_TIMESTAMP}"
    user_data=$(mktemp)
    write_user_data "$user_data" "$instance_type" "$image_url" "$image_name" "${result_base}.txt"
    instance_id=$(aws ec2 run-instances --region "$AWS_REGION" --image-id "$AMI_ID" \
        --instance-type "$instance_type" --key-name "$SSH_KEY_NAME" --subnet-id "$SUBNET_ID" \
        --security-group-ids "$SECURITY_GROUP_ID" --user-data "file://$user_data" \
        --iam-instance-profile "Name=$INSTANCE_PROFILE" \
        --tag-specifications 'ResourceType=instance,Tags=[{Key=Name,Value=bulk-extractor-release-validation}]' \
        --query 'Instances[0].InstanceId' --output text)
    rm -f "$user_data"
    INSTANCE_IDS+=("$instance_id")
    INSTANCE_TYPES_BY_ID+=("$instance_type")
    IMAGE_NAMES+=("$image_name")
    START_TIMES+=("$(date +%s)")
    RESULT_KEYS+=("${result_base}.txt")
    COMPLETE["$instance_id"]=0
    echo "Launched $instance_id: $instance_type / $image_name"
}

for instance_type in "${INSTANCE_TYPES[@]}"; do
    for image_url in "${IMAGE_URLS[@]}"; do launch "$instance_type" "$image_url"; done
done

progress_bar() {
    local percentage=$1 filled=$(( ${1%.*} / 5 )) bar
    printf -v bar '%*s' "$filled" ''
    bar=${bar// /#}
    printf '[%-20s] %3.0f%%' "$bar" "$percentage"
}

render() {
    local index id state address log percentage elapsed cpu bar
    tput clear; tput cup 0 0
    printf '%-20s %5s %-24s %-10s %s\n' INSTANCE CPU IMAGE STATE PROGRESS
    for index in "${!INSTANCE_IDS[@]}"; do
        id=${INSTANCE_IDS[$index]}
        state=$(aws ec2 describe-instances --region "$AWS_REGION" --instance-ids "$id" --query 'Reservations[0].Instances[0].State.Name' --output text)
        address=$(aws ec2 describe-instances --region "$AWS_REGION" --instance-ids "$id" --query 'Reservations[0].Instances[0].PublicIpAddress' --output text)
        log=
        if [[ $address != None && $state == running ]]; then
            log=$(ssh -i "$SSH_PRIVATE_KEY" -o BatchMode=yes -o ConnectTimeout=4 -o StrictHostKeyChecking=accept-new \
                "$SSH_USER@$address" "sudo tail -n 100 '$LOG_PATH'" 2>/dev/null || true)
        fi
        percentage=$(printf '%s\n' "$log" | sed -nE 's/.*\(([0-9]+(\.[0-9]+)?)%\).*/\1/p' | tail -n 1)
        [[ -n $percentage ]] || percentage=0
        [[ $state == stopped ]] && { COMPLETE["$id"]=1; percentage=100; }
        elapsed=$(( $(date +%s) - ${START_TIMES[$index]} ))
        cpu=$(aws ec2 describe-instance-types --region "$AWS_REGION" --instance-types "${INSTANCE_TYPES_BY_ID[$index]}" --query 'InstanceTypes[0].VCpuInfo.DefaultVCpus' --output text)
        bar=$(progress_bar "$percentage")
        printf '%-20s %5s %-24s %-10s %s %4ss\n' "$id" "$cpu" "${IMAGE_NAMES[$index]}" "$state" "$bar" "$elapsed"
    done
}

while :; do
    render
    all_complete=true
    for id in "${INSTANCE_IDS[@]}"; do [[ ${COMPLETE[$id]} == 1 ]] || { all_complete=false; break; }; done
    $all_complete && break
    sleep "$POLL_SECONDS"
done
tput sgr0; printf '\nCompletion report\n'
for index in "${!INSTANCE_IDS[@]}"; do
    id=${INSTANCE_IDS[$index]}; result=$(mktemp)
    aws s3 cp "s3://$RESULT_BUCKET/${RESULT_KEYS[$index]}" "$result" --only-show-errors || true
    printf '%s %s %s\n' "$id" "${INSTANCE_TYPES_BY_ID[$index]}" "${IMAGE_NAMES[$index]}"
    [[ -s $result ]] && sed 's/^/  /' "$result" || echo '  result text was not uploaded'
    rm -f "$result"
done
