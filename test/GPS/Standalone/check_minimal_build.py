"""Build GPS library selections without Qt and verify their dependency boundaries."""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
import tempfile
from pathlib import Path

CASES = {
    "DriverReports": ({"QGCGPSDriverReportsConsumer"}, "QGCGPSDriverReportsHeaderChecks"),
    "ReceiverConfig": ({"QGCGPSReceiverConfigConsumer"}, "QGCGPSReceiverConfigHeaderChecks"),
    "Px4Adapter": (
        {"QGCGPSPx4AdapterConsumer", "QGCGPSDriverReportsConsumer"},
        "QGCGPSDriverReportsHeaderChecks",
    ),
}


def run(command: list[str]) -> str:
    result = subprocess.run(
        command,
        check=False,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        timeout=120,
    )
    if result.returncode:
        raise ValueError(f"Command failed ({result.returncode}): {command}\n{result.stdout}")
    return result.stdout


def check_artifacts(build: Path, component: str, config: str) -> None:
    reply = build / ".cmake/api/v1/reply"
    indexes = sorted(reply.glob("index-*.json"))
    if not indexes:
        raise ValueError("CMake did not produce a file-api reply")
    index = json.loads(indexes[-1].read_text(encoding="utf-8"))
    model_file = index["reply"]["codemodel-v2"]["jsonFile"]
    model = json.loads((reply / model_file).read_text(encoding="utf-8"))
    configurations = [item for item in model["configurations"] if item["name"] == config]
    if len(configurations) != 1:
        raise ValueError(f"No unique file-api configuration matches {config!r}")
    for target in configurations[0]["targets"]:
        name = target["name"]
        if name in {"QGCGPSDriver", "px4-gpsdrivers"}:
            raise ValueError(f"Unexpected PX4 runtime target: {name}")
        if name == "QGCGPSPx4Adapter" and component != "Px4Adapter":
            raise ValueError("Unexpected compatibility adapter")
        if name != "QGCGPSReceiverConfig" or component == "ReceiverConfig":
            continue
        target_model = json.loads((reply / target["jsonFile"]).read_text(encoding="utf-8"))
        for artifact in target_model["artifacts"]:
            path = build / artifact["path"]
            if path.exists():
                raise ValueError(f"Unrequested configuration artifact was built: {path}")


def check_build(args: argparse.Namespace, build: Path) -> None:
    expected_tests, header_check = CASES[args.component]
    config = args.config or "Release"
    query = build / ".cmake/api/v1/query"
    query.mkdir(parents=True)
    (query / "codemodel-v2").touch()
    configure = [
        args.cmake,
        "-S",
        str(args.source_dir),
        "-B",
        str(build),
        "-G",
        args.generator,
        f"-DCMAKE_CXX_COMPILER={args.compiler}",
        f"-DCMAKE_BUILD_TYPE={config}",
        f"-DQGC_GPS_COMPONENTS={args.component}",
        "-DCMAKE_DISABLE_FIND_PACKAGE_Qt6=ON",
    ]
    if args.platform:
        configure.extend(["-A", args.platform])
    if args.toolset:
        configure.extend(["-T", args.toolset])
    run(configure)
    build_command = [args.cmake, "--build", str(build), "--config", config, "--parallel", "2"]
    # Build ALL, not just the consumer, to detect accidentally enabled sibling libraries.
    run(build_command)
    run([*build_command, "--target", header_check])
    test_command = [args.ctest, "--test-dir", str(build), "--build-config", config]
    discovery = json.loads(run([*test_command, "--show-only=json-v1"]))
    actual_tests = [item["name"] for item in discovery["tests"]]
    if sorted(actual_tests) != sorted(expected_tests):
        raise ValueError(f"Expected tests {sorted(expected_tests)}, got {sorted(actual_tests)}")
    check_artifacts(build, args.component, config)
    run([*test_command, "--output-on-failure", "--no-tests=error"])


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--component", choices=CASES, required=True)
    parser.add_argument("--source-dir", type=Path, required=True)
    parser.add_argument("--build-root", type=Path, required=True)
    parser.add_argument("--cmake", required=True)
    parser.add_argument("--ctest", required=True)
    parser.add_argument("--generator", required=True)
    parser.add_argument("--compiler", required=True)
    parser.add_argument("--config", default="Release")
    parser.add_argument("--platform", default="")
    parser.add_argument("--toolset", default="")
    args = parser.parse_args()
    try:
        args.build_root.mkdir(parents=True, exist_ok=True)
        # Each invocation owns its temporary build, so stale artifacts cannot mask a regression.
        with tempfile.TemporaryDirectory(
            prefix=f"{args.component}-", dir=args.build_root
        ) as directory:
            check_build(args, Path(directory))
    except (OSError, ValueError, KeyError, subprocess.TimeoutExpired) as error:
        print(f"{args.component}: {error}", file=sys.stderr)
        return 1
    print(f"{args.component}: isolated build, headers, artifacts and test selection passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
