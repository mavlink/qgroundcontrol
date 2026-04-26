#!/usr/bin/env python3
"""Check attestation conditions and resolve SBOM paths."""

from __future__ import annotations

import argparse
import os
from pathlib import Path

from ci_bootstrap import ensure_tools_dir

ensure_tools_dir(__file__)

from common.gh_actions import write_github_output  # noqa: E402


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--subject-path", required=True)
    parser.add_argument("--subject-name", required=True)
    parser.add_argument("--scan-path", default="")
    parser.add_argument("--sbom-format", default="spdx-json")
    parser.add_argument("--runner-temp", required=True)
    args = parser.parse_args()

    subject = Path(args.subject_path)

    # PR builds aren't released; skip the SBOM scan + Sigstore signing entirely.
    if os.environ.get("GITHUB_EVENT_NAME") == "pull_request":
        print("Skipping attestation for pull_request build")
        write_github_output({"skip": "true"})
        return

    if not subject.exists():
        print(f"::warning::Artifact not found: {subject}")
        write_github_output({"skip": "true"})
        return

    scan_path = args.scan_path or str(subject.parent)
    suffix = "cdx.json" if args.sbom_format == "cyclonedx-json" else "spdx.json"
    sbom_path = str(Path(args.runner_temp) / f"{args.subject_name}.sbom.{suffix}")

    write_github_output({
        "skip": "false",
        "scan-path": scan_path,
        "sbom-path": sbom_path,
    })
    print(f"Will attest: {subject}")
    print(f"Scan path: {scan_path}")
    print(f"SBOM path: {sbom_path}")


if __name__ == "__main__":
    main()
