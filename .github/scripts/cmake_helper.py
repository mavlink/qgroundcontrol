#!/usr/bin/env python3
"""CMake build and configure helpers for CI.

Subcommands:
    build       Run cmake --build with CI-friendly output and timing
    configure   Run configure.py with standardized arguments
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

from common.gh_actions import append_github_env, gh_error, gh_notice, write_github_output


def _run_with_tee(cmd: list[str], output_file: str) -> int:
    # Use `bash | tee` so children keep a real stdout — Popen+PIPE deadlocks
    # Gradle/javac on Windows when grandchildren block-buffer 8KB+ output.
    bash = shutil.which("bash")
    if bash:
        quoted_cmd = " ".join(shlex.quote(c) for c in cmd)
        quoted_log = shlex.quote(output_file)
        script = f"set -o pipefail; {quoted_cmd} 2>&1 | tee {quoted_log}"
        return subprocess.run([bash, "-c", script], check=False).returncode

    with open(output_file, "w", encoding="utf-8") as log:
        proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        assert proc.stdout is not None
        for line in proc.stdout:
            sys.stdout.write(line)
            log.write(line)
        proc.wait()
        return proc.returncode


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
                gh_error(
                    f"parallel-jobs must be a positive integer, got '{args.parallel_jobs}'"
                )
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
        exit_code = _run_with_tee(cmd, output_file)
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


def cmd_configure(args: argparse.Namespace) -> None:
    """Run configure.py with standardized arguments."""
    workspace = os.environ.get("GITHUB_WORKSPACE", ".")
    cmd = [
        sys.executable,
        os.path.join(workspace, "tools", "configure.py"),
        "-S",
        args.source_dir,
        "-B",
        args.build_dir,
        "-G",
        args.generator,
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
        cmd.extend(args.extra_args.split())

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
        # CTest -I start,end,stride: stride=shard_count, start=shard_index+1, end=0 (last).
        start_idx = args.shard_index + 1
        cmd += ["-I", f"{start_idx},0,{args.shard_count}"]

    start = time.monotonic()
    exit_code = _run_with_tee(_maybe_wrap_xvfb(cmd), args.ctest_output)
    duration = int(time.monotonic() - start)
    gh_notice(f"Tests completed in {duration}s")
    sys.exit(exit_code)


_CACHE_LINE_RE = re.compile(r"^([A-Za-z0-9_.\-]+):[^=]+=(.*)$")


def read_cache_var(cache_path: str, name: str) -> str | None:
    """Return the value of a CMake cache variable, or None if not set."""
    return read_cache_dict(cache_path).get(name)


def read_cache_dict(cache_path: str) -> dict[str, str]:
    """Return all typed entries from CMakeCache.txt as a flat name->value dict."""
    entries: dict[str, str] = {}
    try:
        with open(cache_path, encoding="utf-8") as fh:
            for line in fh:
                match = _CACHE_LINE_RE.match(line.rstrip("\n"))
                if match:
                    entries[match.group(1)] = match.group(2)
    except FileNotFoundError:
        pass
    return entries


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
