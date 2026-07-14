"""Contracts for AppImage packaging in the containerized Linux builds."""

from __future__ import annotations

import json

from _helpers import REPO_ROOT


def test_appimagelint_mounts_target_with_fuse_in_container() -> None:
    entrypoint = (REPO_ROOT / "deploy/docker/entrypoint.sh").read_text()
    create_appimage = (REPO_ROOT / "cmake/install/CreateAppImage.cmake").read_text()
    docker_workflow = (REPO_ROOT / ".github/workflows/docker.yml").read_text()
    linux_workflow = (REPO_ROOT / ".github/workflows/linux.yml").read_text()
    upload_action = (REPO_ROOT / ".github/actions/attest-and-upload/action.yml").read_text()
    variants = json.loads((REPO_ROOT / "deploy/docker/variants.json").read_text())["variants"]

    assert "export APPIMAGE_EXTRACT_AND_RUN=1" in entrypoint
    assert 'COMMAND "${CMAKE_COMMAND}" -E env --unset=APPIMAGE_EXTRACT_AND_RUN' in create_appimage
    assert '--json-report "${APPIMAGELINT_REPORT_PATH}"' in create_appimage
    assert "AppImage passed validation" not in create_appimage
    assert "AppImageLint completed; review findings" in create_appimage
    assert "build/appimagelint-report.json" in docker_workflow
    assert "build/appimagelint-report.json" in linux_workflow
    assert "additional-artifact-paths:" in upload_action
    assert "${{ inputs.additional-artifact-paths }}" in upload_action

    fuse_by_id = {variant["id"]: variant["fuse"] for variant in variants}
    assert all(
        fuse_by_id[variant] for variant in {"ubuntu", "ubuntu-2204", "ubuntu-2604", "debian"}
    )
    assert not any(fuse_by_id[variant] for variant in {"fedora", "arch"})
