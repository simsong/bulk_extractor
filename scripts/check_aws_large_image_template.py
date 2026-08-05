#!/usr/bin/env python3
"""Check the non-negotiable safety properties of the AWS release gate."""

import json
import subprocess
import sys
from pathlib import Path


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def main() -> None:
    if len(sys.argv) != 2:
        raise SystemExit(f"usage: {Path(sys.argv[0]).name} TEMPLATE.json")
    template = json.loads(Path(sys.argv[1]).read_text())
    resources = template["Resources"]
    instance = resources["ValidationInstance"]["Properties"]
    volume = resources["ScratchVolume"]["Properties"]
    bucket = resources["ResultBucket"]["Properties"]
    user_data = instance["UserData"]["Fn::Base64"]["Fn::Sub"]

    require(instance["InstanceType"] == "c6i.2xlarge", "instance type is no longer cost-bounded")
    require(instance["InstanceInitiatedShutdownBehavior"] == "terminate", "instance shutdown must terminate")
    require(resources["ValidationWaitCondition"]["Properties"]["Timeout"] == "28800", "runtime cap must be 8 hours")
    require(volume["Encrypted"] is True, "scratch volume must be encrypted")
    require(bucket["BucketEncryption"]["ServerSideEncryptionConfiguration"][0]["ServerSideEncryptionByDefault"]["SSEAlgorithm"] == "AES256", "result bucket must encrypt by default")
    require(all(bucket["PublicAccessBlockConfiguration"].values()), "result bucket must block public access")
    require("ExpirationInDays" in bucket["LifecycleConfiguration"]["Rules"][0], "result artifacts need lifecycle cleanup")
    require("BudgetName" in resources["ComputeBudget"]["Properties"]["Budget"], "compute budget needs a unique name")
    require("set -euo pipefail" in user_data, "instance setup must fail fast")
    require("signal_status=FAILURE" in user_data and "signal_status=SUCCESS" in user_data, "wait condition must report failures accurately")
    require("summary_upload_failed" in user_data, "summary upload failure must fail the validation")
    for script in ("aws_large_image_worker.sh", "release_aws_large_image.sh"):
        subprocess.run(["bash", "-n", str(Path(__file__).with_name(script))], check=True)
    print("AWS large-image template safety checks passed")


if __name__ == "__main__":
    main()
