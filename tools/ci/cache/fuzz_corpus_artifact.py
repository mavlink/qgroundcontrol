#!/usr/bin/env python3
"""Restore the GPS fuzzers' corpus from the newest nightly GPS Standalone artifact.

The nightly run uploads the inputs its fuzzers kept as an artifact, because GitHub evicts caches
from the repository's full cache pool within hours. Only the workflow's own scheduled and manual
default-branch runs are trusted, only `<harness>/<input>` members are extracted, and a failure
only warns: the fuzzers then start from the committed seeds.
"""

from __future__ import annotations

import argparse
import re
import shutil
import subprocess
import sys
import tempfile
import zipfile
from pathlib import Path
from typing import Any

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import _bootstrap as _bootstrap
from common.gh_actions import (
    download_artifact_zip,
    gh_api_json,
    gh_warning,
    require_repository,
)

# Bump the version, here and in the workflow, to start over from the seeds.
ARTIFACT_NAME = "gps-fuzz-corpus-v1"
PRODUCER_WORKFLOW = ".github/workflows/gps-standalone.yml"
TRUSTED_EVENTS = frozenset({"schedule", "workflow_dispatch"})
DOWNLOAD_ATTEMPTS = 2
DOWNLOAD_TIMEOUT_SECONDS = 120.0
# The workflow's largest -max_len: no harness reads a longer input.
MAX_INPUT_BYTES = 65536
# libFuzzer names each input it keeps by its SHA-1.
INPUT_MEMBER = re.compile(r"[a-z0-9-]+/[0-9a-f]{40}")
_ERRORS = (
    OSError,
    ValueError,
    KeyError,
    TypeError,
    AttributeError,
    RuntimeError,
    zipfile.BadZipFile,
    subprocess.SubprocessError,
)


def trusted_producer(run: dict[str, Any], repository: dict[str, Any]) -> bool:
    """Only the workflow's own nightly runs on the default branch, never a fork's or a pull request's."""
    return (
        run.get("path") == PRODUCER_WORKFLOW
        and run.get("event") in TRUSTED_EVENTS
        and run.get("head_branch") == repository["default_branch"]
        and (run.get("head_repository") or {}).get("id") == repository["id"]
    )


def find_artifact(repo: str) -> dict[str, Any] | None:
    repository = gh_api_json(f"repos/{repo}")
    listing = gh_api_json(f"repos/{repo}/actions/artifacts?name={ARTIFACT_NAME}&per_page=30")
    for artifact in sorted(listing["artifacts"], key=lambda item: item["id"], reverse=True):
        if artifact.get("expired") or artifact.get("name") != ARTIFACT_NAME:
            continue
        producer = (artifact.get("workflow_run") or {}).get("id")
        if not isinstance(producer, int) or producer <= 0:
            continue
        if trusted_producer(gh_api_json(f"repos/{repo}/actions/runs/{producer}"), repository):
            return artifact
    return None


def extract_inputs(archive: Path, destination: Path) -> int:
    """Copy the archive's `<harness>/<input>` members into *destination*, skipping every other member."""
    count = 0
    with zipfile.ZipFile(archive) as bundle:
        for member in bundle.infolist():
            if (
                member.is_dir()
                or not INPUT_MEMBER.fullmatch(member.filename)
                or member.file_size > MAX_INPUT_BYTES
            ):
                continue
            target = destination / member.filename
            target.parent.mkdir(parents=True, exist_ok=True)
            with bundle.open(member) as source, target.open("wb") as output:
                shutil.copyfileobj(source, output)
            count += 1
    return count


def restore(repo: str, destination: Path) -> int:
    """Return the number of inputs restored; 0 when no nightly run left a usable artifact."""
    artifact = find_artifact(repo)
    if artifact is None:
        print(f"No {ARTIFACT_NAME} artifact from a nightly GPS Standalone run.")
        return 0
    with tempfile.TemporaryDirectory() as temporary:
        archive = Path(temporary) / "corpus.zip"
        download_artifact_zip(
            repo,
            artifact["id"],
            archive,
            attempts=DOWNLOAD_ATTEMPTS,
            timeout=DOWNLOAD_TIMEOUT_SECONDS,
        )
        count = extract_inputs(archive, destination)
    print(f"Restored {count} inputs from run {artifact['workflow_run']['id']}.")
    return count


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--destination", type=Path, required=True)
    args = parser.parse_args(argv)
    try:
        restore(require_repository(), args.destination)
    except _ERRORS as error:
        gh_warning(f"fuzzing corpus unavailable; the fuzzers start from their seeds: {error}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
