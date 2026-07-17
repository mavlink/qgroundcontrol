#!/usr/bin/env python3
"""Attestation helpers: gate SBOM signing and resolve artifact paths."""

from __future__ import annotations

import argparse
import hmac
import os
import re
import sys
from pathlib import Path

from ci_bootstrap import ensure_tools_dir

ensure_tools_dir(__file__)

from common.gh_actions import gh_error, gh_warning, write_github_output
from common.io import sha256_file

_SHA256_LINE = re.compile(r"([0-9a-fA-F]{64}) [ *](.+)")


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
    write_github_output({"path": str(path)})


def cmd_checksum(args: argparse.Namespace) -> None:
    source = Path(args.source_path)
    if not source.is_file():
        gh_error(f"Checksum source is not a file: {source}")
        sys.exit(1)

    checksum_path = Path(f"{source}.sha256")
    actual_digest = sha256_file(source)

    if checksum_path.is_file():
        lines = [
            line
            for line in checksum_path.read_text(encoding="utf-8-sig").splitlines()
            if line.strip()
        ]
        match = _SHA256_LINE.fullmatch(lines[0]) if len(lines) == 1 else None
        if match is None:
            gh_error(f"Invalid SHA-256 checksum file: {checksum_path}")
            sys.exit(1)

        expected_digest, listed_name = match.groups()
        if Path(listed_name).name.casefold() != source.name.casefold():
            gh_error(f"Checksum file names '{listed_name}', expected '{source.name}'")
            sys.exit(1)
        if not hmac.compare_digest(expected_digest.casefold(), actual_digest):
            gh_error(f"SHA-256 checksum mismatch for: {source}")
            sys.exit(1)

    checksum_path.write_text(f"{actual_digest}  {source.name}\n", encoding="utf-8")
    write_github_output({"path": str(checksum_path)})


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

    p_checksum = sub.add_parser(
        "checksum", help="Verify or create a canonical SHA-256 checksum sidecar"
    )
    p_checksum.add_argument("--source-path", required=True)

    args = parser.parse_args()
    {
        "check": cmd_check,
        "checksum": cmd_checksum,
        "resolve-path": cmd_resolve_path,
    }[args.command](args)


if __name__ == "__main__":
    main()
