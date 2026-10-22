#!/usr/bin/env python3
"""Prepare a build artifact for attestation and upload.

``prepare`` resolves the artifact, verifies or writes its SHA-256 sidecar, writes the
CPM dependency SBOM and build-identity manifest beside it, and emits the paths the
attestation and upload steps consume.

``checksum`` verifies or writes only the SHA-256 sidecar of a package uploaded outside
attest-and-upload; ``--require-existing`` fails when the packager did not emit one.
"""

from __future__ import annotations

import argparse
import json
import os
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import _bootstrap as _bootstrap
from common.gh_actions import gh_error, write_github_output
from common.io import ensure_sha256_sidecar, sha256_file, write_json

from ci.security.generate_cpm_sbom import generate_sbom


def resolve_artifact(override: str, default: str) -> str:
    path = override or default
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
    return path


def write_dependency_sbom(build_dir: Path, output: Path) -> None:
    sbom = generate_sbom(build_dir)
    count = len(sbom["components"])
    if not count:
        gh_error("attest-and-upload: CPM dependency SBOM contains no components")
        sys.exit(1)
    output.write_text(json.dumps(sbom, indent=2), encoding="utf-8")
    print(f"SBOM written to {output} ({count} components)")


def write_metadata(source: Path, build_dir: Path) -> Path:
    """Record producer identity alongside the package, without runner-local secrets."""
    digest = sha256_file(source)
    config = {}
    cache = build_dir / "CMakeCache.txt"
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
    return path


def cmd_prepare(args: argparse.Namespace) -> None:
    artifact = resolve_artifact(args.override, args.default)
    source = Path(artifact)
    try:
        checksum = ensure_sha256_sidecar(source)
    except (OSError, ValueError) as exc:
        gh_error(f"attest-and-upload: {exc}")
        sys.exit(1)

    parent = str(source.parent)
    dependency_sbom = f"{parent}/{args.subject_name}.dependencies.cdx.json"
    build_dir = Path(args.build_dir)
    write_dependency_sbom(build_dir, Path(dependency_sbom))
    metadata = write_metadata(source, build_dir)

    # PR builds aren't released; skip the SBOM scan + Sigstore signing entirely.
    skip = os.environ.get("GITHUB_EVENT_NAME") == "pull_request"
    sbom_path = str(Path(args.runner_temp) / f"{args.subject_name}.sbom.spdx.json")
    if skip:
        print("Skipping attestation for pull_request build")
    else:
        print(f"Will attest: {artifact}")
        print(f"Scan path: {args.build_dir}")
        print(f"SBOM path: {sbom_path}")

    write_github_output(
        {
            "path": artifact,
            "parent": parent,
            "checksum": str(checksum),
            "dependency-sbom": dependency_sbom,
            "metadata": str(metadata),
            "skip": "true" if skip else "false",
            "scan-path": args.build_dir,
            "sbom-path": sbom_path,
        }
    )


def cmd_checksum(args: argparse.Namespace) -> None:
    source = Path(args.source_path)
    checksum = source.with_name(f"{source.name}.sha256")
    try:
        if args.require_existing and not checksum.is_file():
            raise FileNotFoundError(f"Checksum sidecar not found: {checksum}")
        ensure_sha256_sidecar(source, checksum)
    except (OSError, ValueError) as exc:
        gh_error(f"checksum: {exc}")
        sys.exit(1)

    print(f"Verified {checksum}")
    write_github_output({"checksum": str(checksum)})


def main(argv: list[str] | None = None) -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)

    p_prepare = sub.add_parser(
        "prepare", help="Resolve the artifact and write its checksum, dependency SBOM and metadata"
    )
    p_prepare.add_argument("--override", default="", help="Artifact path overriding --default")
    p_prepare.add_argument("--default", required=True, help="Default artifact path")
    p_prepare.add_argument("--build-dir", required=True, help="CMake build directory to inventory")
    p_prepare.add_argument("--subject-name", required=True, help="Attestation subject name")
    p_prepare.add_argument("--runner-temp", required=True, help="Directory for the SPDX SBOM")
    p_prepare.set_defaults(func=cmd_prepare)

    p_checksum = sub.add_parser(
        "checksum", help="Verify the artifact's <name>.sha256 sidecar, writing it when absent"
    )
    p_checksum.add_argument("--source-path", required=True, help="Artifact to checksum")
    p_checksum.add_argument(
        "--require-existing",
        action="store_true",
        help="Fail instead of writing the sidecar when the packager did not emit one",
    )
    p_checksum.set_defaults(func=cmd_checksum)

    args = parser.parse_args(argv)
    args.func(args)


if __name__ == "__main__":
    main()
