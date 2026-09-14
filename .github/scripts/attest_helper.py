"""Attestation helpers: gate SBOM signing and resolve artifact paths."""

from __future__ import annotations

import argparse
import os
import sys
from pathlib import Path

from ci_bootstrap import ensure_tools_dir

ensure_tools_dir(__file__)

from common.gh_actions import gh_error, gh_warning, write_github_output
from common.io import ensure_sha256_sidecar, sha256_file, write_json


def cmd_check(args: argparse.Namespace) -> None:
    subject = Path(args.subject_path)

    # PR builds aren't released; skip the SBOM scan + Sigstore signing entirely.
    if os.environ.get("GITHUB_EVENT_NAME") == "pull_request":
        print("Skipping attestation for pull_request build")
        write_github_output({"skip": "true"})
        return

    if not subject.exists():
        gh_warning(f"Artifact not found: {subject}")
        write_github_output({"skip": "true"})
        return

    scan_path = args.scan_path or str(subject.parent)
    suffix = "cdx.json" if args.sbom_format == "cyclonedx-json" else "spdx.json"
    sbom_path = str(Path(args.runner_temp) / f"{args.subject_name}.sbom.{suffix}")

    write_github_output(
        {
            "skip": "false",
            "scan-path": scan_path,
            "sbom-path": sbom_path,
        }
    )
    print(f"Will attest: {subject}")
    print(f"Scan path: {scan_path}")
    print(f"SBOM path: {sbom_path}")


def cmd_resolve_path(args: argparse.Namespace) -> None:
    path = args.override or args.default
    p = Path(path)
    if not p.exists():
        gh_error(f"attest-and-upload: artifact not found at '{path}'")
        parent = p.parent
        if parent.is_dir():
            for entry in sorted(parent.iterdir()):
                print(f"  {entry.name}")
        else:
            print("(parent dir missing)")
        sys.exit(1)
    write_github_output({"parent": str(p.parent), "path": str(path)})


def cmd_checksum(args: argparse.Namespace) -> None:
    try:
        checksum = ensure_sha256_sidecar(Path(args.source_path))
    except (OSError, ValueError) as exc:
        gh_error(f"attest-and-upload: {exc}")
        sys.exit(1)
    write_github_output({"path": str(checksum)})


def cmd_metadata(args: argparse.Namespace) -> None:
    """Record producer identity alongside the package, without runner-local secrets."""
    source = Path(args.source_path)
    digest = sha256_file(source)
    config = {}
    cache = Path(args.build_dir) / "CMakeCache.txt"
    allowed = {
        "CMAKE_BUILD_TYPE",
        "CMAKE_CXX_COMPILER_ID",
        "CMAKE_SYSTEM_PROCESSOR",
        "QGC_BUILD_TESTING",
        "QGC_MACOS_UNIVERSAL_BUILD",
    }
    if cache.is_file():
        for line in cache.read_text(encoding="utf-8").splitlines():
            key = line.partition(":")[0]
            if key in allowed and "=" in line:
                config[key] = line.partition("=")[2]
    path = source.with_name(source.name + ".build.json")
    write_json(
        path,
        {
            "schema_version": 1,
            "repository": os.environ.get("GITHUB_REPOSITORY", ""),
            "commit": os.environ.get("GITHUB_SHA", ""),
            "ref": os.environ.get("GITHUB_REF", ""),
            "workflow": os.environ.get("GITHUB_WORKFLOW", ""),
            "job": os.environ.get("GITHUB_JOB", ""),
            "run_id": os.environ.get("GITHUB_RUN_ID", ""),
            "run_attempt": os.environ.get("GITHUB_RUN_ATTEMPT", ""),
            "runner_os": os.environ.get("RUNNER_OS", ""),
            "runner_arch": os.environ.get("RUNNER_ARCH", ""),
            "configuration": config,
            "artifact": {"name": source.name, "size": source.stat().st_size, "sha256": digest},
        },
        sort_keys=True,
    )
    write_github_output({"path": str(path)})


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)

    p_check = sub.add_parser("check", help="Decide whether to attest and emit SBOM paths")
    p_check.add_argument("--subject-path", required=True)
    p_check.add_argument("--subject-name", required=True)
    p_check.add_argument("--scan-path", default="")
    p_check.add_argument("--sbom-format", default="spdx-json")
    p_check.add_argument("--runner-temp", required=True)

    p_resolve = sub.add_parser(
        "resolve-path", help="Validate artifact source path and emit it as an output"
    )
    p_resolve.add_argument("--override", default="")
    p_resolve.add_argument("--default", required=True)

    p_checksum = sub.add_parser("checksum", help="Verify or create the artifact's SHA-256 sidecar")
    p_checksum.add_argument("--source-path", required=True)

    p_metadata = sub.add_parser(
        "metadata", help="Write package producer identity and configuration"
    )
    p_metadata.add_argument("--source-path", required=True)
    p_metadata.add_argument("--build-dir", required=True)

    args = parser.parse_args()
    {
        "check": cmd_check,
        "checksum": cmd_checksum,
        "resolve-path": cmd_resolve_path,
        "metadata": cmd_metadata,
    }[args.command](args)


if __name__ == "__main__":
    main()
