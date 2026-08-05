#!/usr/bin/env python3
"""Emit the android.yml build matrix as a JSON 'include' list.

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

from ci_bootstrap import ensure_tools_dir

ensure_tools_dir(__file__)

from common.gh_actions import parse_bool, write_github_output

Leg = dict[str, str | bool]

LINUX_JOB: Leg = {
    "host": "linux",
    "arch": "linux_gcc_64",
    "qt_host_path": "gcc_64",
    "shell": "bash",
    "primary": True,
    "emulator": False,
    "runson_runner": "linux-x64-builder",
    "fallback_runner": "ubuntu-latest",
}

MAC_JOB: Leg = {
    "host": "mac",
    "arch": "clang_64",
    "qt_host_path": "macos",
    "shell": "bash",
    "primary": False,
    "emulator": False,
    "runson_runner": "",
    "fallback_runner": "macos-15",
}

WINDOWS_JOB: Leg = {
    "host": "windows",
    "arch": "win64_msvc2022_64",
    "qt_host_path": "msvc2022_64",
    "shell": "pwsh",
    "primary": False,
    "emulator": False,
    "aqt_source": "git+https://github.com/miurahr/aqtinstall.git@8d961e61720b3c06583517fbc68d57ec0a4e9f95",
    "runson_runner": "windows-x64-builder",
    "fallback_runner": "windows-2022",
}

LINUX_EMULATOR_JOB: Leg = {
    "host": "linux-emulator",
    "qt_host": "linux",
    "arch": "linux_gcc_64",
    "qt_host_path": "gcc_64",
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
    return legs


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--is-pr",
        default=os.environ.get("IS_PR", "0"),
        help="'1'/'true' for PR builds (default from $IS_PR)",
    )
    args = parser.parse_args(argv)

    include = build_matrix(parse_bool(args.is_pr))
    serialized = json.dumps(include, separators=(",", ":"))
    write_github_output({"include": serialized})
    print(f"include={serialized}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
