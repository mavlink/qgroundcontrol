"""Repository-wide policy checks for GitHub Actions workflows."""

from __future__ import annotations

import json
import re
from typing import TYPE_CHECKING, Any

import yaml
from _helpers import REPO_ROOT

if TYPE_CHECKING:
    from pathlib import Path

WORKFLOWS_DIR = REPO_ROOT / ".github" / "workflows"
WORKFLOWS = sorted(WORKFLOWS_DIR.glob("*.yml"))
HARDEN_RUNNER = "step-security/harden-runner@v2"
CI_SCRIPTS_WORKFLOW = WORKFLOWS_DIR / "ci-scripts.yml"
QT_INSTALL_ACTION = REPO_ROOT / ".github" / "actions" / "qt-install" / "action.yml"
BUILD_RESULTS_WORKFLOW = WORKFLOWS_DIR / "build-results.yml"
PRECOMMIT_WORKFLOW = WORKFLOWS_DIR / "pre-commit.yml"
CLUSTERFUZZLITE_WORKFLOW = WORKFLOWS_DIR / "clusterfuzzlite.yml"
CODECOV_ACTIONS = (
    REPO_ROOT / ".github" / "actions" / "coverage" / "action.yml",
    REPO_ROOT / ".github" / "actions" / "test-report" / "action.yml",
)
IOS_WORKFLOW = WORKFLOWS_DIR / "ios.yml"
ANDROID_WORKFLOW = WORKFLOWS_DIR / "android.yml"
LINUX_WORKFLOW = WORKFLOWS_DIR / "linux.yml"
MACOS_WORKFLOW = WORKFLOWS_DIR / "macos.yml"
RELEASE_WORKFLOW = WORKFLOWS_DIR / "release.yml"
RUNNER_IMAGES_WORKFLOW = WORKFLOWS_DIR / "runner-images.yml"
WINDOWS_WORKFLOW = WORKFLOWS_DIR / "windows.yml"
DOCKER_WORKFLOW = WORKFLOWS_DIR / "docker.yml"
BUILD_CONFIG = REPO_ROOT / ".github" / "build-config.json"
CMAKE_LISTS = REPO_ROOT / "CMakeLists.txt"
CMAKE_CONFIGURE_ACTION = REPO_ROOT / ".github" / "actions" / "cmake-configure" / "action.yml"
CMAKE_PRESETS = REPO_ROOT / "CMakePresets.json"
IOS_CMAKE_PRESETS = REPO_ROOT / "cmake" / "presets" / "iOS.json"
DOWNLOAD_ARTIFACTS_ACTION = (
    REPO_ROOT / ".github" / "actions" / "download-all-artifacts" / "action.yml"
)
PLATFORM_WORKFLOW_FILES = {
    "Linux": "linux.yml",
    "Windows": "windows.yml",
    "MacOS": "macos.yml",
    "Android": "android.yml",
    "iOS": "ios.yml",
}
AQT_SOURCE = (
    "git+https://github.com/miurahr/aqtinstall.git@9e49c82edc6d946db376dec907cca5b4b486eec5"
)


def _load_workflow(path: Path) -> dict[str, Any]:
    document = yaml.safe_load(path.read_text(encoding="utf-8"))
    assert isinstance(document, dict), f"{path.name} must contain a YAML mapping"
    if True in document and "on" not in document:
        document["on"] = document.pop(True)
    return document


def test_ci_scripts_uses_full_checkout_for_repository_contract_tests() -> None:
    workflow = _load_workflow(CI_SCRIPTS_WORKFLOW)
    required_trigger_paths = {
        "CMakeLists.txt",
        "CMakePresets.json",
        "cmake/install/CreateAppImage.cmake",
        "cmake/presets/**",
    }
    for event_name in ("pull_request", "push"):
        assert required_trigger_paths <= set(workflow["on"][event_name]["paths"])

    steps = workflow["jobs"]["test-ci-scripts"]["steps"]
    checkout = next(step for step in steps if step.get("uses") == "actions/checkout@v7")
    checkout_inputs = checkout.get("with", {})
    assert "sparse-checkout" not in checkout_inputs, (
        "repository contract tests require a full checkout so new file dependencies cannot drift"
    )
    assert checkout_inputs["persist-credentials"] is False


def test_platform_workflows_inherit_the_shared_aqt_source() -> None:
    qt_install = yaml.safe_load(QT_INSTALL_ACTION.read_text(encoding="utf-8"))
    build_config = json.loads(BUILD_CONFIG.read_text())
    assert build_config["qt"]["aqt_source"] == AQT_SOURCE
    assert qt_install["inputs"]["aqt-source"]["default"] == ""

    for workflow_path in WORKFLOWS:
        workflow_text = workflow_path.read_text(encoding="utf-8")
        assert "aqt-source:" not in workflow_text
        assert "aqt_source:" not in workflow_text

    for action_name in ("build-setup", "qt-android", "qt-ios"):
        action_text = (REPO_ROOT / ".github" / "actions" / action_name / "action.yml").read_text(
            encoding="utf-8"
        )
        assert "aqt-source:" not in action_text


def _configure_preset_names(path: Path, visited: set[Path] | None = None) -> set[str]:
    visited = visited or set()
    path = path.resolve()
    if path in visited:
        return set()
    visited.add(path)

    document = json.loads(path.read_text(encoding="utf-8"))
    names = {preset["name"] for preset in document.get("configurePresets", [])}
    for include in document.get("include", []):
        names |= _configure_preset_names(path.parent / include, visited)
    return names


def test_ci_cmake_configuration_uses_repository_presets() -> None:
    action = yaml.safe_load(CMAKE_CONFIGURE_ACTION.read_text(encoding="utf-8"))
    assert action["inputs"]["preset"]["required"] is True

    preset_names = _configure_preset_names(CMAKE_PRESETS)
    legacy_inputs = {"build-type", "coverage", "testing", "use-qt-cmake"}
    configure_steps = []
    for workflow_path in WORKFLOWS:
        workflow = _load_workflow(workflow_path)
        for job in workflow.get("jobs", {}).values():
            matrix = job.get("strategy", {}).get("matrix", {})
            for entry in matrix.get("include", []) if isinstance(matrix, dict) else []:
                if "preset" in entry:
                    assert entry["preset"] in preset_names
            for step in job.get("steps", []):
                if step.get("uses") != "./.github/actions/cmake-configure":
                    continue
                configure_steps.append((workflow_path.name, step))
                inputs = step.get("with", {})
                assert inputs.get("preset"), f"{workflow_path.name}: configure step needs a preset"
                assert not legacy_inputs & inputs.keys(), (
                    f"{workflow_path.name}: preset-owned configure inputs must not be overridden"
                )
                selection = str(inputs["preset"])
                if "${{" not in selection:
                    assert selection in preset_names

    assert configure_steps
    referenced = set()
    for _workflow_name, step in configure_steps:
        selection = str(step["with"]["preset"])
        referenced |= set(re.findall(r"(?:Android|Linux|Windows|macOS|iOS)[\w-]*", selection))
    assert referenced <= preset_names, f"Unknown configure presets: {referenced - preset_names}"


def test_android_cmake_targets_the_configured_minimum_sdk() -> None:
    build_config = json.loads(BUILD_CONFIG.read_text(encoding="utf-8"))
    assert int(build_config["android"]["min_sdk"]) < int(build_config["android"]["platform"])

    workflow = _load_workflow(ANDROID_WORKFLOW)
    configure = next(
        step
        for step in workflow["jobs"]["build"]["steps"]
        if step.get("uses") == "./.github/actions/cmake-configure"
    )
    assert configure["env"]["ANDROID_MIN_SDK"] == "${{ steps.setup.outputs.android_min_sdk }}"


def test_ios_ca_bundle_uses_an_immutable_pinned_snapshot() -> None:
    build_config = json.loads(BUILD_CONFIG.read_text(encoding="utf-8"))
    gstreamer = build_config["gstreamer"]
    assert re.fullmatch(
        r"https://curl\.se/ca/cacert-\d{4}-\d{2}-\d{2}\.pem", gstreamer["ca_bundle_url"]
    )
    assert re.fullmatch(r"[a-f0-9]{64}", gstreamer["ca_bundle_sha256"])

    ios_cmake = (REPO_ROOT / "cmake" / "GStreamer" / "platform" / "IOS.cmake").read_text(
        encoding="utf-8"
    )
    normalized_cmake = " ".join(ios_cmake.split())
    assert 'GET "${QGC_BUILD_CONFIG_CONTENT}" "gstreamer" "ca_bundle_url"' in normalized_cmake
    assert 'URLS "${_ca_url}"' in normalized_cmake


def _version_tuple(value: str) -> tuple[int, int, int]:
    parts = [int(part) for part in value.split(".")]
    assert 1 <= len(parts) <= 3
    padded = [*parts, 0, 0][:3]
    return padded[0], padded[1], padded[2]


def test_cmake_minimum_versions_stay_in_sync() -> None:
    build_config = json.loads(BUILD_CONFIG.read_text(encoding="utf-8"))
    expected = _version_tuple(build_config["build"]["cmake_minimum_version"])

    cmake_lists_match = re.search(
        r"^cmake_minimum_required\(VERSION ([0-9.]+)\)",
        CMAKE_LISTS.read_text(encoding="utf-8"),
        re.MULTILINE,
    )
    assert cmake_lists_match
    assert _version_tuple(cmake_lists_match.group(1)) == expected

    required = json.loads(CMAKE_PRESETS.read_text(encoding="utf-8"))["cmakeMinimumRequired"]
    actual = (required["major"], required["minor"], required.get("patch", 0))
    assert actual == expected, "CMakePresets.json must match build-config"


def test_ios_deployment_target_stays_in_sync() -> None:
    build_config = json.loads(BUILD_CONFIG.read_text(encoding="utf-8"))
    expected = build_config["apple"]["ios_deployment_target"]
    presets = json.loads(IOS_CMAKE_PRESETS.read_text(encoding="utf-8"))["configurePresets"]
    ios_base = next(preset for preset in presets if preset["name"] == "ios-base")
    assert ios_base["cacheVariables"]["CMAKE_OSX_DEPLOYMENT_TARGET"] == expected


def test_platform_workflow_name_consumers_stay_in_sync() -> None:
    configured = {
        name.strip()
        for name in json.loads(BUILD_CONFIG.read_text())["build"]["platform_workflows"].split(",")
        if name.strip()
    }
    assert configured == set(PLATFORM_WORKFLOW_FILES)
    for name, filename in PLATFORM_WORKFLOW_FILES.items():
        assert _load_workflow(WORKFLOWS_DIR / filename)["name"] == name

    build_results = _load_workflow(BUILD_RESULTS_WORKFLOW)
    trigger = build_results["on"]
    assert set(trigger["workflow_run"]["workflows"]) == configured | {"pre-commit"}

    release_jobs = _load_workflow(RELEASE_WORKFLOW)["jobs"]
    wait_step = next(
        step
        for step in release_jobs["wait-for-builds"]["steps"]
        if "filter-workflow-names" in step.get("with", {})
    )
    wait_inputs = wait_step["with"]
    assert set(wait_inputs["filter-workflow-names"].splitlines()) == configured
    assert wait_inputs["filter-workflow-events"] == "workflow_dispatch"
    assert wait_inputs["sha"] == "${{ github.sha }}"

    downloader = next(
        step
        for step in release_jobs["upload-artifacts"]["steps"]
        if step.get("uses") == "./.github/actions/download-all-artifacts"
    )
    assert downloader["with"]["head-sha"] == "${{ github.sha }}"
    assert downloader["with"]["event"] == "workflow_dispatch"

    action = yaml.safe_load(DOWNLOAD_ARTIFACTS_ACTION.read_text())
    defaults = set(action["inputs"]["workflows"]["default"].split(","))
    assert defaults == configured


def test_baseline_updates_reuse_the_platform_workflow_gate() -> None:
    workflow_text = BUILD_RESULTS_WORKFLOW.read_text(encoding="utf-8")
    assert "require-success: 'true'" in workflow_text
    assert "check_baseline_ready.py" not in workflow_text
    assert "steps.ready.outputs.ready" not in workflow_text
    assert workflow_text.count("runs-file: ${{ runner.temp }}/workflow-runs-cache.json") == 2


def test_build_results_comment_survives_optional_artifact_failures() -> None:
    workflow = _load_workflow(BUILD_RESULTS_WORKFLOW)
    steps = workflow["jobs"]["post-pr-comment"]["steps"]
    steps_by_name = {step["name"]: step for step in steps}
    workflow_text = BUILD_RESULTS_WORKFLOW.read_text(encoding="utf-8")

    assert "pr.head?.sha !== run.head_sha" in workflow_text
    assert "Build results unavailable" not in workflow_text
    assert steps_by_name["Download artifacts from all platform workflows"]["continue-on-error"]
    assert steps_by_name["Collect artifact sizes"]["continue-on-error"]
    report_env = steps_by_name["Generate combined report"]["env"]
    assert report_env["ARTIFACT_DOWNLOAD_FAILED"] == "${{ steps.download.outcome == 'failure' }}"


def test_precommit_checks_only_the_pull_request_diff() -> None:
    workflow = _load_workflow(PRECOMMIT_WORKFLOW)
    steps = workflow["jobs"]["pre-commit"]["steps"]
    checkout = next(step for step in steps if step.get("uses") == "actions/checkout@v7")
    run_step = next(step for step in steps if step.get("name") == "Run pre-commit")

    assert checkout["with"]["fetch-depth"] == 0
    assert 'if [[ "${GITHUB_EVENT_NAME}" == "pull_request" ]]' in run_step["run"]
    assert "args+=(--changed)" in run_step["run"]
    assert 'python3 ./tools/pre_commit.py "${args[@]}"' in run_step["run"]


def test_docker_packages_use_git_versions_and_require_expected_artifacts() -> None:
    workflow = _load_workflow(DOCKER_WORKFLOW)
    steps = workflow["jobs"]["build"]["steps"]
    steps_by_name = {step["name"]: step for step in steps}
    checkout = steps_by_name["Checkout"]

    assert checkout["with"]["fetch-depth"] == 0, "git describe requires tags for package versions"
    assert "--required" in steps_by_name["Find build artifact"]["run"]
    assert "--required" in steps_by_name["Find native package"]["run"]


def test_windows_installers_use_git_versions() -> None:
    workflow = _load_workflow(WINDOWS_WORKFLOW)
    steps = workflow["jobs"]["build"]["steps"]
    steps_by_name = {step["name"]: step for step in steps}

    assert steps_by_name["Checkout repo"]["with"]["fetch-depth"] == 0, (
        "git describe requires tags for installer versions"
    )
    assert steps_by_name["Create Installer"]["with"]["target"] == "qgc-package"


def test_clusterfuzzlite_runs_only_for_code_changes() -> None:
    workflow = _load_workflow(CLUSTERFUZZLITE_WORKFLOW)
    assert "schedule" not in workflow["on"]
    assert set(workflow["jobs"]) == {"fuzz", "continuous-build"}

    fuzz_steps = workflow["jobs"]["fuzz"]["steps"]
    build_step = next(step for step in fuzz_steps if step["name"] == "Build fuzzers")
    assert build_step["with"]["keep-unaffected-fuzz-targets"] is True


def test_runner_images_is_manual_only() -> None:
    workflow = _load_workflow(RUNNER_IMAGES_WORKFLOW)
    assert workflow["on"] == {"workflow_dispatch": None}


def test_linux_codecov_uploads_use_oidc() -> None:
    workflow = _load_workflow(LINUX_WORKFLOW)
    assert workflow["jobs"]["debug-validation"]["permissions"]["id-token"] == "write"
    assert "CODECOV_TOKEN" not in LINUX_WORKFLOW.read_text(encoding="utf-8")

    upload_steps = []
    for action_path in CODECOV_ACTIONS:
        action = yaml.safe_load(action_path.read_text(encoding="utf-8"))
        upload_steps.extend(
            step
            for step in action["runs"]["steps"]
            if step.get("uses") == "codecov/codecov-action@v7"
        )
        assert "codecov-token" not in action["inputs"]

    assert upload_steps
    assert all(step["with"]["use_oidc"] is True for step in upload_steps)
    coverage_upload = upload_steps[0]["with"]
    assert coverage_upload["disable_search"] is True
    assert coverage_upload["fail_ci_if_error"] is False


def test_release_uses_native_macos_and_windows_sboms() -> None:
    macos_text = MACOS_WORKFLOW.read_text(encoding="utf-8")
    windows_text = WINDOWS_WORKFLOW.read_text(encoding="utf-8")
    release_text = RELEASE_WORKFLOW.read_text(encoding="utf-8")

    assert "subject-name: QGroundControl-macos" in macos_text
    assert "subject-name: ${{ matrix.package }}-windows" in windows_text
    assert "event: workflow_dispatch" in release_text
    for sbom_name in (
        "QGroundControl-macos.sbom.spdx.json",
        "QGroundControl-installer-AMD64-windows.sbom.spdx.json",
        "QGroundControl-installer-ARM64-windows.sbom.spdx.json",
        "QGroundControl-installer-AMD64-ARM64-windows.sbom.spdx.json",
    ):
        assert sbom_name in release_text
    assert "id: attest-macos" not in release_text
    assert "id: attest-windows" not in release_text


def test_release_dispatches_platform_builds_and_publishes_checksums() -> None:
    release_text = RELEASE_WORKFLOW.read_text(encoding="utf-8")

    for workflow in ("linux.yml", "windows.yml", "macos.yml", "android.yml", "ios.yml"):
        assert workflow in release_text
    assert 'gh workflow run "$workflow" --ref "$RELEASE_TAG"' in release_text
    assert 'checksum="${file}.sha256"' in release_text
    assert "Required checksum not found" in release_text


def test_ios_builds_device_and_simulator_targets() -> None:
    workflow = _load_workflow(IOS_WORKFLOW)
    job = workflow["jobs"]["build"]

    assert job["runs-on"] == "${{ matrix.runner }}"
    assert job["strategy"]["matrix"]["include"] == [
        {
            "target": "device",
            "runner": "macos-15",
            "build_type": "Release",
            "preset": "iOS",
        },
        {
            "target": "simulator",
            "runner": "macos-15-intel",
            "build_type": "Debug",
            "preset": "iOS-debug",
        },
    ]

    steps = {step["name"]: step for step in job["steps"]}
    configure = steps["Configure"]["with"]
    assert "matrix.target == 'device'" in configure["generator"]
    assert "-DCMAKE_OSX_SYSROOT=iphonesimulator" in configure["extra-args"]
    assert "-DCMAKE_OSX_ARCHITECTURES=x86_64" in configure["extra-args"]
    assert (
        'lipo "$APP_PATH/$PACKAGE" -verify_arch x86_64' in steps["Verify Simulator Bundle"]["run"]
    )

    for step_name in (
        "Import App Store signing certificate",
        "Download App Store provisioning profiles",
        "Select App Store provisioning profile",
        "Package IPA",
        "Attest and Upload",
        "Upload to TestFlight",
    ):
        assert "matrix.target == 'device'" in steps[step_name]["if"]


def test_workflows_have_explicit_permissions() -> None:
    for path in WORKFLOWS:
        workflow = _load_workflow(path)
        assert "permissions" in workflow, (
            f"{path.name} must declare an explicit permission baseline"
        )


def test_executable_workflow_jobs_are_bounded_and_hardened() -> None:
    for path in WORKFLOWS:
        workflow = _load_workflow(path)
        jobs = workflow.get("jobs", {})
        assert isinstance(jobs, dict), f"{path.name} jobs must be a mapping"

        for job_name, job in jobs.items():
            if not isinstance(job, dict) or "steps" not in job:
                continue
            steps = job["steps"]
            assert isinstance(steps, list) and steps, f"{path.name}:{job_name} must have steps"
            assert "timeout-minutes" in job, f"{path.name}:{job_name} must set timeout-minutes"
            assert steps[0].get("uses") == HARDEN_RUNNER, (
                f"{path.name}:{job_name} must run {HARDEN_RUNNER} before other steps"
            )

            for step in steps:
                if step.get("uses") != "actions/checkout@v7":
                    continue
                persist = step.get("with", {}).get("persist-credentials")
                assert persist is False, (
                    f"{path.name}:{job_name} checkout must set persist-credentials: false"
                )
