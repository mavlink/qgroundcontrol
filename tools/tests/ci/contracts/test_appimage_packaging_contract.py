"""Contracts for AppImage packaging in the containerized Linux builds."""

from __future__ import annotations

import json

from ..._helpers import REPO_ROOT, get_step, get_steps, load_action, load_workflow

LINUX_STEPS = get_steps(load_workflow("linux.yml"), "build")


def test_appimagelint_reports_with_fuse_only_where_enabled() -> None:
    create_appimage = (REPO_ROOT / "cmake/install/CreateAppImage.cmake").read_text()
    variants = json.loads((REPO_ROOT / "deploy/docker/variants.json").read_text())["variants"]

    assert 'COMMAND "${CMAKE_COMMAND}" -E env --unset=APPIMAGE_EXTRACT_AND_RUN' in create_appimage
    assert '--json-report "${APPIMAGELINT_REPORT_PATH}"' in create_appimage
    assert "AppImage passed validation" not in create_appimage
    assert "AppImageLint completed; review findings" in create_appimage
    docker_upload = get_step(
        get_steps(load_workflow("docker.yml"), "build"), "Upload build artifact"
    )
    assert "${{ github.workspace }}/build/appimagelint-report.json" in docker_upload["with"]["path"]
    linux_upload = get_step(LINUX_STEPS, "Attest and Upload")
    assert "/build/appimagelint-report.json" in linux_upload["with"]["additional-artifact-paths"]
    upload_action = load_action("attest-and-upload")
    assert "additional-artifact-paths" in upload_action["inputs"]
    saved = get_step(get_steps(upload_action), "Save artifact")["with"]["path"].splitlines()
    assert "${{ inputs.additional-artifact-paths }}" in saved

    fuse_by_id = {variant["id"]: variant["fuse"] for variant in variants}
    assert all(
        fuse_by_id[variant] for variant in {"ubuntu", "ubuntu-2204", "ubuntu-2604", "debian"}
    )
    assert not any(fuse_by_id[variant] for variant in {"fedora", "arch"})


def test_official_appimages_publish_delta_update_metadata() -> None:
    install = (REPO_ROOT / "cmake/install/Install.cmake").read_text()
    create_appimage = (REPO_ROOT / "cmake/install/CreateAppImage.cmake").read_text()

    assert 'CACHE STRING "AppImage update information passed to appimagetool' in install
    assert (
        'set(QGC_APPIMAGE_UPDATE_INFORMATION \\"${QGC_APPIMAGE_UPDATE_INFORMATION}\\")' in install
    )
    assert '--updateinformation "${QGC_APPIMAGE_UPDATE_INFORMATION}"' in create_appimage
    assert 'NOT EXISTS "${APPIMAGE_PATH}.zsync"' in create_appimage
    # Only the upstream repository's AppImages point their updater at its releases.
    update_information = load_workflow("linux.yml")["jobs"]["build"]["env"][
        "QGC_APPIMAGE_UPDATE_INFORMATION"
    ]
    assert update_information.startswith("${{ github.repository == 'mavlink/qgroundcontrol' && ")
    assert "'gh-releases-zsync|mavlink|qgroundcontrol|latest|{0}.AppImage.zsync'" in (
        update_information
    )
    expected = "${{ env.QGC_APPIMAGE_UPDATE_INFORMATION }}"
    configure = get_step(LINUX_STEPS, "Configure")
    assert configure["with"]["extra-args"] == f"-DQGC_APPIMAGE_UPDATE_INFORMATION={expected}"
    verify = get_step(LINUX_STEPS, "Verify AppImage")
    assert verify["with"]["expected-appimage-update-information"] == expected
    assert "expected-appimage-update-information" in load_action("verify-executable")["inputs"]
    zsync = get_step(LINUX_STEPS, "Attest and Upload")["with"]["additional-artifact-paths"]
    assert "'{0}/build/{1}.AppImage.zsync'" in zsync
