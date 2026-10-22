#!/usr/bin/env python3
"""Emit the android.yml build matrix as a JSON 'include' list.

detect_changes.py decides whether the event builds Android at all; when it
doesn't, the list is empty and `has_jobs` is false.

The windows leg is skipped on pull requests: it is the PR long pole by a
decent amount, and an Android/Windows host build adds little PR signal beyond
the linux Android leg. It still runs on push, merge queue, and manual
dispatch.

Runner selection stays in the workflow: each leg carries `runson_runner`
(RunsOn runner name, empty for GitHub-hosted-only legs) and `fallback_runner`,
combined by the job's `runs-on` expression.
"""

from __future__ import annotations

import argparse
import json
import os
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import _bootstrap as _bootstrap
from common.gh_actions import write_github_output
from qgc_tools.release_tag_check import GitQueryError

from ci.planning.detect_changes import detect_builds, report_tag_query_error

Leg = dict[str, str | bool]

LINUX_JOB: Leg = {
    "host": "linux",
    "arch": "linux_gcc_64",
    "shell": "bash",
    "primary": True,
    "emulator": False,
    "runson_runner": "linux-x64-builder",
    "fallback_runner": "ubuntu-latest",
}

MAC_JOB: Leg = {
    "host": "mac",
    "arch": "clang_64",
    "shell": "bash",
    "primary": False,
    "emulator": False,
    "runson_runner": "",
    "fallback_runner": "macos-15",
}

WINDOWS_JOB: Leg = {
    "host": "windows",
    "arch": "win64_msvc2022_64",
    "shell": "pwsh",
    "primary": False,
    "emulator": False,
    "runson_runner": "windows-x64-builder",
    "fallback_runner": "windows-2022",
}

LINUX_EMULATOR_JOB: Leg = {
    "host": "linux-emulator",
    "qt_host": "linux",
    "arch": "linux_gcc_64",
    "shell": "bash",
    "primary": False,
    "emulator": True,
    "runson_runner": "linux-x64-emulator",
    "fallback_runner": "ubuntu-latest",
}


def build_matrix(is_pr: bool) -> list[Leg]:
    """Return the matrix include-list for the given event type."""
    legs = [LINUX_JOB, MAC_JOB, WINDOWS_JOB, LINUX_EMULATOR_JOB]
    if is_pr:
        legs.remove(WINDOWS_JOB)
    matrix = []
    for leg in legs:
        if leg["emulator"]:
            abis = ["x86_64"]
        elif leg["primary"] and not is_pr:
            abis = ["arm64-v8a", "armeabi-v7a"]
        else:
            abis = ["arm64-v8a"]
        matrix.append(
            {
                **leg,
                "android_abis": ";".join(abis),
                "artifact_abi_suffix": "-".join(sorted(abis)),
            }
        )
    return matrix


def main(argv: list[str] | None = None) -> int:
    argparse.ArgumentParser(description=__doc__).parse_args(argv)
    try:
        should_build = detect_builds(["android"]).builds["android"]
    except GitQueryError as exc:
        return report_tag_query_error(exc)

    include = build_matrix(os.environ.get("EVENT_NAME") == "pull_request") if should_build else []
    serialized = json.dumps(include, separators=(",", ":"))
    write_github_output({"include": serialized, "has_jobs": "true" if include else "false"})
    print(f"include={serialized}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
