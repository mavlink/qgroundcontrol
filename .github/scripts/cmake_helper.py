#!/usr/bin/env python3
"""CMake build and configure helpers for CI.

Subcommands:
    build       Run cmake --build with CI-friendly output and timing
    configure   Run configure.py with standardized arguments
    detect-jobs Detect number of parallel jobs for the current platform
    ctest       Run CTest with standardized arguments and timing
"""

from __future__ import annotations

import argparse
import os
import re
import subprocess
import sys
import time

from ci_bootstrap import ensure_tools_dir

ensure_tools_dir(__file__)

from common.gh_actions import append_github_env, write_github_output  # noqa: E402


def detect_jobs(requested: str = "auto") -> int:
    """Detect number of parallel jobs for the current platform."""
    if requested != "auto":
        if not re.match(r"^[1-9]\d*$", requested):
            print(f"::error::Invalid parallel job count: {requested}", file=sys.stderr)
            sys.exit(1)
        return int(requested)

    try:
        return os.cpu_count() or 2
    except Exception:
        return 2


def cmd_detect_jobs(args: argparse.Namespace) -> None:
    jobs = detect_jobs(args.parallel)
    write_github_output({"jobs": str(jobs)})


def cmd_build(args: argparse.Namespace) -> None:
    """Run cmake --build with CI-friendly output."""
    cmd = ["cmake", "--build", "."]

    if args.target:
        cmd += ["--target", args.target]
    if args.build_type:
        cmd += ["--config", args.build_type]
    if args.parallel:
        if args.parallel_jobs:
            if not re.match(r"^[1-9]\d*$", args.parallel_jobs):
                print(f"::error::parallel-jobs must be a positive integer, got '{args.parallel_jobs}'",
                      file=sys.stderr)
                sys.exit(1)
            cmd += ["--parallel", args.parallel_jobs]
        else:
            cmd.append("--parallel")

    output_file = args.output_file
    if args.reviewdog and not output_file:
        output_file = os.path.join(os.environ.get("RUNNER_TEMP", "."), "build-output.log")

    if output_file and args.reviewdog:
        append_github_env({"REVIEWDOG_LOG": output_file})

    print(f"Running: {' '.join(cmd)}")
    start = time.monotonic()

    if output_file:
        with open(output_file, "w") as log:
            proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
            for line in proc.stdout:
                sys.stdout.write(line)
                log.write(line)
            proc.wait()
            exit_code = proc.returncode
    else:
        result = subprocess.run(cmd, check=False)
        exit_code = result.returncode

    duration = int(time.monotonic() - start)

    if exit_code == 0:
        print(f"::notice::Build completed in {duration}s")
    else:
        print(f"::error::Build failed after {duration}s")
        if not args.continue_on_error:
            sys.exit(exit_code)


def cmd_configure(args: argparse.Namespace) -> None:
    """Run configure.py with standardized arguments."""
    workspace = os.environ.get("GITHUB_WORKSPACE", ".")
    cmd = [
        sys.executable, os.path.join(workspace, "tools", "configure.py"),
        "-S", args.source_dir,
        "-B", args.build_dir,
        "-G", args.generator,
        "-t", args.build_type,
    ]
    if args.testing:
        cmd.append("--testing")
    if args.coverage:
        cmd.append("--coverage")
    if args.stable:
        cmd.append("--stable")
    if not args.use_qt_cmake:
        cmd.append("--no-qt-cmake")
    if args.unity_build:
        cmd += ["--unity", "--unity-batch", args.unity_batch_size]
    if args.extra_args:
        cmd.append("--")
        cmd.extend(args.extra_args.split())

    start = time.monotonic()
    result = subprocess.run(cmd, check=False)
    duration = int(time.monotonic() - start)
    print(f"::notice::Configure completed in {duration}s")
    sys.exit(result.returncode)


def cmd_ctest(args: argparse.Namespace) -> None:
    """Run CTest with standardized arguments and timing."""
    cmd = [
        "ctest",
        "--output-on-failure",
        "--output-junit", args.junit_output,
        "--parallel", str(args.jobs),
    ]
    if args.include_labels:
        cmd += ["-L", args.include_labels]
    if args.exclude_labels:
        cmd += ["-LE", args.exclude_labels]

    start = time.monotonic()

    with open(args.ctest_output, "w") as log:
        proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        for line in proc.stdout:
            sys.stdout.write(line)
            log.write(line)
        proc.wait()
        exit_code = proc.returncode

    duration = int(time.monotonic() - start)
    print(f"::notice::Tests completed in {duration}s")
    sys.exit(exit_code)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)

    # detect-jobs
    p_jobs = sub.add_parser("detect-jobs")
    p_jobs.add_argument("--parallel", default="auto")

    # build
    p_build = sub.add_parser("build")
    p_build.add_argument("--target", default="")
    p_build.add_argument("--build-type", default="")
    p_build.add_argument("--parallel", action="store_true", default=False)
    p_build.add_argument("--parallel-jobs", default="")
    p_build.add_argument("--output-file", default="")
    p_build.add_argument("--continue-on-error", action="store_true", default=False)
    p_build.add_argument("--reviewdog", action="store_true", default=False)

    # configure
    p_conf = sub.add_parser("configure")
    p_conf.add_argument("--source-dir", required=True)
    p_conf.add_argument("--build-dir", required=True)
    p_conf.add_argument("--generator", default="Ninja")
    p_conf.add_argument("--build-type", default="Release")
    p_conf.add_argument("--testing", action="store_true", default=False)
    p_conf.add_argument("--coverage", action="store_true", default=False)
    p_conf.add_argument("--stable", action="store_true", default=False)
    p_conf.add_argument("--use-qt-cmake", action="store_true", default=True)
    p_conf.add_argument("--no-qt-cmake", action="store_false", dest="use_qt_cmake")
    p_conf.add_argument("--unity-build", action="store_true", default=False)
    p_conf.add_argument("--unity-batch-size", default="16")
    p_conf.add_argument("--extra-args", default="")

    # ctest
    p_ctest = sub.add_parser("ctest")
    p_ctest.add_argument("--junit-output", required=True)
    p_ctest.add_argument("--ctest-output", required=True)
    p_ctest.add_argument("--jobs", type=int, required=True)
    p_ctest.add_argument("--include-labels", default="")
    p_ctest.add_argument("--exclude-labels", default="")

    args = parser.parse_args()
    commands = {
        "detect-jobs": cmd_detect_jobs,
        "build": cmd_build,
        "configure": cmd_configure,
        "ctest": cmd_ctest,
    }
    commands[args.command](args)


if __name__ == "__main__":
    main()
