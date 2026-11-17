"""Build the verified package set that publish_set.py hands to the publishing jobs."""

from __future__ import annotations

import hashlib
import json
from typing import TYPE_CHECKING

from ci.publish.publish_decide import REQUIRED_WORKFLOWS
from ci.publish.publish_set import MANIFEST_NAME, PublishSet, load_set
from qgc_tools.build_artifacts import PUBLISHED_BUILDS, companion_names

if TYPE_CHECKING:
    from collections.abc import Collection, Mapping
    from pathlib import Path

REPO = "mavlink/qgroundcontrol"
MASTER = "refs/heads/master"
# Upstream master commits: 36436db3 is seven commits behind 13faea02.
OLDER_COMMIT = "36436db319c731b70ab25dd3178896c4c22c75f6"
COMMIT = "13faea0245c22a1d763b96721fe5c1711edc52e2"
# The v5.1.5 tag's commit, on Stable_V5.1.
TAG_COMMIT = "3a67d31f0c36bf3fe38ec52970d250a89d0aaf67"
# 13faea02's publishing runs.
RUN_IDS = {
    "Linux": 42969994287,
    "Windows": 42969994035,
    "MacOS": 42969993894,
    "Android": 42969993919,
}
NAMES = sorted(build.name for build in PUBLISHED_BUILDS)


def write_set(
    directory: Path,
    *,
    ref: str = MASTER,
    commit: str = COMMIT,
    repo: str = REPO,
    without: Collection[str] = (),
    conclusions: Mapping[str, str] | None = None,
    update_files: bool = False,
) -> PublishSet:
    """Write the published packages of *commit* into *directory* and load them back.

    All six, or those that a branch built which has none of the names in *without*. Every
    workflow's run passed at its second attempt, unless *conclusions* says how one concluded.
    With *update_files* the AppImages embed update information, so each has its ``.zsync``.
    """
    directory.mkdir(parents=True, exist_ok=True)
    packages = []
    for build in sorted(PUBLISHED_BUILDS, key=lambda build: build.name):
        if build.name in without:
            continue
        data = f"{build.name} of {commit}".encode()
        digest = hashlib.sha256(data).hexdigest()
        (directory / build.name).write_bytes(data)
        (directory / f"{build.name}.sha256").write_text(
            f"{digest}  {build.name}\n", encoding="utf-8", newline="\n"
        )
        companions = []
        for name in companion_names(build.name) if update_files else ():
            content = f"zsync: 0.6.2\nFilename: {build.name}\n\nblock sums of {commit}".encode()
            (directory / name).write_bytes(content)
            companions.append(
                {
                    "name": name,
                    "sha256": hashlib.sha256(content).hexdigest(),
                    "size": len(content),
                }
            )
        packages.append(
            {
                "name": build.name,
                "sha256": digest,
                "size": len(data),
                "workflow": build.workflow,
                "run_id": RUN_IDS[build.workflow],
                "companions": companions,
            }
        )
    manifest = {
        "schema_version": 2,
        "repository": repo,
        "ref": ref,
        "commit": commit,
        "runs": [
            {
                "workflow": workflow,
                "path": path,
                "id": RUN_IDS[workflow],
                "attempt": 2,
                "conclusion": (conclusions or {}).get(workflow, "success"),
            }
            for workflow, path in REQUIRED_WORKFLOWS.items()
        ],
        "packages": packages,
    }
    (directory / MANIFEST_NAME).write_text(json.dumps(manifest), encoding="utf-8")
    return load_set(directory, ref=ref, commit=commit)
