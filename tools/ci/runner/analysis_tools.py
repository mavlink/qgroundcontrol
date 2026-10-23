#!/usr/bin/env python3
"""Put the analysis.yml job's analysis executables on PATH for its later steps.

LLVM's bin directory goes on PATH first. clang-tidy installs from the PyPI wheel pinned by
version and SHA-256 and goes ahead of it, after verifying .clang-tidy; Clazy comes from the
tools/setup/build_clazy.py prefix. Clazy and clang-scan-deps stay on the compiler's LLVM.
The LLVM version and clang-tidy pins default to build-config.json's `analysis` section and the
tools to the step's ANALYSIS_TOOLS (space-separated).
"""

from __future__ import annotations

import argparse
import os
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import _bootstrap as _bootstrap
from common.build_config import get_build_config_value
from common.gh_actions import append_github_path, env_default, gh_error

CLAZY_PREFIX = Path(".cache/clazy")


def install_clang_tidy(version: str, sha256: str, runner_temp: Path) -> Path:
    """Install the hash-pinned clang-tidy wheel under runner_temp and return its bin directory."""
    requirements = runner_temp / "clang-tidy-requirements.txt"
    requirements.write_text(f"clang-tidy=={version} --hash=sha256:{sha256}\n", encoding="utf-8")
    target = runner_temp / "clang-tidy"
    subprocess.run(
        [
            "uv",
            "pip",
            "install",
            "--quiet",
            "--require-hashes",
            "--no-deps",
            "--only-binary",
            ":all:",
            "--target",
            str(target),
            "-r",
            str(requirements),
        ],
        check=True,
    )
    return target / "clang_tidy" / "data" / "bin"


def select_tools(
    tools: list[str],
    *,
    llvm_version: str,
    clang_tidy_version: str,
    clang_tidy_sha256: str,
    runner_temp: Path,
    workspace: Path,
) -> None:
    """Verify each requested tool and add its directory to $GITHUB_PATH, LLVM's first."""
    append_github_path(f"/usr/lib/llvm-{llvm_version}/bin")
    if "clang-tidy" in tools:
        tidy_bin = install_clang_tidy(clang_tidy_version, clang_tidy_sha256, runner_temp)
        clang_tidy = str(tidy_bin / "clang-tidy")
        subprocess.run([clang_tidy, "--version"], check=True)
        subprocess.run(
            [clang_tidy, f"--config-file={workspace / '.clang-tidy'}", "--verify-config"],
            check=True,
        )
        append_github_path(str(tidy_bin))
    if "clazy" in tools:
        clazy_bin = workspace / CLAZY_PREFIX / "bin"
        append_github_path(str(clazy_bin))
        subprocess.run([str(clazy_bin / "clazy-standalone"), "--version"], check=True)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--llvm-version", default=get_build_config_value("analysis.llvm_version"))
    parser.add_argument("--tools", default=env_default("ANALYSIS_TOOLS"))
    parser.add_argument(
        "--clang-tidy-version", default=get_build_config_value("analysis.clang_tidy_version")
    )
    parser.add_argument(
        "--clang-tidy-sha256", default=get_build_config_value("analysis.clang_tidy_sha256")
    )
    args = parser.parse_args(argv)

    tools = args.tools.split()
    required = {"analysis.llvm_version": args.llvm_version}
    if "clang-tidy" in tools:
        required |= {
            "analysis.clang_tidy_version": args.clang_tidy_version,
            "analysis.clang_tidy_sha256": args.clang_tidy_sha256,
        }
    missing = [name for name, value in required.items() if not value]
    if missing:
        gh_error(f"Missing analysis toolchain inputs: {', '.join(missing)}")
        return 1
    try:
        select_tools(
            tools,
            llvm_version=args.llvm_version,
            clang_tidy_version=args.clang_tidy_version,
            clang_tidy_sha256=args.clang_tidy_sha256,
            runner_temp=Path(env_default("RUNNER_TEMP", os.curdir)),
            workspace=Path(env_default("GITHUB_WORKSPACE", os.curdir)),
        )
    except (OSError, subprocess.CalledProcessError) as error:
        gh_error(f"Selecting analysis executables failed: {error}")
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
