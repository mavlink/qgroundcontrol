"""CMake build and configure helpers for CI.

Subcommands:
    build       Run cmake --build with CI-friendly output and timing
    configure   Configure from a CMake preset with CI-specific overrides
    detect-jobs Detect number of parallel jobs for the current platform
    ctest       Run CTest with standardized arguments and timing
    cache-var   Read a CMake cache variable from CMakeCache.txt
"""

from __future__ import annotations

import argparse
import os
import re
import shlex
import shutil
import subprocess
import sys
import time

from ci_bootstrap import ensure_tools_dir

ensure_tools_dir(__file__)

from common.cmake import read_cache_var
from common.gh_actions import append_github_env, gh_error, gh_notice, write_github_output
from common.proc import run_tee


def detect_jobs(requested: str = "auto") -> int:
    """Detect number of parallel jobs for the current platform."""
    if requested != "auto":
        if not re.match(r"^[1-9]\d*$", requested):
            gh_error(f"Invalid parallel job count: {requested}")
            sys.exit(1)
        return int(requested)

    return os.cpu_count() or 2


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
                gh_error(f"parallel-jobs must be a positive integer, got '{args.parallel_jobs}'")
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
        exit_code = run_tee(cmd, output_file)
    else:
        result = subprocess.run(cmd, check=False)
        exit_code = result.returncode

    duration = int(time.monotonic() - start)

    if exit_code == 0:
        gh_notice(f"Build completed in {duration}s")
    else:
        gh_error(f"Build failed after {duration}s")
        if not args.continue_on_error:
            sys.exit(exit_code)


def split_extra_args(extra_args: str, *, windows: bool | None = None) -> list[str]:
    """Split CMake arguments without consuming Windows path separators."""
    if windows is None:
        windows = os.name == "nt"
    if windows:
        extra_args = extra_args.replace("\\", "\\\\")
    return shlex.split(extra_args)


def cmd_configure(args: argparse.Namespace) -> None:
    """Configure from a preset, retaining the legacy wrapper as a fallback."""
    workspace = os.environ.get("GITHUB_WORKSPACE", ".")
    if args.preset:
        cmd = [
            "cmake",
            "--preset",
            args.preset,
            "-S",
            args.source_dir,
            "-B",
            args.build_dir,
        ]
        if args.generator:
            cmd += ["-G", args.generator]
        if args.stable:
            cmd.append("-DQGC_STABLE_BUILD=ON")
        if args.extra_args:
            cmd.extend(split_extra_args(args.extra_args))

        start = time.monotonic()
        result = subprocess.run(cmd, check=False)
        duration = int(time.monotonic() - start)
        gh_notice(f"Configure completed in {duration}s")
        sys.exit(result.returncode)

    cmd = [
        sys.executable,
        os.path.join(workspace, "tools", "configure.py"),
        "-S",
        args.source_dir,
        "-B",
        args.build_dir,
        "-G",
        args.generator or "Ninja",
        "-t",
        args.build_type,
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
        cmd.extend(split_extra_args(args.extra_args))

    start = time.monotonic()
    result = subprocess.run(cmd, check=False)
    duration = int(time.monotonic() - start)
    gh_notice(f"Configure completed in {duration}s")
    sys.exit(result.returncode)


def _maybe_wrap_xvfb(cmd: list[str]) -> list[str]:
    """Prefix `xvfb-run` on headless Linux so the offscreen plugin's GLX path
    gets a display and QRhiGles2 can create a software GL context."""
    if sys.platform.startswith("linux") and not os.environ.get("DISPLAY"):
        xvfb = shutil.which("xvfb-run")
        if xvfb:
            print("::notice::No DISPLAY; running CTest under xvfb-run")
            return [xvfb, "-a", *cmd]
        print("::warning::xvfb-run not found; tests run without a GL context")
    return cmd


def cmd_ctest(args: argparse.Namespace) -> None:
    """Run CTest with standardized arguments and timing."""
    cmd = [
        "ctest",
        "--output-on-failure",
        "--output-junit",
        args.junit_output,
        "--parallel",
        str(args.jobs),
    ]
    if args.include_labels:
        cmd += ["-L", args.include_labels]
    if args.exclude_labels:
        cmd += ["-LE", args.exclude_labels]
    if args.repeat:
        cmd += ["--repeat", args.repeat]
    if args.shard_count and args.shard_count > 1:
        if args.shard_index < 0 or args.shard_index >= args.shard_count:
            gh_error(
                f"shard-index must be between 0 and {args.shard_count - 1}, got {args.shard_index}"
            )
            sys.exit(1)
        # An omitted end means the last test; an explicit 0 disables the range.
        start_idx = args.shard_index + 1
        cmd += ["-I", f"{start_idx},,{args.shard_count}"]

    start = time.monotonic()
    exit_code = run_tee(_maybe_wrap_xvfb(cmd), args.ctest_output)
    duration = int(time.monotonic() - start)
    gh_notice(f"Tests completed in {duration}s")
    sys.exit(exit_code)


def cmd_cache_var(args: argparse.Namespace) -> None:
    cache_path = os.path.join(args.build_dir, "CMakeCache.txt")
    value = read_cache_var(cache_path, args.name)
    if value is None:
        if args.default is not None:
            value = args.default
        elif args.required:
            gh_error(f"CMake cache variable {args.name} not found in {cache_path}")
            sys.exit(1)
        else:
            value = ""
    print(value)
    write_github_output({args.output_key or args.name.lower(): value})


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
    p_conf.add_argument("--preset", default="")
    p_conf.add_argument("--generator", default="")
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
    p_ctest.add_argument(
        "--repeat",
        default="",
        metavar="SPEC",
        help="CTest --repeat spec (e.g. until-fail:5). Optional; absent = no repeat.",
    )
    p_ctest.add_argument(
        "--shard-index",
        type=int,
        default=0,
        metavar="N",
        help="0-based shard index (default 0). Ignored unless --shard-count > 1.",
    )
    p_ctest.add_argument(
        "--shard-count",
        type=int,
        default=0,
        metavar="N",
        help="Total number of shards. 0 or 1 disables sharding (default).",
    )

    # cache-var
    p_cv = sub.add_parser("cache-var", help="Read a CMakeCache.txt variable")
    p_cv.add_argument("--build-dir", required=True)
    p_cv.add_argument("--name", required=True, help="Cache variable name (case-sensitive)")
    p_cv.add_argument("--default", default=None, help="Fallback when the variable is missing")
    p_cv.add_argument("--required", action="store_true", help="Exit 1 if missing and no --default")
    p_cv.add_argument(
        "--output-key", default="", help="GITHUB_OUTPUT key (default: lowercase name)"
    )

    args = parser.parse_args()
    commands = {
        "detect-jobs": cmd_detect_jobs,
        "build": cmd_build,
        "configure": cmd_configure,
        "ctest": cmd_ctest,
        "cache-var": cmd_cache_var,
    }
    commands[args.command](args)


if __name__ == "__main__":
    main()
