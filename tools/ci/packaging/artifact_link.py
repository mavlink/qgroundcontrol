#!/usr/bin/env python3
"""Link an uploaded workflow artifact from the job summary.

Runs after actions/upload-artifact with its artifact-url output, so the summary section written by
the step before the upload points at its diagnostics. An empty URL (nothing uploaded) writes nothing.
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import _bootstrap as _bootstrap
from common.gh_actions import env_default, write_step_summary
from common.markdown import code_span, escape_text

_ARTIFACT_URL = re.compile(r"https://[^\s()<>]+")


def artifact_link(label: str, name: str, url: str) -> str:
    """Return a summary paragraph linking artifact *name* at *url*."""
    return f"{escape_text(label)}: [{code_span(name)}]({url})\n\n"


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--label", required=True, help="Text before the link")
    parser.add_argument("--name", default=env_default("ARTIFACT_NAME"), help="Artifact name")
    parser.add_argument(
        "--url", default=env_default("ARTIFACT_URL"), help="upload-artifact's artifact-url output"
    )
    args = parser.parse_args(argv)
    if not args.url:
        return 0
    if not _ARTIFACT_URL.fullmatch(args.url):
        parser.error(f"--url must be an https URL: {args.url!r}")
    if not args.name:
        parser.error("--name (or ARTIFACT_NAME) is required with a URL")
    print(f"{args.label}: {args.url}")
    write_step_summary(artifact_link(args.label, args.name, args.url))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
