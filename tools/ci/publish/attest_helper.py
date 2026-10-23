#!/usr/bin/env python3
"""Prepare a build artifact for attestation and upload.

``prepare`` resolves the artifact (default: $RUNNER_TEMP/build/<artifact name>), verifies or
writes its SHA-256 sidecar, writes the CPM dependency SBOM and build-identity manifest beside it,
emits the paths and attestation subject the later steps consume plus whether to publish to AWS,
and adds an Artifact section (file, size, SHA-256) to the job summary.

``checksum`` verifies or writes only the SHA-256 sidecar of a package uploaded outside
attest-and-upload; CPack must emit the ``.deb`` and ``.rpm`` sidecars, so theirs must exist.
"""

from __future__ import annotations

import argparse
import os
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import _bootstrap as _bootstrap
from common.format import format_bytes, format_count
from common.gh_actions import env_flag, gh_error, write_github_output, write_step_summary
from common.io import ensure_sha256_sidecar, sha256_file, write_json
from common.markdown import code_span, summary_section

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
    write_json(output, sbom)
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


def publishes_to_aws(*, upload_aws: bool, has_credentials: bool) -> bool:
    """Whether to publish to S3: requested and credentialed pushes or tag dispatches in mavlink.

    The event, ref type, and owner come from the runner's GITHUB_* variables.
    """
    event = os.environ.get("GITHUB_EVENT_NAME", "")
    release_event = event == "push" or (
        event == "workflow_dispatch" and os.environ.get("GITHUB_REF_TYPE") == "tag"
    )
    return (
        upload_aws
        and has_credentials
        and release_event
        and os.environ.get("GITHUB_REPOSITORY_OWNER") == "mavlink"
    )


def cmd_prepare(args: argparse.Namespace) -> None:
    subject_name = args.subject_name or args.package_name
    build_dir = args.runner_temp / "build"
    artifact = resolve_artifact(args.override, str(build_dir / args.artifact_name))
    source = Path(artifact)
    try:
        checksum = ensure_sha256_sidecar(source)
    except (OSError, ValueError) as exc:
        gh_error(f"attest-and-upload: {exc}")
        sys.exit(1)

    dependency_sbom = source.parent / f"{subject_name}.dependencies.cdx.json"
    write_dependency_sbom(build_dir, dependency_sbom)
    metadata = write_metadata(source, build_dir)
    sbom_path = args.runner_temp / f"{subject_name}.sbom.spdx.json"
    publish_aws = publishes_to_aws(
        upload_aws=env_flag("UPLOAD_AWS"), has_credentials=env_flag("HAS_AWS_CREDENTIALS")
    )
    print(f"Artifact: {artifact}\nSBOM path: {sbom_path}\nPublish to AWS: {publish_aws}")

    write_github_output(
        {
            "path": artifact,
            "checksum": str(checksum),
            "dependency-sbom": str(dependency_sbom),
            "metadata": str(metadata),
            "sbom-path": str(sbom_path),
            "subject-name": subject_name,
            "publish-aws": "true" if publish_aws else "false",
        }
    )
    size = source.stat().st_size
    write_step_summary(
        summary_section(
            "Artifact",
            [
                ("File", code_span(source.name)),
                ("Size", f"{format_bytes(size)} ({format_count(size)} bytes)"),
                ("SHA-256", code_span(checksum.read_text(encoding="utf-8").split()[0])),
                ("Attestation subject", code_span(subject_name)),
            ],
        )
    )


def cmd_checksum(args: argparse.Namespace) -> None:
    source = Path(args.source_path)
    checksum = source.with_name(f"{source.name}.sha256")
    try:
        if source.suffix in {".deb", ".rpm"} and not checksum.is_file():
            raise FileNotFoundError(f"Checksum sidecar not found: {checksum}")
        ensure_sha256_sidecar(source, checksum)
    except (OSError, ValueError) as exc:
        gh_error(f"checksum: {exc}")
        sys.exit(1)

    print(f"Verified {checksum}")
    write_github_output({"checksum": str(checksum)})


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)

    p_prepare = sub.add_parser(
        "prepare", help="Resolve the artifact and write its checksum, dependency SBOM and metadata"
    )
    p_prepare.add_argument(
        "--artifact-name", required=True, help="Artifact file name in <runner temp>/build"
    )
    p_prepare.add_argument("--override", default="", help="Artifact path overriding the default")
    p_prepare.add_argument("--package-name", required=True, help="GitHub artifact name")
    p_prepare.add_argument(
        "--subject-name", default="", help="Attestation subject name (default: --package-name)"
    )
    runner_temp = os.environ.get("RUNNER_TEMP")
    p_prepare.add_argument(
        "--runner-temp",
        type=Path,
        default=runner_temp,
        required=not runner_temp,
        help="Holds the build/ directory to inventory and the SPDX SBOM (default: $RUNNER_TEMP)",
    )
    p_prepare.set_defaults(func=cmd_prepare)

    p_checksum = sub.add_parser(
        "checksum",
        help="Verify the artifact's <name>.sha256 sidecar, writing it when absent (not for .deb/.rpm)",
    )
    p_checksum.add_argument("--source-path", required=True, help="Artifact to checksum")
    p_checksum.set_defaults(func=cmd_checksum)

    args = parser.parse_args(argv)
    args.func(args)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
