"""What the build workflows mark for publishing, what publishes it, and what lists it agree.

The platform workflows mark packages (attest-and-upload's ``publish`` input), and each package's
marker lists its workflow's part of ``qgc_tools.build_artifacts.PUBLISHED_BUILDS``, which is what
the publisher then requires of that workflow's run. publish.yml follows the workflows that mark
any and publish_decide.py requires a run of each, the Pages downloads page lists
``PUBLISHED_BUILDS``, and the user guide links the master builds.
"""

from __future__ import annotations

import re
from typing import Any

from ci.planning.android_matrix import build_matrix
from ci.publish.publish_decide import REQUIRED_WORKFLOWS
from qgc_tools.build_artifacts import (
    ARTIFACT_RETENTION_DAYS,
    BUILDS_URL,
    PUBLISHED_BUILDS,
    companion_names,
    is_publishable_name,
    workflow_packages,
)

from ..._helpers import (
    REPO_ROOT,
    evaluate_expression,
    get_steps,
    load_action,
    load_workflow,
    workflow_names,
)

ATTEST_ACTION = "./.github/actions/attest-and-upload"
DOWNLOAD_DOCS = REPO_ROOT / "docs/en/qgc-user-guide/getting_started/download_and_install.md"
_PLACEHOLDER = re.compile(r"\$\{\{\s*([\w.]+)\s*\}\}")


def _matrix_entries(job: dict[str, Any]) -> list[dict[str, Any]]:
    """The job's matrix entries on a push: its static include list, or the planned Android legs."""
    matrix = (job.get("strategy") or {}).get("matrix")
    include = matrix.get("include") if isinstance(matrix, dict) else None
    if isinstance(include, list):
        return include
    if include == "${{ fromJSON(needs.changes.outputs.matrix) }}":
        return build_matrix(is_pr=False)
    return [{}]


def _marked_names(workflow_file: str) -> set[str]:
    """The file names *workflow_file* marks for publishing on a push."""
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
                if evaluate_expression(str(step["with"]["publish"]), context) is not True:
                    continue
                name = _PLACEHOLDER.sub(
                    lambda match, ctx=context: str(evaluate_expression(match[1], ctx)),
                    step["with"]["artifact-name"],
                )
                names.add(name)
    return names


def _marking_workflows() -> dict[str, set[str]]:
    """Every workflow file that marks a package, with the names it marks."""
    marked = {name: _marked_names(name) for name in workflow_names()}
    return {name: names for name, names in marked.items() if names}


def test_published_builds_are_what_the_workflows_mark() -> None:
    # By workflow: a package's marker lists the packages PUBLISHED_BUILDS gives its workflow,
    # and the publisher requires that workflow's run to provide exactly those. A package marked
    # but not listed, or listed but not marked, would fail every publish of the commit.
    marked = {
        str(load_workflow(file)["name"]): sorted(names)
        for file, names in _marking_workflows().items()
    }
    assert marked == {workflow: workflow_packages(workflow) for workflow in marked}
    assert sum(map(len, marked.values())) == len(PUBLISHED_BUILDS)
    # The publisher refuses a name this checkout does not list, whatever a package asks for.
    assert all(is_publishable_name(name) for names in marked.values() for name in names)


def test_the_build_attests_the_metadata_that_marks_a_package_and_names_its_job() -> None:
    # publish_set.py verifies the provenance of the build metadata like the package's, and looks
    # up the job the metadata names: the id the caller passes.
    steps = get_steps(load_action("attest-and-upload"))
    prepare = next(index for index, step in enumerate(steps) if step.get("id") == "prepare")
    assert steps[prepare]["env"]["JOB_ID"] == "${{ inputs.job-id }}"
    assert '--job-id "${JOB_ID}"' in steps[prepare]["run"]

    provenance = [
        index
        for index, step in enumerate(steps)
        if str(step.get("uses", "")).startswith("actions/attest@")
        and "sbom-path" not in step["with"]
    ]
    assert len(provenance) == 1
    # The package, the metadata and the companion file, as the prepare step lists them.
    assert steps[provenance[0]]["with"]["subject-path"] == (
        "${{ steps.prepare.outputs.provenance-subjects }}"
    )
    # The metadata is written once, before it is attested, and uploaded as attested.
    upload = next(
        index
        for index, step in enumerate(steps)
        if str(step.get("uses", "")).startswith("actions/upload-artifact@")
    )
    assert prepare < provenance[0] < upload
    assert all("run" not in step for step in steps[prepare + 1 : upload])
    uploaded = steps[upload]["with"]["path"]
    assert "${{ steps.prepare.outputs.metadata }}" in uploaded
    assert "${{ steps.prepare.outputs.companion }}" in uploaded
    assert steps[prepare]["env"]["COMPANION"] == "${{ inputs.companion-path }}"
    assert '--companion "${COMPANION}"' in steps[prepare]["run"]
    # publish_set.py tells a run whose artifacts expired by this retention.
    assert f"|| '{ARTIFACT_RETENTION_DAYS}' }}}}" in steps[upload]["with"]["retention-days"]


def test_an_appimage_that_embeds_update_information_is_uploaded_with_its_update_file() -> None:
    # The update information names `<package>.zsync` on the latest GitHub Release, so a
    # release needs that file whenever the information is embedded, and only then. The
    # AppImage's step passes it under the condition that embeds the information.
    job = load_workflow("linux.yml")["jobs"]["build"]
    information = job["env"]["QGC_APPIMAGE_UPDATE_INFORMATION"]
    step = next(step for step in job["steps"] if step.get("uses") == ATTEST_ACTION)
    companion = step["with"]["companion-path"]

    for package in workflow_packages("Linux"):
        name = package.removesuffix(".AppImage")
        (update_file,) = companion_names(package)
        embedded = f"gh-releases-zsync|mavlink|qgroundcontrol|latest|{update_file}"
        assert information.strip() == (
            "${{ github.repository == 'mavlink/qgroundcontrol' && "
            "format('gh-releases-zsync|mavlink|qgroundcontrol|latest|{0}.AppImage.zsync', "
            "matrix.package) || '' }}"
        )
        assert embedded.endswith(f"|{name}.AppImage.zsync")
        assert companion == (
            "${{ env.QGC_APPIMAGE_UPDATE_INFORMATION != '' && "
            "format('{0}/build/{1}.AppImage.zsync', runner.temp, matrix.package) || '' }}"
        )
    # No other package has a companion file.
    others = [build.name for build in PUBLISHED_BUILDS if build.workflow != "Linux"]
    assert not any(companion_names(name) for name in others)


def test_every_step_that_can_mark_a_package_passes_its_job_s_id() -> None:
    # `job.check_run_id` is the job's id in the jobs API. The `job` context is documented for
    # a workflow's steps, not for the steps of a composite action, so the caller passes it.
    passing = set()
    for file in workflow_names():
        for job in load_workflow(file)["jobs"].values():
            for step in job.get("steps") or []:
                if step.get("uses") != ATTEST_ACTION or str(step["with"]["publish"]) == "false":
                    continue
                assert step["with"].get("job-id") == "${{ job.check_run_id }}", (file, step["name"])
                passing.add(file)
    assert passing == set(_marking_workflows())


def test_publish_follows_and_requires_exactly_the_workflows_that_mark_packages() -> None:
    """A workflow left out would publish nothing; one that marks nothing would block every publish."""
    marking = {
        str(load_workflow(name)["name"]): f".github/workflows/{name}"
        for name in _marking_workflows()
    }
    followed = load_workflow("publish.yml")["on"]["workflow_run"]["workflows"]
    assert sorted(followed) == sorted(marking)
    # The path is the workflow file each package's attestation must name as its signer.
    assert marking == REQUIRED_WORKFLOWS


def test_user_guide_links_the_published_master_builds() -> None:
    docs = DOWNLOAD_DOCS.read_text(encoding="utf-8")
    linked = set(re.findall(rf"{re.escape(BUILDS_URL)}/master/([\w.-]+)\)", docs))
    assert linked == {build.name for build in PUBLISHED_BUILDS}
