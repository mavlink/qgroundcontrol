"""The packages S3 serves from builds/<ref>/ match qgc_tools.build_artifacts.PUBLISHED_BUILDS.

The Pages downloads page lists PUBLISHED_BUILDS; the platform workflows decide what is uploaded
(attest-and-upload with upload-aws), and the user guide links the master builds.
"""

from __future__ import annotations

import re
from typing import Any

from qgc_tools.build_artifacts import BUILDS_URL, PUBLISHED_BUILDS

from ..._helpers import REPO_ROOT, evaluate_expression, load_workflow

ATTEST_ACTION = "./.github/actions/attest-and-upload"
DOWNLOAD_DOCS = REPO_ROOT / "docs/en/qgc-user-guide/getting_started/download_and_install.md"
_PLACEHOLDER = re.compile(r"\$\{\{\s*([\w.]+)\s*\}\}")


def _matrix_entries(job: dict[str, Any]) -> list[dict[str, Any]]:
    """The job's static matrix include entries; one unknown entry for a planned matrix."""
    matrix = (job.get("strategy") or {}).get("matrix")
    if isinstance(matrix, dict) and isinstance(matrix.get("include"), list):
        return matrix["include"]
    return [{}]


def _uploaded_names(workflow_file: str) -> set[str]:
    workflow = load_workflow(workflow_file)
    names = set()
    for job in workflow["jobs"].values():
        for step in job.get("steps") or []:
            if step.get("uses") != ATTEST_ACTION:
                continue
            for entry in _matrix_entries(job):
                context = {
                    "matrix": entry,
                    "env": {**workflow.get("env", {}), **job.get("env", {})},
                }
                # A planned matrix's values are unknown here, so only a literal 'false' opts out.
                upload = evaluate_expression(str(step["with"].get("upload-aws", "true")), context)
                if upload is False or upload == "false":
                    continue
                name = _PLACEHOLDER.sub(
                    lambda match, ctx=context: str(evaluate_expression(match[1], ctx)),
                    step["with"]["artifact-name"],
                )
                names.add(name)
    return names


def test_published_builds_are_what_the_workflows_upload() -> None:
    uploaded = set().union(
        *(
            _uploaded_names(name)
            for name in ("linux.yml", "windows.yml", "macos.yml", "android.yml", "ios.yml")
        )
    )
    assert uploaded == {build.name for build in PUBLISHED_BUILDS}


def test_user_guide_links_the_published_master_builds() -> None:
    docs = DOWNLOAD_DOCS.read_text(encoding="utf-8")
    linked = set(re.findall(rf"{re.escape(BUILDS_URL)}/master/([\w.-]+)\)", docs))
    assert linked == {build.name for build in PUBLISHED_BUILDS}
