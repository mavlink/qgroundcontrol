#!/usr/bin/env python3
"""CMake build and configure helpers for CI.

Subcommands:
    build       Run cmake --build with CI-friendly output and timing
    configure   Run the shared configure library with CI defaults
    ctest       Run CTest with standardized arguments and timing
    summary     Write the job summary's Build section: result, duration, toolchain,
                compiler and moc cache hit rates, and the slowest build steps
"""

from __future__ import annotations

import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import _bootstrap as _bootstrap
from common.cmake import read_cache_dict
from common.file_traversal import find_repo_root
from common.format import format_count, format_duration, format_ratio
from common.gh_actions import (
    append_github_env,
    env_default,
    env_flag,
    gh_error,
    gh_notice,
    gh_warning,
    write_github_output,
    write_step_summary,
)
from common.markdown import code_block, code_span, md_table, summary_section
from common.proc import run_tee
from moccache.stats import KIND_DESCRIPTIONS, collect_stats
from qgc_tools import python_env
from qgc_tools.build_profile import parse_ninja_log, snapshot_summary, write_snapshot
from qgc_tools.cmake_configure import CMakeConfig, configure
from qgc_tools.compiler_cache import ccache_facts, ccache_stats, sccache_facts, sccache_stats


def detect_jobs(requested: str = "auto") -> int:
    """Detect number of parallel jobs for the current platform."""
    if requested != "auto":
        if not re.match(r"^[1-9]\d*$", requested):
            raise ValueError(f"Invalid parallel job count: {requested}")
        return int(requested)

    return os.cpu_count() or 2


def multi_config_build_type() -> str:
    """Return CMAKE_BUILD_TYPE when the build tree in the working directory is multi-config.

    Multi-config generators (Xcode for iOS-appstore) build and test the configuration that
    `--config`/`-C` select; single-config generators (Ninja) ignore both.
    """
    cache = read_cache_dict("CMakeCache.txt")
    return cache.get("CMAKE_BUILD_TYPE", "") if cache.get("CMAKE_CONFIGURATION_TYPES") else ""


def cmd_build(args: argparse.Namespace) -> int:
    """Run cmake --build with CI-friendly output."""
    cmd = ["cmake", "--build", "."]

    if args.target:
        cmd += ["--target", args.target]
    if build_type := args.build_type or multi_config_build_type():
        cmd += ["--config", build_type]
    if args.parallel:
        if args.parallel_jobs:
            if not re.match(r"^[1-9]\d*$", args.parallel_jobs):
                gh_error(f"parallel-jobs must be a positive integer, got '{args.parallel_jobs}'")
                return 1
            cmd += ["--parallel", args.parallel_jobs]
        else:
            cmd.append("--parallel")

    output_file = args.output_file
    if args.reviewdog and not output_file:
        output_file = os.path.join(os.environ.get("RUNNER_TEMP", "."), "build-output.log")

    if output_file and args.reviewdog:
        append_github_env({"REVIEWDOG_LOG": output_file})

    print(f"Running: {' '.join(cmd)}", flush=True)
    previous = None
    try:
        previous = parse_ninja_log(Path(".ninja_log")) if Path(".ninja_log").exists() else []
    except (OSError, ValueError) as error:
        gh_warning(f"Cannot read previous Ninja timings: {error}")
    start = time.monotonic()

    if output_file:
        exit_code = run_tee(cmd, output_file)
    else:
        result = subprocess.run(cmd, check=False)
        exit_code = result.returncode

    duration = int(time.monotonic() - start)

    if Path(".ninja_log").is_file():
        try:
            directory = Path(
                tempfile.mkdtemp(prefix="build-profile-", dir=os.environ.get("RUNNER_TEMP"))
            )
            write_snapshot(Path.cwd(), directory, previous=previous, wall_seconds=duration)
            write_github_output({"profile_path": str(directory), "profile_name": directory.name})
        except (OSError, ValueError) as error:
            gh_warning(f"Cannot write Ninja build timings: {error}")

    write_github_output(
        {"build_success": str(exit_code == 0).lower(), "duration_seconds": str(duration)}
    )
    if exit_code == 0:
        gh_notice(f"Build completed in {duration}s")
    else:
        gh_error(f"Build failed after {duration}s")
    return exit_code


_COMPILER_SETTING = re.compile(r'^set\(CMAKE_CXX_COMPILER_(ID|VERSION) "([^"]*)"\)', re.MULTILINE)
SLOWEST_STEPS = 10


def toolchain(build_dir: Path) -> str:
    """Describe the build type, generator, and C++ compiler that configure recorded."""
    cache = (
        read_cache_dict(build_dir / "CMakeCache.txt")
        if (build_dir / "CMakeCache.txt").is_file()
        else {}
    )
    compiler = ""
    for path in sorted(build_dir.glob("CMakeFiles/*/CMakeCXXCompiler.cmake")):
        settings = dict(
            _COMPILER_SETTING.findall(path.read_text(encoding="utf-8", errors="replace"))
        )
        compiler = " ".join(part for part in (settings.get("ID"), settings.get("VERSION")) if part)
    parts = [cache.get("CMAKE_BUILD_TYPE", ""), cache.get("CMAKE_GENERATOR", ""), compiler]
    if cache.get("QGC_UNITY_BUILD", "").upper() in {"ON", "TRUE", "1"}:
        parts.append("unity")
    return ", ".join(part for part in parts if part)


def _compiler_cache() -> tuple[list[tuple[str, object]], str]:
    if os.environ.get("QGC_COMPILER_CACHE_TOOL") == "sccache":
        report = sccache_stats()
        log = Path(os.environ.get("SCCACHE_ERROR_LOG", ""))
        try:
            tail = "\n".join(log.read_text(encoding="utf-8", errors="replace").splitlines()[-50:])
        except OSError:
            tail = ""
        return (sccache_facts(report) if report else []), (code_block(tail) if tail.strip() else "")
    stats = ccache_stats()
    return (ccache_facts(stats) if stats else []), ""


def _moc_cache(build_dir: Path) -> tuple[list[tuple[str, object]], str]:
    stats = collect_stats(build_dir)
    total = stats.hits + stats.misses
    if not total and not stats.counts:
        return [], ""
    facts: list[tuple[str, object]] = [
        ("moc cache", f"{format_ratio(stats.hits, total)} hits ({stats.scope})"),
        ("moc cache size", stats.size()),
    ]
    events = md_table(
        ["Event", "Count", "Meaning"],
        [
            [code_span(kind), format_count(count), KIND_DESCRIPTIONS.get(kind, "")]
            for kind, count in sorted(stats.counts.items())
        ],
        align=["left", "right", "left"],
        markdown=True,
    )
    return facts, events if stats.counts else ""


def cmd_summary(args: argparse.Namespace) -> int:
    """Write the Build section of the job summary from the build's outputs and cache statistics."""
    build_dir = Path.cwd()
    outcome = {"success": "passed", "failure": "failed"}.get(args.outcome, args.outcome)
    seconds = float(args.seconds) if re.fullmatch(r"\d+(\.\d+)?", args.seconds or "") else None
    cache_facts, sccache_log = _compiler_cache()
    moc_facts, moc_events = _moc_cache(build_dir)
    step_facts, timings = (
        snapshot_summary(Path(args.profile_dir), limit=SLOWEST_STEPS)
        if args.profile_dir
        else ([], [])
    )
    target = args.target if args.target not in {"", "all"} else ""
    section = summary_section(
        f"Build ({target})" if target else "Build",
        [
            ("Result", outcome),
            ("Duration", format_duration(seconds) if seconds is not None else ""),
            ("Toolchain", toolchain(build_dir)),
            *cache_facts,
            *moc_facts,
            *step_facts,
        ],
        collapsed=[
            *timings,
            ("moc cache events", moc_events),
            ("sccache error log (last 50 lines)", sccache_log),
        ],
    )
    print(section)
    write_step_summary(section)
    return 0


def stable_build_ref() -> bool:
    """Release tags and Stable* branch builds set QGC_STABLE_BUILD."""
    ref = os.environ.get("GITHUB_REF", "")
    return os.environ.get("GITHUB_REF_TYPE") == "tag" or ref.startswith("refs/heads/Stable")


def cmd_configure(args: argparse.Namespace) -> int:
    """Configure the checkout from a preset with the shared configure library and CI defaults."""
    if not args.preset:
        gh_error("configure needs a CMake preset (--preset or INPUT_PRESET)")
        return 1
    extra_args = args.extra_args.split()
    venv_python = python_env.executable("python")
    if venv_python.is_file() and os.access(venv_python, os.X_OK):
        # CMake-invoked jinja generators need the locked environment's packages.
        extra_args.append(f"-DPython3_EXECUTABLE={venv_python}")
    else:
        gh_warning(
            f"venv python not found at {venv_python}; CMake will pick system python (may lack jinja2)"
        )
    if cache_program := os.environ.get("QGC_CACHE_PROGRAM"):
        extra_args.append(f"-DQGC_CACHE_PROGRAM:FILEPATH={cache_program}")

    config = CMakeConfig(
        source_dir=find_repo_root(Path(__file__)),
        build_dir=Path(args.build_dir),
        preset=args.preset,
        stable=args.stable,
        use_qt_cmake=args.use_qt_cmake,
        extra_args=extra_args,
    )
    start = time.monotonic()
    exit_code = configure(config)
    duration = int(time.monotonic() - start)
    gh_notice(f"Configure completed in {duration}s")
    return exit_code


def _maybe_wrap_xvfb(cmd: list[str]) -> list[str]:
    """Prefix `xvfb-run` on headless Linux so the offscreen plugin's GLX path
    gets a display and QRhiGles2 can create a software GL context."""
    if sys.platform.startswith("linux") and not os.environ.get("DISPLAY"):
        xvfb = shutil.which("xvfb-run")
        if xvfb:
            gh_notice("No DISPLAY; running CTest under xvfb-run")
            return [xvfb, "-a", *cmd]
        gh_warning("xvfb-run not found; tests run without a GL context")
    return cmd


def cmd_ctest(args: argparse.Namespace) -> int:
    """Run CTest with standardized arguments and timing."""
    try:
        jobs = detect_jobs(args.jobs)
    except ValueError as error:
        gh_error(str(error))
        return 1
    cmd = [
        "ctest",
        "--output-on-failure",
        "--no-tests=error",
        "--output-junit",
        args.junit_output,
        "--parallel",
        str(jobs),
    ]
    if build_type := args.build_type or multi_config_build_type():
        cmd += ["-C", build_type]
    if args.include_labels:
        cmd += ["-L", args.include_labels]
    if args.exclude_labels:
        cmd += ["-LE", args.exclude_labels]

    start = time.monotonic()
    exit_code = run_tee(_maybe_wrap_xvfb(cmd), args.ctest_output)
    duration = int(time.monotonic() - start)
    gh_notice(f"Tests completed in {duration}s")
    return exit_code


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)

    # build and configure defaults come from the composite actions' INPUT_* step environment.
    p_build = sub.add_parser("build")
    p_build.add_argument("--target", default=env_default("INPUT_TARGET"))
    p_build.add_argument(
        "--build-type", default="", help="Default: a multi-config tree's CMAKE_BUILD_TYPE"
    )
    p_build.add_argument("--parallel", action="store_true", default=env_flag("INPUT_PARALLEL"))
    p_build.add_argument("--parallel-jobs", default=env_default("INPUT_PARALLEL_JOBS"))
    p_build.add_argument("--output-file", default=env_default("INPUT_OUTPUT_FILE"))
    p_build.add_argument("--reviewdog", action="store_true", default=env_flag("INPUT_REVIEWDOG"))

    p_conf = sub.add_parser("configure")
    p_conf.add_argument("--build-dir", default=env_default("INPUT_BUILD_DIR", "build"))
    p_conf.add_argument("--preset", default=env_default("INPUT_PRESET"))
    p_conf.add_argument("--stable", action="store_true", default=stable_build_ref())
    p_conf.add_argument(
        "--no-qt-cmake",
        action="store_false",
        dest="use_qt_cmake",
        default=env_flag("INPUT_USE_QT_CMAKE", default=True),
    )
    p_conf.add_argument("--extra-args", default=env_default("INPUT_EXTRA_ARGS"))

    # ctest
    p_ctest = sub.add_parser("ctest")
    p_ctest.add_argument("--junit-output", required=True)
    p_ctest.add_argument("--ctest-output", required=True)
    p_ctest.add_argument(
        "--build-type", default="", help="Default: a multi-config tree's CMAKE_BUILD_TYPE"
    )
    p_ctest.add_argument(
        "--jobs", default="auto", help="Parallel CTest jobs, or 'auto' for one per CPU"
    )
    p_ctest.add_argument("--include-labels", default="")
    p_ctest.add_argument("--exclude-labels", default="")

    # summary reads the cmake-build action's step results from its environment.
    p_summary = sub.add_parser("summary")
    p_summary.add_argument("--target", default=env_default("INPUT_TARGET"))
    p_summary.add_argument("--outcome", default=env_default("BUILD_OUTCOME", "unknown"))
    p_summary.add_argument("--seconds", default=env_default("BUILD_SECONDS"))
    p_summary.add_argument("--profile-dir", default=env_default("BUILD_PROFILE_PATH"))

    args = parser.parse_args(argv)
    commands = {
        "build": cmd_build,
        "configure": cmd_configure,
        "ctest": cmd_ctest,
        "summary": cmd_summary,
    }
    return commands[args.command](args)


if __name__ == "__main__":
    raise SystemExit(main())
