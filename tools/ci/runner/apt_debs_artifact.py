#!/usr/bin/env python3
"""Restore a Copilot session's apt downloads from the newest master Copilot Setup Steps artifact.

GitHub evicts the setup workflow's apt download cache from the repository's full cache pool within
hours, so its master runs also upload the .debs as an artifact. apt checks only the size of a .deb
already in its archive directory, so only that workflow's own default-branch runs are trusted. apt
downloads whatever the artifact lacks, and a failure only warns.
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

from ci.runner.apt_install_retry import archives_dir

ARTIFACT_NAME = "copilot-apt-debs"
PRODUCER_WORKFLOW = ".github/workflows/copilot-setup-steps.yml"
TRUSTED_EVENTS = frozenset({"push", "schedule", "workflow_dispatch"})
# Copilot gives the restore step two minutes; a stalled transfer leaves the rest to apt.
DOWNLOAD_ATTEMPTS = 2
DOWNLOAD_TIMEOUT_SECONDS = 50.0
_DEB_NAME = re.compile(r"[A-Za-z0-9][A-Za-z0-9.+~%_-]*\.deb")
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
    """Only the setup workflow's own default-branch runs, never a fork's or a pull request's."""
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


def extract_debs(archive: Path, destination: Path) -> int:
    """Copy the archive's top-level .deb files into *destination*, skipping every other member."""
    destination.mkdir(parents=True, exist_ok=True)
    count = 0
    with zipfile.ZipFile(archive) as bundle:
        for member in bundle.infolist():
            if member.is_dir() or not _DEB_NAME.fullmatch(member.filename):
                continue
            with (
                bundle.open(member) as source,
                (destination / member.filename).open("wb") as target,
            ):
                shutil.copyfileobj(source, target)
            count += 1
    return count


def restore(repo: str, destination: Path) -> int:
    """Return the number of .debs restored; 0 when master has no usable artifact."""
    artifact = find_artifact(repo)
    if artifact is None:
        print(f"No {ARTIFACT_NAME} artifact from a master Copilot Setup Steps run.")
        return 0
    with tempfile.TemporaryDirectory() as temporary:
        archive = Path(temporary) / "debs.zip"
        download_artifact_zip(
            repo,
            artifact["id"],
            archive,
            attempts=DOWNLOAD_ATTEMPTS,
            timeout=DOWNLOAD_TIMEOUT_SECONDS,
        )
        count = extract_debs(archive, destination)
    print(f"Restored {count} .deb files from run {artifact['workflow_run']['id']}.")
    return count


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--destination", type=Path, default=None)
    args = parser.parse_args(argv)
    try:
        restore(require_repository(), args.destination or archives_dir())
    except _ERRORS as error:
        gh_warning(f"apt download artifact unavailable; apt downloads every package: {error}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
