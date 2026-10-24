#!/usr/bin/env python3
"""CMake build and configure helpers for CI.

Subcommands:
    build       Run cmake --build with CI-friendly output and timing, teeing it to a log, then
                write the job summary's Build section: result, duration, toolchain, compiler and
                moc cache hit rates, the slowest build steps and, after a failure, the failed
                targets and first errors, which also become error annotations
    configure   Run the shared configure library with CI defaults, teeing CMake's output to the
                build directory, then write the job summary's Configure section: result,
                duration, preset, generator, configure-check reuse and, after a failure, the
                first CMake error, which also becomes an error annotation
    ctest       Run CTest with standardized arguments and timing
"""

from __future__ import annotations

import argparse
import hashlib
import os
import re
import secrets
import shutil
import subprocess
import sys
import tempfile
import time
from dataclasses import dataclass, replace
from pathlib import Path
from typing import TYPE_CHECKING, Any

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
    gh_warning,
    write_github_output,
    write_step_summary,
)
from common.markdown import (
    code_block,
    code_span,
    escape_text,
    md_table,
    more_items,
    summary_section,
)
from common.proc import run_tee
from moccache.stats import KIND_DESCRIPTIONS, collect_stats
from qgc_tools import python_env
from qgc_tools.build_errors import (
    MAX_ERRORS,
    BuildFailure,
    Diagnostic,
    cmake_errors,
    parse_build_failure,
    repo_path,
)
from qgc_tools.build_profile import parse_ninja_log, snapshot_summary, write_snapshot
from qgc_tools.cmake_configure import CMakeConfig, configure
from qgc_tools.compiler_cache import ccache_facts, ccache_stats, sccache_facts, sccache_stats

if TYPE_CHECKING:
    from collections.abc import Sequence

BUILD_LOG = "build-output.log"
CONFIGURE_LOG = "cmake-configure.log"
CONFIGURE_TAIL_LINES = 40
REPO_ROOT = find_repo_root(Path(__file__))


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


@dataclass
class BuildRun:
    """What the Build summary reports about one cmake --build invocation."""

    seconds: int | None = None
    profile_dir: str = ""
    # Compiler-cache statistics before the build, so the summary shows only this build's share.
    cache_baseline: dict[str, Any] | None = None
    failure: BuildFailure | None = None


def cmd_build(args: argparse.Namespace) -> int:
    """Run cmake --build, then write the Build section even when the build fails.

    A cancelled job (KeyboardInterrupt) writes no section.
    """
    run = BuildRun()
    try:
        exit_code = _build(args, run)
    except Exception:
        write_build_summary(Path.cwd(), args.target, "failed", run)
        raise
    write_build_summary(Path.cwd(), args.target, "passed" if exit_code == 0 else "failed", run)
    return exit_code


def _build(args: argparse.Namespace, run: BuildRun) -> int:
    """Run cmake --build with CI-friendly output, recording its duration and Ninja timings."""
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

    output_file = args.output_file or os.path.join(os.environ.get("RUNNER_TEMP", "."), BUILD_LOG)
    if args.reviewdog:
        append_github_env({"REVIEWDOG_LOG": output_file})

    print(f"Running: {' '.join(cmd)}", flush=True)
    previous = None
    try:
        previous = parse_ninja_log(Path(".ninja_log")) if Path(".ninja_log").exists() else []
    except (OSError, ValueError) as error:
        gh_warning(f"Cannot read previous Ninja timings: {error}")
    run.cache_baseline = _compiler_cache_stats()
    start = time.monotonic()

    exit_code = run_tee(cmd, output_file)

    duration = int(time.monotonic() - start)
    run.seconds = duration

    if Path(".ninja_log").is_file():
        try:
            directory = Path(
                tempfile.mkdtemp(prefix="build-profile-", dir=os.environ.get("RUNNER_TEMP"))
            )
            write_snapshot(Path.cwd(), directory, previous=previous, wall_seconds=duration)
            write_github_output({"profile_path": str(directory), "profile_name": directory.name})
            run.profile_dir = str(directory)
        except (OSError, ValueError) as error:
            gh_warning(f"Cannot write Ninja build timings: {error}")

    write_github_output({"build_success": str(exit_code == 0).lower()})
    if exit_code == 0:
        print(f"Build completed in {duration}s", flush=True)
    else:
        print(f"Build failed after {duration}s", flush=True)
        run.failure = report_build_failure(Path(output_file), duration)
    return exit_code


def _read_log(path: Path) -> str:
    try:
        return path.read_text(encoding="utf-8", errors="replace")
    except OSError:
        return ""


def _annotate_error(diagnostic: Diagnostic, *, bases: Sequence[Path], title: str = "") -> None:
    """Annotate *diagnostic* at its checkout file, or name its location when outside the checkout."""
    file = repo_path(diagnostic.file, bases=bases, repo_root=REPO_ROOT) if diagnostic.file else ""
    message = diagnostic.message
    if diagnostic.file and not file:
        message = f"{diagnostic.location}: {message}"
    gh_error(
        message,
        title=title or diagnostic.title,
        file=file,
        line=diagnostic.line,
        col=diagnostic.col,
    )


def report_build_failure(log: Path, seconds: int) -> BuildFailure:
    """Annotate the first errors of a failed build from its log and return them for the summary.

    Compilers report paths from the build directory, the working directory here.
    """
    failure = parse_build_failure(_read_log(log))
    for diagnostic in failure.errors:
        _annotate_error(diagnostic, bases=(Path.cwd(), REPO_ROOT))
    if not failure.errors:
        gh_error(f"Build failed after {seconds}s; no error recognized in {log}")
    return failure


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


def _compiler_cache_stats() -> dict[str, Any] | None:
    if os.environ.get("QGC_COMPILER_CACHE_TOOL") == "sccache":
        return sccache_stats()
    return ccache_stats()


# Statistics that describe the cache rather than count events, so they are never differenced.
CACHE_GAUGES = frozenset(
    {
        "cache_size",
        "cache_size_kibibyte",
        "files_in_cache",
        "max_cache_size",
        "max_cache_size_kibibyte",
        "max_files_in_cache",
        "stats_updated_timestamp",
        "stats_zeroed_timestamp",
    }
)


def counter_delta(after: dict[str, Any], before: dict[str, Any] | None) -> dict[str, Any]:
    """Return *after*'s counters less *before*'s, recursing into nested counts.

    A counter that went down was zeroed in between, so it keeps its own value.
    """
    if not before:
        return after
    delta: dict[str, Any] = {}
    for key, value in after.items():
        earlier = before.get(key)
        if isinstance(value, dict) and isinstance(earlier, dict):
            delta[key] = counter_delta(value, earlier)
        elif (
            key not in CACHE_GAUGES
            and type(value) is int
            and type(earlier) is int
            and value >= earlier
        ):
            delta[key] = value - earlier
        else:
            delta[key] = value
    return delta


def _compiler_cache(baseline: dict[str, Any] | None) -> tuple[list[tuple[str, object]], str]:
    if os.environ.get("QGC_COMPILER_CACHE_TOOL") == "sccache":
        report = sccache_stats()
        log = Path(os.environ.get("SCCACHE_ERROR_LOG", ""))
        try:
            tail = "\n".join(log.read_text(encoding="utf-8", errors="replace").splitlines()[-50:])
        except OSError:
            tail = ""
        facts = sccache_facts(counter_delta(report, baseline)) if report else []
        return facts, (code_block(tail) if tail.strip() else "")
    stats = ccache_stats()
    return (ccache_facts(counter_delta(stats, baseline)) if stats else []), ""


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


def _error_text(diagnostic: Diagnostic, bases: Sequence[Path]) -> str:
    """One error as the log reported it, naming a checkout file relative to the checkout."""
    if "\n" in diagnostic.excerpt or not diagnostic.file:
        return diagnostic.excerpt or diagnostic.message
    file = repo_path(diagnostic.file, bases=bases, repo_root=REPO_ROOT) or diagnostic.file
    return f"{replace(diagnostic, file=file).location}: error: {diagnostic.message}"


def failure_details(failure: BuildFailure | None, bases: Sequence[Path]) -> str:
    """Return the failed targets and the first errors as Markdown, or ``""`` without any."""
    if failure is None:
        return ""
    parts = []
    if targets := failure.failed_targets:
        listed = [f"- {code_span(target)}" for target in targets[:MAX_ERRORS]]
        if len(targets) > MAX_ERRORS:
            listed.append(f"- {more_items(len(targets) - MAX_ERRORS)}")
        parts.append("Failed targets:\n\n" + "\n".join(listed))
    if errors := failure.errors:
        entries = [_error_text(diagnostic, bases) for diagnostic in errors]
        text = (
            "\n\n".join(entries) if any("\n" in entry for entry in entries) else "\n".join(entries)
        )
        shown = len(errors)
        heading = (
            f"First {shown} of {failure.error_count} errors:"
            if failure.error_count > shown
            else "Errors:"
        )
        parts.append(f"{heading}\n\n{code_block(text)}")
    return "\n\n".join(parts)


def write_build_summary(build_dir: Path, target: str, result: str, run: BuildRun) -> None:
    """Write the Build section from the build's result and the compiler and moc cache statistics.

    moccache reads the stats log under $MOCCACHE_DIR, else the checkout's .cache/moccache, which
    is also where the CMake moc launcher writes it. A failure here only warns: the summary must
    not fail a build or skip its cache save.
    """
    try:
        cache_facts, sccache_log = _compiler_cache(run.cache_baseline)
        moc_facts, moc_events = _moc_cache(build_dir)
        step_facts, timings = (
            snapshot_summary(Path(run.profile_dir), limit=SLOWEST_STEPS)
            if run.profile_dir
            else ([], [])
        )
        target = target if target not in {"", "all"} else ""
        section = summary_section(
            f"Build ({target})" if target else "Build",
            [
                ("Result", result),
                ("Duration", format_duration(run.seconds) if run.seconds is not None else ""),
                ("Toolchain", toolchain(build_dir)),
                *cache_facts,
                *moc_facts,
                *step_facts,
            ],
            collapsed=[
                (
                    "Failed targets / first errors",
                    failure_details(run.failure, (build_dir, REPO_ROOT)),
                ),
                *timings,
                ("moc cache events", moc_events),
                ("sccache error log (last 50 lines)", sccache_log),
            ],
        )
    except Exception as error:
        gh_warning(f"Cannot write the Build summary: {error}")
        return
    print(section)
    write_step_summary(section)


def stable_build_ref() -> bool:
    """Release tags and Stable* branch builds set QGC_STABLE_BUILD."""
    ref = os.environ.get("GITHUB_REF", "")
    return os.environ.get("GITHUB_REF_TYPE") == "tag" or ref.startswith("refs/heads/Stable")


INCLUDE_PATH_VARIABLES = ("INCLUDE", "CPATH", "C_INCLUDE_PATH", "CPLUS_INCLUDE_PATH")
PACKAGE_INVENTORIES = (
    ("dpkg-query", "-W", "-f=${binary:Package}=${Version}\n"),
    ("rpm", "-qa"),
)
HOMEBREW_CELLARS = (Path("/opt/homebrew/Cellar"), Path("/usr/local/Cellar"))


def system_header_digest() -> str:
    """Hash what decides which system headers configure checks find, beyond the toolchain.

    Dependencies probe some headers unconditionally (SDL3's libudev.h, libarchive's sys/acl.h),
    so a package installed after the cache plan ran must not reuse results measured without it.
    Windows SDK and MSVC headers come from INCLUDE.
    """
    parts = [f"{name}={os.environ.get(name, '')}" for name in INCLUDE_PATH_VARIABLES]
    for command in PACKAGE_INVENTORIES:
        if shutil.which(command[0]):
            result = subprocess.run(command, capture_output=True, text=True, check=False)
            parts += [f"{command[0]} exit={result.returncode}", *sorted(result.stdout.splitlines())]
            break
    for cellar in HOMEBREW_CELLARS:
        if cellar.is_dir():
            parts += sorted(path.relative_to(cellar).as_posix() for path in cellar.glob("*/*"))
    return hashlib.sha256("\n".join(parts).encode()).hexdigest()[:16]


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
    # The cache action points this into the restored compiler cache (CheckResultCache.cmake).
    if check_results := os.environ.get("QGC_CHECK_RESULT_CACHE_DIR"):
        salt = os.environ.get("QGC_CHECK_RESULT_CACHE_SALT", "")
        extra_args += [
            f"-DQGC_CHECK_RESULT_CACHE_DIR:PATH={check_results}",
            f"-DQGC_CHECK_RESULT_CACHE_SALT:STRING={salt},system={system_header_digest()}",
        ]

    config = CMakeConfig(
        source_dir=REPO_ROOT,
        build_dir=Path(args.build_dir),
        preset=args.preset,
        stable=args.stable,
        use_qt_cmake=args.use_qt_cmake,
        extra_args=extra_args,
    )
    build_dir = Path(args.build_dir)
    # Written first, so the action can upload the logs of a configure that raised.
    write_github_output({"logs_artifact": configure_logs_artifact(args.preset)})
    start = time.monotonic()
    try:
        exit_code = configure(config, log_file=build_dir / CONFIGURE_LOG)
    except Exception:
        write_configure_summary(build_dir, args.preset, "failed", None, "", None)
        raise
    duration = int(time.monotonic() - start)
    output = _read_log(build_dir / CONFIGURE_LOG)
    error = None
    if exit_code == 0:
        print(f"Configure completed in {duration}s", flush=True)
    else:
        print(f"Configure failed after {duration}s", flush=True)
        error = report_configure_failure(output, exit_code, build_dir)
    write_configure_summary(
        build_dir, args.preset, "passed" if exit_code == 0 else "failed", duration, output, error
    )
    return exit_code


def configure_logs_artifact(preset: str) -> str:
    """Name the artifact for a failed configure's logs, unique within the workflow run."""
    job = os.environ.get("GITHUB_JOB", "local")
    name = re.sub(r"[^\w.-]+", "-", f"{job}-{preset}")
    return f"cmake-configure-{name}-{secrets.token_hex(3)}"


def report_configure_failure(output: str, exit_code: int, build_dir: Path) -> Diagnostic | None:
    """Annotate the first CMake error of a failed configure, or the end of its output."""
    if errors := cmake_errors(output):
        # CMake reports paths from the source directory, or absolute ones.
        _annotate_error(errors[0], bases=(REPO_ROOT, build_dir), title="CMake configure failed")
        return errors[0]
    tail = _tail(output, 10)
    message = f"CMake exited with status {exit_code}"
    gh_error(f"{message}:\n{tail}" if tail else message, title="CMake configure failed")
    return None


def _tail(text: str, count: int) -> str:
    return "\n".join([line.rstrip() for line in text.splitlines() if line.strip()][-count:])


_CHECK_RESULTS = re.compile(
    r"^-- QGC: (?:Reused (?P<reused>\d+) configure-check results"
    r"|(?P<missing>No saved configure-check results)"
    r"|(?P<ignored>Ignoring configure-check results)"
    r"|Saved (?P<saved>\d+) configure-check results)",
    re.MULTILINE,
)


def configure_checks(output: str) -> str:
    """Summarize what CheckResultCache.cmake reported about reusing saved configure checks."""
    parts = []
    for match in _CHECK_RESULTS.finditer(output):
        if match["reused"]:
            parts.append(f"reused {format_count(int(match['reused']))} saved results")
        elif match["missing"]:
            parts.append("none saved for this toolchain")
        elif match["ignored"]:
            parts.append("saved results ignored (different fingerprint)")
        else:
            parts.append(f"saved {format_count(int(match['saved']))} results")
    return ", ".join(parts)


def write_configure_summary(
    build_dir: Path,
    preset: str,
    result: str,
    seconds: int | None,
    output: str,
    error: Diagnostic | None,
) -> None:
    """Write the Configure section; a failure here only warns, like the Build section."""
    try:
        cache = read_cache_dict(build_dir / "CMakeCache.txt")
        tail = _tail(output, CONFIGURE_TAIL_LINES) if result != "passed" else ""
        section = summary_section(
            "Configure",
            [
                ("Result", result),
                ("Duration", format_duration(seconds) if seconds is not None else ""),
                ("Preset", code_span(preset)),
                ("Generator", escape_text(cache.get("CMAKE_GENERATOR", ""))),
                ("Configure checks", configure_checks(output)),
            ],
            collapsed=[
                ("CMake error", code_block(error.excerpt) if error else ""),
                (
                    f"Configure output (last {CONFIGURE_TAIL_LINES} lines)",
                    code_block(tail) if tail else "",
                ),
            ],
        )
    except Exception as exception:
        gh_warning(f"Cannot write the Configure summary: {exception}")
        return
    print(section)
    write_step_summary(section)


def _maybe_wrap_xvfb(cmd: list[str]) -> list[str]:
    """Prefix `xvfb-run` on headless Linux so the offscreen plugin's GLX path
    gets a display and QRhiGles2 can create a software GL context."""
    if sys.platform.startswith("linux") and not os.environ.get("DISPLAY"):
        xvfb = shutil.which("xvfb-run")
        if xvfb:
            print("No DISPLAY; running CTest under xvfb-run", flush=True)
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
    print(f"Tests completed in {duration}s", flush=True)
    return exit_code


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)

    # Defaults come from the composite actions' INPUT_* step environment.
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

    p_ctest = sub.add_parser("ctest")
    p_ctest.add_argument("--junit-output", default=env_default("INPUT_JUNIT_OUTPUT"))
    p_ctest.add_argument("--ctest-output", default=env_default("INPUT_CTEST_OUTPUT"))
    p_ctest.add_argument(
        "--build-type", default="", help="Default: a multi-config tree's CMAKE_BUILD_TYPE"
    )
    p_ctest.add_argument(
        "--jobs",
        default=env_default("INPUT_PARALLEL", "auto"),
        help="Parallel CTest jobs, or 'auto' for one per CPU",
    )
    p_ctest.add_argument("--include-labels", default=env_default("INPUT_INCLUDE_LABELS"))
    p_ctest.add_argument("--exclude-labels", default=env_default("INPUT_EXCLUDE_LABELS"))

    args = parser.parse_args(argv)
    if args.command == "ctest" and not (args.junit_output and args.ctest_output):
        p_ctest.error("--junit-output and --ctest-output (or their INPUT_* variables) are required")
    commands = {"build": cmd_build, "configure": cmd_configure, "ctest": cmd_ctest}
    return commands[args.command](args)


if __name__ == "__main__":
    raise SystemExit(main())
