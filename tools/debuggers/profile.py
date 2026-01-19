#!/usr/bin/env python3
"""Profile QGroundControl using various tools.

Usage:
    ./tools/debuggers/profile.py                     # Run with perf (default)
    ./tools/debuggers/profile.py --memcheck          # Check for memory leaks (valgrind)
    ./tools/debuggers/profile.py --callgrind         # CPU profiling (valgrind)
    ./tools/debuggers/profile.py --massif            # Heap profiling (valgrind)
    ./tools/debuggers/profile.py --heaptrack         # Heap profiling (heaptrack)
    ./tools/debuggers/profile.py --perf              # CPU profiling (perf)
    ./tools/debuggers/profile.py --sanitize          # Build with sanitizers
    ./tools/debuggers/profile.py --timeout SECONDS   # Set profiling timeout (default: 300)

Requirements:
    - valgrind (for memcheck, callgrind, massif)
    - perf (for CPU profiling)
    - heaptrack (optional, for heap profiling)
    - kcachegrind (optional, for viewing callgrind output)
"""

from __future__ import annotations

import argparse
import os
import re
import shutil
import subprocess
import sys
from dataclasses import dataclass
from datetime import datetime
from pathlib import Path


@dataclass
class ProfileResult:
    """Result of a profiling run."""

    output_file: Path
    exit_code: int
    timed_out: bool = False


class Profiler:
    """Multi-tool profiler for QGroundControl."""

    def __init__(
        self,
        build_dir: Path,
        output_dir: Path,
        timeout: int = 300,
        extra_args: list[str] | None = None,
    ):
        self.build_dir = build_dir
        self.output_dir = output_dir
        self.timeout = timeout
        self.extra_args = extra_args or []
        self.executable = build_dir / "QGroundControl"
        self.repo_root = Path(__file__).parent.parent.parent

    def _timestamp(self) -> str:
        """Generate timestamp for output files."""
        return datetime.now().strftime("%Y%m%d-%H%M%S")

    def _check_tool(self, tool: str, install_hint: str) -> bool:
        """Check if a tool is available."""
        if shutil.which(tool) is None:
            print(f"Error: {tool} not found. Install with: {install_hint}", file=sys.stderr)
            return False
        return True

    def _check_executable(self) -> bool:
        """Check if QGroundControl executable exists."""
        if not self.executable.exists() or not os.access(self.executable, os.X_OK):
            print(f"Error: Executable not found: {self.executable}", file=sys.stderr)
            print("Build the project first: cmake --build build", file=sys.stderr)
            return False
        return True

    def _run_with_timeout(
        self, cmd: list[str], env: dict[str, str] | None = None
    ) -> ProfileResult:
        """Run command with timeout, returning result."""
        output_file = Path(cmd[-1]) if cmd else Path()

        merged_env = os.environ.copy()
        if env:
            merged_env.update(env)

        try:
            result = subprocess.run(
                cmd,
                timeout=self.timeout,
                env=merged_env,
                capture_output=False,
            )
            return ProfileResult(output_file=output_file, exit_code=result.returncode)
        except subprocess.TimeoutExpired:
            print(f"Error: Command timed out after {self.timeout}s", file=sys.stderr)
            return ProfileResult(output_file=output_file, exit_code=124, timed_out=True)

    def run_memcheck(self) -> int:
        """Run memory leak check with valgrind."""
        if not self._check_tool("valgrind", "sudo apt install valgrind"):
            return 1
        if not self._check_executable():
            return 1

        self.output_dir.mkdir(parents=True, exist_ok=True)
        output = self.output_dir / f"memcheck-{self._timestamp()}.log"
        supp_file = self.repo_root / "tools" / "debuggers" / "valgrind.supp"

        print(f"Running memory leak check...")
        print(f"Output: {output}")
        print(f"Timeout: {self.timeout}s")

        cmd = [
            "valgrind",
            "--leak-check=full",
            "--show-leak-kinds=all",
            "--track-origins=yes",
            "--verbose",
            f"--log-file={output}",
        ]
        if supp_file.exists():
            cmd.append(f"--suppressions={supp_file}")
        cmd.append(str(self.executable))
        cmd.extend(self.extra_args)

        result = self._run_with_timeout(cmd)

        if result.timed_out:
            return 124

        print(f"Memcheck complete. Results: {output}")

        # Show summary
        if output.exists():
            content = output.read_text()
            for pattern in ["definitely lost:", "indirectly lost:", "possibly lost:"]:
                for line in content.splitlines():
                    if pattern in line:
                        print(line)

        return 0

    def run_callgrind(self) -> int:
        """Run CPU profiling with callgrind."""
        if not self._check_tool("valgrind", "sudo apt install valgrind"):
            return 1
        if not self._check_executable():
            return 1

        self.output_dir.mkdir(parents=True, exist_ok=True)
        output = self.output_dir / f"callgrind-{self._timestamp()}.out"

        print("Running CPU profiling with callgrind...")
        print(f"Output: {output}")
        print("WARNING: This will be SLOW. Use for targeted profiling.")
        print(f"Timeout: {self.timeout}s")

        cmd = [
            "valgrind",
            "--tool=callgrind",
            f"--callgrind-out-file={output}",
            "--collect-jumps=yes",
            "--collect-systime=yes",
            str(self.executable),
            *self.extra_args,
        ]

        result = self._run_with_timeout(cmd)

        if result.timed_out:
            return 124

        print(f"Callgrind complete. Results: {output}")

        if shutil.which("kcachegrind"):
            print("Opening with kcachegrind...")
            subprocess.Popen(["kcachegrind", str(output)])
        else:
            print("Install kcachegrind to visualize: sudo apt install kcachegrind")

        return 0

    def run_massif(self) -> int:
        """Run heap profiling with massif."""
        if not self._check_tool("valgrind", "sudo apt install valgrind"):
            return 1
        if not self._check_executable():
            return 1

        self.output_dir.mkdir(parents=True, exist_ok=True)
        output = self.output_dir / f"massif-{self._timestamp()}.out"

        print("Running heap profiling with massif...")
        print(f"Output: {output}")
        print(f"Timeout: {self.timeout}s")

        cmd = [
            "valgrind",
            "--tool=massif",
            f"--massif-out-file={output}",
            "--detailed-freq=1",
            str(self.executable),
            *self.extra_args,
        ]

        result = self._run_with_timeout(cmd)

        if result.timed_out:
            return 124

        print(f"Massif complete. Results: {output}")

        # Print summary
        if shutil.which("ms_print") and output.exists():
            ms_result = subprocess.run(
                ["ms_print", str(output)], capture_output=True, text=True
            )
            for line in ms_result.stdout.splitlines()[:50]:
                print(line)

        return 0

    def run_heaptrack(self) -> int:
        """Run heap profiling with heaptrack."""
        if not self._check_tool("heaptrack", "sudo apt install heaptrack"):
            return 1
        if not self._check_executable():
            return 1

        self.output_dir.mkdir(parents=True, exist_ok=True)

        print("Running heap profiling with heaptrack...")
        print(f"Timeout: {self.timeout}s")

        cmd = [
            "heaptrack",
            "-o",
            str(self.output_dir / "heaptrack"),
            str(self.executable),
            *self.extra_args,
        ]

        result = self._run_with_timeout(cmd)

        if result.timed_out:
            return 124

        # Find latest output file
        heaptrack_files = sorted(self.output_dir.glob("heaptrack.*.gz"), reverse=True)
        if heaptrack_files:
            latest = heaptrack_files[0]
            print(f"Heaptrack complete. Results: {latest}")

            if shutil.which("heaptrack_gui"):
                print("Opening with heaptrack_gui...")
                subprocess.Popen(["heaptrack_gui", str(latest)])
            else:
                print("Install heaptrack-gui to visualize")

        return 0

    def run_perf(self) -> int:
        """Run CPU profiling with perf."""
        if not self._check_tool("perf", "sudo apt install linux-tools-generic"):
            return 1
        if not self._check_executable():
            return 1

        self.output_dir.mkdir(parents=True, exist_ok=True)
        output = self.output_dir / f"perf-{self._timestamp()}.data"

        print("Running CPU profiling with perf...")
        print(f"Output: {output}")
        print(f"Timeout: {self.timeout}s")

        # Check permissions
        paranoid_path = Path("/proc/sys/kernel/perf_event_paranoid")
        if paranoid_path.exists():
            try:
                paranoid = int(paranoid_path.read_text().strip())
                if paranoid > 1:
                    print(
                        "WARNING: May need root or: sudo sysctl kernel.perf_event_paranoid=-1"
                    )
            except (ValueError, PermissionError):
                pass

        cmd = [
            "perf",
            "record",
            "-g",
            "--call-graph",
            "dwarf",
            "-o",
            str(output),
            str(self.executable),
            *self.extra_args,
        ]

        result = self._run_with_timeout(cmd)

        if result.timed_out:
            return 124

        print(f"Perf recording complete. Results: {output}")

        # Generate report
        print("Generating report...")
        report_result = subprocess.run(
            ["perf", "report", "-i", str(output), "--no-children", "--sort=dso,sym"],
            capture_output=True,
            text=True,
        )
        for line in report_result.stdout.splitlines()[:50]:
            print(line)

        print()
        print(f"Interactive view: perf report -i {output}")
        print(
            f"Flamegraph: perf script -i {output} | stackcollapse-perf.pl | flamegraph.pl > flame.svg"
        )

        return 0

    def run_sanitize(self) -> int:
        """Build and run with sanitizers (ASan + UBSan)."""
        print("Building with sanitizers...")

        sanitize_build = self.repo_root / "build-sanitize"

        # Configure with sanitizers
        cmake_result = subprocess.run(
            [
                "cmake",
                "-B",
                str(sanitize_build),
                "-S",
                str(self.repo_root),
                "-DCMAKE_BUILD_TYPE=Debug",
                "-DCMAKE_C_FLAGS=-fsanitize=address,undefined -fno-omit-frame-pointer",
                "-DCMAKE_CXX_FLAGS=-fsanitize=address,undefined -fno-omit-frame-pointer",
                "-DCMAKE_EXE_LINKER_FLAGS=-fsanitize=address,undefined",
                "-G",
                "Ninja",
            ],
            capture_output=True,
            text=True,
        )

        if cmake_result.returncode != 0:
            print(f"CMake configuration failed:\n{cmake_result.stderr}", file=sys.stderr)
            return 1

        # Build
        build_result = subprocess.run(
            ["cmake", "--build", str(sanitize_build), "--parallel"],
            capture_output=False,
        )

        if build_result.returncode != 0:
            print("Build failed", file=sys.stderr)
            return 1

        print("Sanitizer build complete")
        print(f"Run: {sanitize_build}/QGroundControl")
        print("Errors will be reported at runtime")
        print(f"Timeout: {self.timeout}s")

        # Run with sanitizer options
        env = {
            "ASAN_OPTIONS": "detect_leaks=1:halt_on_error=0:print_stats=1",
            "UBSAN_OPTIONS": "print_stacktrace=1",
        }

        sanitize_exe = sanitize_build / "QGroundControl"
        cmd = [str(sanitize_exe), *self.extra_args]

        result = self._run_with_timeout(cmd, env=env)

        if result.timed_out:
            return 124

        return 0


def parse_args() -> argparse.Namespace:
    """Parse command line arguments."""
    parser = argparse.ArgumentParser(
        description="Profile QGroundControl using various tools",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s                     # Run with perf (default)
  %(prog)s --memcheck          # Check for memory leaks
  %(prog)s --callgrind         # CPU profiling (slow but detailed)
  %(prog)s --sanitize          # Build and run with ASan/UBSan
  %(prog)s --timeout 60        # Set 60 second timeout
""",
    )

    mode_group = parser.add_mutually_exclusive_group()
    mode_group.add_argument(
        "--memcheck", action="store_true", help="Memory leak check (valgrind)"
    )
    mode_group.add_argument(
        "--callgrind", action="store_true", help="CPU profiling (valgrind callgrind)"
    )
    mode_group.add_argument(
        "--massif", action="store_true", help="Heap profiling (valgrind massif)"
    )
    mode_group.add_argument(
        "--heaptrack", action="store_true", help="Heap profiling (heaptrack)"
    )
    mode_group.add_argument(
        "--perf", action="store_true", help="CPU profiling (perf) [default]"
    )
    mode_group.add_argument(
        "--sanitize", action="store_true", help="Build with ASan/UBSan and run"
    )

    parser.add_argument(
        "--timeout",
        type=int,
        default=300,
        metavar="SECONDS",
        help="Profiling timeout in seconds (default: 300)",
    )
    parser.add_argument(
        "-b",
        "--build-dir",
        type=Path,
        default=None,
        help="Build directory (default: build)",
    )
    parser.add_argument(
        "extra_args",
        nargs="*",
        help="Additional arguments to pass to QGroundControl",
    )

    return parser.parse_args()


def main() -> int:
    """Main entry point."""
    args = parse_args()

    repo_root = Path(__file__).parent.parent.parent
    build_dir = args.build_dir or repo_root / "build"
    output_dir = repo_root / "profile"

    profiler = Profiler(
        build_dir=build_dir,
        output_dir=output_dir,
        timeout=args.timeout,
        extra_args=args.extra_args,
    )

    # Dispatch to appropriate mode
    if args.memcheck:
        return profiler.run_memcheck()
    elif args.callgrind:
        return profiler.run_callgrind()
    elif args.massif:
        return profiler.run_massif()
    elif args.heaptrack:
        return profiler.run_heaptrack()
    elif args.sanitize:
        return profiler.run_sanitize()
    else:
        # Default to perf
        return profiler.run_perf()


if __name__ == "__main__":
    sys.exit(main())
