#!/usr/bin/env python3
"""Attestation helpers: gate SBOM signing and resolve artifact paths."""

from __future__ import annotations

import argparse
import os
import sys
from pathlib import Path

from ci_bootstrap import ensure_tools_dir

ensure_tools_dir(__file__)

from common.gh_actions import gh_error, gh_warning, write_github_output


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

    args = parser.parse_args()
    {"check": cmd_check, "resolve-path": cmd_resolve_path}[args.command](args)


if __name__ == "__main__":
    main()
