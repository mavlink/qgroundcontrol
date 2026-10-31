"""The desktop workflows audit and boot what they ship, and run the GStreamer tests on Windows."""

from __future__ import annotations

import re

from ..._helpers import REPO_ROOT, get_step, get_steps, load_workflow

GSTREAMER_README = REPO_ROOT / "src/VideoManager/GStreamer/README.md"


def test_windows_audits_installed_dlls_between_install_and_boot() -> None:
    steps = get_steps(load_workflow("windows.yml"), "build")
    names = [step.get("name") for step in steps]
    install = names.index("Install, upgrade, and verify installer")
    audit = get_step(steps, "Audit installed DLL dependencies")
    assert names.index(audit["name"]) == install + 1
    assert names[install + 2] == "Verify installed executable"
    assert audit["if"] == steps[install]["if"] == steps[install + 2]["if"]
    assert "tools/ci/packaging/pe_dependency_audit.py" in audit["run"]
    # verify-installer.ps1 -Phase Install exports the upgraded install's directory.
    assert '--install-dir "${QGC_INSTALL_TEST_DIR}"' in audit["run"]
    installer_test = (REPO_ROOT / "deploy/windows/verify-installer.ps1").read_text(encoding="utf-8")
    assert '"QGC_INSTALL_TEST_DIR=$installDir"' in installer_test


def test_windows_runs_the_gstreamer_test_classes_on_warp() -> None:
    workflow = load_workflow("windows.yml")
    job = workflow["jobs"]["gstreamer-tests"]
    assert job["needs"] == "changes"
    steps = job["steps"]
    assert get_step(steps, "Configure")["with"]["preset"] == "Windows-debug"
    assert (
        "-DQGC_ENABLE_GST_VIDEOSTREAMING=ON" in get_step(steps, "Configure")["with"]["extra-args"]
    )

    environment = get_step(steps, "Point CTest at the GStreamer SDK")
    assert "--export-gstreamer-env" in environment["run"]
    tests = get_step(steps, "Run GStreamer tests")
    assert tests["uses"] == "./.github/actions/run-unit-tests"
    assert "steps.gst-env.outcome == 'success'" in tests["if"]
    assert tests["env"] == {"QSG_RHI_PREFER_SOFTWARE_RENDERER": "1"}
    assert tests["with"]["include-labels"] == "Unit|Integration"
    # The test classes the GStreamer README lists, and the video classes outside it.
    pattern = re.compile(tests["with"]["tests-regex"])
    documented = re.findall(
        r"^\| `(\w+Test)` \|", GSTREAMER_README.read_text(encoding="utf-8"), re.M
    )
    assert documented
    assert all(pattern.match(name) for name in documented)
    assert all(pattern.match(name) for name in ("VideoManagerTest", "SubtitleWriterTest"))
    assert not pattern.match("ParameterManagerTest")

    gate = workflow["jobs"]["windows-ok"]
    assert "gstreamer-tests" in gate["needs"]


def test_linux_boots_the_appimage_in_clean_distribution_containers() -> None:
    workflow = load_workflow("linux.yml")
    job = workflow["jobs"]["appimage-distros"]
    assert job["needs"] == ["changes", "build"]
    images = [leg["image"] for leg in job["strategy"]["matrix"]["include"]]
    # GitHub-hosted runners are exempt from Docker Hub's pull limit; ECR Public's anonymous data quota is not
    # enough for their shared IPs.
    assert job["runs-on"] == "ubuntu-24.04"
    assert images == [
        "docker.io/library/ubuntu:24.04",
        "docker.io/library/debian:13",
    ]
    x86_64 = next(
        leg
        for leg in workflow["jobs"]["build"]["strategy"]["matrix"]["include"]
        if leg["arch"] == "linux_gcc_64"
    )
    download = get_step(job["steps"], "Download AppImage")
    assert download["with"]["name"] == x86_64["package"]
    boot = job["steps"][-1]
    assert "tools/ci/packaging/appimage_container_boot.py" in boot["run"]
    assert f"/appimage/{x86_64['package']}.AppImage" in boot["run"]
    assert boot["env"] == {"IMAGE": "${{ matrix.image }}"}


def test_macos_audits_the_bundle_inside_the_dmg_it_boots() -> None:
    script = (REPO_ROOT / "tools/ci/packaging/verify_executable.py").read_text(encoding="utf-8")
    assert "from ci.packaging.macho_bundle_audit import audit_bundle" in script
    steps = get_steps(load_workflow("macos.yml"), "build")
    assert get_step(steps, "Verify DMG executable")["with"]["type"] == "dmg"
