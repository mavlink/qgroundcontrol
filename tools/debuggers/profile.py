#!/usr/bin/env python3
"""Profile QGroundControl; pass application arguments after --."""

from __future__ import annotations

import argparse
import os
import re
import shutil
import subprocess
import sys
from datetime import datetime
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]


def find_executable(build: Path, config: str) -> Path:
    for directory in (build / config, build):
        for relative in ("QGroundControl", "QGroundControl.app/Contents/MacOS/QGroundControl"):
            candidate = directory / relative
            if candidate.is_file() and os.access(candidate, os.X_OK):
                return candidate
    raise FileNotFoundError(f"No QGroundControl executable in {build} ({config}); build it first")


def report(command: list[str]) -> None:
    result = subprocess.run(command, check=True, capture_output=True, text=True)
    print("\n".join(result.stdout.splitlines()[:50]))


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    modes = parser.add_mutually_exclusive_group()
    for mode in ("perf", "memcheck", "callgrind", "massif", "heaptrack", "sanitize"):
        modes.add_argument(f"--{mode}", dest="mode", action="store_const", const=mode)
    parser.set_defaults(mode="perf")
    parser.add_argument("-b", "--build-dir", type=Path)
    parser.add_argument("--config", default="Debug")
    parser.add_argument("--output-dir", type=Path, default=REPO / "profile")
    parser.add_argument("--no-viewer", action="store_true")
    parser.add_argument("args", nargs=argparse.REMAINDER)
    args = parser.parse_args(argv)
    app_args = args.args[1:] if args.args[:1] == ["--"] else args.args
    build = (
        args.build_dir or REPO / ("build-sanitize" if args.mode == "sanitize" else "build")
    ).resolve()
    try:
        if args.mode == "sanitize":
            flags = "-fsanitize=address,undefined -fno-omit-frame-pointer"
            subprocess.run(
                [
                    sys.executable,
                    str(REPO / "tools/configure.py"),
                    "-B",
                    str(build),
                    "-S",
                    str(REPO),
                    "--debug",
                    "--",
                    f"-DCMAKE_C_FLAGS={flags}",
                    f"-DCMAKE_CXX_FLAGS={flags}",
                    "-DCMAKE_EXE_LINKER_FLAGS=-fsanitize=address,undefined",
                ],
                check=True,
            )
            subprocess.run(
                ["cmake", "--build", str(build), "--config", "Debug", "--parallel"], check=True
            )
            return subprocess.run(
                [str(find_executable(build, "Debug")), *app_args],
                cwd=REPO,
                check=False,
                env=os.environ
                | {
                    "ASAN_OPTIONS": "detect_leaks=1:halt_on_error=0:print_stats=1",
                    "UBSAN_OPTIONS": "print_stacktrace=1",
                },
            ).returncode
        binary = find_executable(build, args.config)
        tool = "valgrind" if args.mode in ("memcheck", "callgrind", "massif") else args.mode
        if not shutil.which(tool):
            raise FileNotFoundError(f"Required profiler is not on PATH: {tool}")
        args.output_dir.mkdir(parents=True, exist_ok=True)
        stamp = datetime.now().strftime("%Y%m%d-%H%M%S-%f")
        suffix = {"memcheck": "log", "perf": "data"}.get(args.mode, "out")
        output = (args.output_dir / f"{args.mode}-{stamp}.{suffix}").resolve()
        commands = {
            "memcheck": [
                "valgrind",
                "--leak-check=full",
                "--show-leak-kinds=all",
                "--track-origins=yes",
                "--verbose",
                f"--log-file={output}",
                f"--suppressions={REPO / 'tools/debuggers/valgrind.supp'}",
            ],
            "callgrind": [
                "valgrind",
                "--tool=callgrind",
                f"--callgrind-out-file={output}",
                "--collect-jumps=yes",
                "--collect-systime=yes",
            ],
            "massif": [
                "valgrind",
                "--tool=massif",
                f"--massif-out-file={output}",
                "--detailed-freq=1",
            ],
            "heaptrack": ["heaptrack", "-o", str(output)],
            "perf": ["perf", "record", "-g", "--call-graph", "dwarf", "-o", str(output)],
        }
        print(f"Recording {args.mode}: {output}", flush=True)
        result = subprocess.run(
            [*commands[args.mode], str(binary), *app_args], cwd=REPO, check=False
        )
        if result.returncode:
            return result.returncode
        if args.mode == "memcheck" and output.is_file():
            print(
                "\n".join(
                    line
                    for line in output.read_text().splitlines()
                    if re.search(r"(definitely|indirectly|possibly) lost:", line)
                )
            )
        elif args.mode == "perf":
            report(
                ["perf", "report", "--stdio", "-i", str(output), "--no-children", "--sort=dso,sym"]
            )
        elif args.mode == "massif" and shutil.which("ms_print"):
            report(["ms_print", str(output)])
        if not args.no_viewer:
            viewer = {"callgrind": "kcachegrind", "heaptrack": "heaptrack_gui"}.get(args.mode)
            candidates = sorted(output.parent.glob(f"{output.name}*"))
            if viewer and shutil.which(viewer) and candidates:
                subprocess.Popen([viewer, str(candidates[0])], start_new_session=True)
        return 0
    except (OSError, subprocess.CalledProcessError) as error:
        print(f"Profiling failed: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
