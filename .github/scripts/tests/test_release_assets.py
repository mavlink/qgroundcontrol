"""Tests for release_assets.py and its release workflow contract."""

from __future__ import annotations

import json
import shutil
from typing import TYPE_CHECKING

import pytest
import yaml
from _helpers import REPO_ROOT
from android_matrix import build_matrix
from common.io import ensure_sha256_sidecar, sha256_file
from release_assets import (
    REQUIRED_DEPENDENCY_SBOMS,
    REQUIRED_PACKAGES,
    REQUIRED_PLATFORM_SBOMS,
    collect_release_assets,
)

if TYPE_CHECKING:
    from pathlib import Path

ANDROID_RELEASE_ARTIFACT = "QGroundControl-linux-arm64-v8a-armeabi-v7a"
PACKAGE_PATHS = (
    "QGroundControl-x86_64/QGroundControl-x86_64.AppImage",
    "QGroundControl-aarch64/QGroundControl-aarch64.AppImage",
    "QGroundControl/QGroundControl.dmg",
    "QGroundControl-installer-AMD64/QGroundControl-installer-AMD64.exe",
    "QGroundControl-installer-ARM64/QGroundControl-installer-ARM64.exe",
    "QGroundControl-installer-AMD64-ARM64/QGroundControl-installer-AMD64-ARM64.exe",
    f"{ANDROID_RELEASE_ARTIFACT}/QGroundControl.apk",
    "QGroundControl-ios/QGroundControl.ipa",
)

SPDX_DOCUMENT = '{"spdxVersion": "SPDX-2.3"}\n'
CYCLONEDX_DOCUMENT = '{"bomFormat": "CycloneDX"}\n'
CYCLONEDX_DEPENDENCY_DOCUMENT = (
    '{"bomFormat": "CycloneDX", "components": [{"name": "dependency"}]}\n'
)


def _create_complete_release(tmp_path: Path) -> tuple[Path, list[Path]]:
    artifacts = tmp_path / "artifacts"
    for relative_path in PACKAGE_PATHS:
        package = artifacts / relative_path
        package.parent.mkdir(parents=True, exist_ok=True)
        package.write_bytes(relative_path.encode())
        ensure_sha256_sidecar(package)
        package.with_name(package.name + ".build.json").write_text(
            json.dumps(
                {
                    "schema_version": 1,
                    "commit": "abc",
                    "artifact": {"name": package.name, "sha256": sha256_file(package)},
                }
            )
        )
        if package.suffix == ".AppImage":
            package.with_name(f"{package.name}.zsync").write_text("zsync\n", encoding="utf-8")

    for sbom_name in REQUIRED_PLATFORM_SBOMS:
        sbom = artifacts / f"sbom-{sbom_name}" / sbom_name
        sbom.parent.mkdir(parents=True)
        sbom.write_text(SPDX_DOCUMENT, encoding="utf-8")

    for sbom_name in REQUIRED_DEPENDENCY_SBOMS:
        artifact_name = sbom_name.removesuffix(".dependencies.cdx.json")
        if artifact_name == "QGroundControl-linux":
            artifact_name = ANDROID_RELEASE_ARTIFACT
        sbom = artifacts / artifact_name / sbom_name
        sbom.parent.mkdir(parents=True, exist_ok=True)
        sbom.write_text(CYCLONEDX_DEPENDENCY_DOCUMENT, encoding="utf-8")

    source_sboms = [tmp_path / "sbom-source.spdx.json", tmp_path / "sbom-source.cdx.json"]
    source_sboms[0].write_text(SPDX_DOCUMENT, encoding="utf-8")
    source_sboms[1].write_text(CYCLONEDX_DOCUMENT, encoding="utf-8")
    return artifacts, source_sboms


def test_collect_release_assets_requires_complete_validated_set(tmp_path: Path) -> None:
    artifacts, source_sboms = _create_complete_release(tmp_path)

    assets = collect_release_assets(artifacts, source_sboms)

    assert len(assets) == 44
    assert assets == sorted(assets, key=lambda path: path.as_posix())
    assert all(path.is_file() for path in assets)


def test_release_android_selector_matches_official_producer_and_stable_sboms(
    tmp_path: Path,
) -> None:
    workflow = yaml.safe_load((REPO_ROOT / ".github/workflows/android.yml").read_text())
    package = workflow["jobs"]["build"]["env"]["PACKAGE"]
    official = next(leg for leg in build_matrix(is_pr=False) if leg["primary"])
    artifact_name = f"{package}-{official['host']}-{official['artifact_abi_suffix']}"
    assert artifact_name == ANDROID_RELEASE_ARTIFACT
    assert dict(REQUIRED_PACKAGES)["Android APK"] == f"{artifact_name}/*.apk"
    assert "QGroundControl-linux.sbom.spdx.json" in REQUIRED_PLATFORM_SBOMS
    assert "QGroundControl-linux.dependencies.cdx.json" in REQUIRED_DEPENDENCY_SBOMS

    artifacts, source_sboms = _create_complete_release(tmp_path)
    assets = collect_release_assets(artifacts, source_sboms, head_sha="abc")
    assert artifacts / artifact_name / "QGroundControl.apk" in assets
    assert artifacts / artifact_name / "QGroundControl.apk.sha256" in assets
    assert artifacts / artifact_name / "QGroundControl.apk.build.json" in assets
    assert artifacts / artifact_name / "QGroundControl-linux.dependencies.cdx.json" in assets
    assert any(path.name == "QGroundControl-linux.sbom.spdx.json" for path in assets)


@pytest.mark.parametrize(
    "other_artifact",
    [
        "QGroundControl-linux-arm64-v8a",
        "QGroundControl-mac-arm64-v8a",
        "QGroundControl-linux",
        "QGroundControl-linux-armeabi-v7a-arm64-v8a",
    ],
)
def test_release_never_substitutes_another_android_package(
    tmp_path: Path, other_artifact: str
) -> None:
    artifacts, source_sboms = _create_complete_release(tmp_path)
    (artifacts / ANDROID_RELEASE_ARTIFACT).rename(artifacts / other_artifact)

    with pytest.raises(ValueError, match=r"Android APK.*found 0"):
        collect_release_assets(artifacts, source_sboms, head_sha="abc")


def test_release_ignores_single_abi_pr_package_when_official_package_exists(tmp_path: Path) -> None:
    artifacts, source_sboms = _create_complete_release(tmp_path)
    single_abi = next(leg for leg in build_matrix(is_pr=True) if leg["primary"])
    pr_artifact = f"QGroundControl-{single_abi['host']}-{single_abi['artifact_abi_suffix']}"
    pr_directory = artifacts / pr_artifact
    pr_directory.mkdir()
    official_directory = artifacts / ANDROID_RELEASE_ARTIFACT
    for name in (
        "QGroundControl.apk",
        "QGroundControl.apk.sha256",
        "QGroundControl.apk.build.json",
    ):
        shutil.copyfile(official_directory / name, pr_directory / name)

    assets = collect_release_assets(artifacts, source_sboms, head_sha="abc")
    assert official_directory / "QGroundControl.apk" in assets
    assert not any(path.parent == pr_directory for path in assets)


def test_collect_release_assets_rejects_missing_or_bad_checksum(tmp_path: Path) -> None:
    artifacts, source_sboms = _create_complete_release(tmp_path)
    package = artifacts / PACKAGE_PATHS[0]
    checksum = package.with_name(f"{package.name}.sha256")
    checksum.unlink()

    with pytest.raises(FileNotFoundError):
        collect_release_assets(artifacts, source_sboms)

    checksum.write_text(f"{'0' * 64}  {package.name}\n", encoding="utf-8")
    with pytest.raises(ValueError, match="checksum mismatch"):
        collect_release_assets(artifacts, source_sboms)


def test_collect_release_assets_rejects_missing_sbom_or_duplicate_package(tmp_path: Path) -> None:
    artifacts, source_sboms = _create_complete_release(tmp_path)
    missing_sbom = next(artifacts.rglob(REQUIRED_PLATFORM_SBOMS[0]))
    missing_sbom.unlink()

    with pytest.raises(ValueError, match="found 0"):
        collect_release_assets(artifacts, source_sboms)

    missing_sbom.write_text(SPDX_DOCUMENT, encoding="utf-8")
    duplicate = artifacts / "QGroundControl-x86_64/duplicate.AppImage"
    duplicate.write_bytes(b"duplicate")
    ensure_sha256_sidecar(duplicate)
    duplicate.with_name(f"{duplicate.name}.zsync").write_text("zsync\n", encoding="utf-8")

    with pytest.raises(ValueError, match="found 2"):
        collect_release_assets(artifacts, source_sboms)


def test_collect_release_assets_rejects_invalid_sbom(tmp_path: Path) -> None:
    artifacts, source_sboms = _create_complete_release(tmp_path)
    source_sboms[1].write_text('{"bomFormat": "not-cyclonedx"}\n', encoding="utf-8")

    with pytest.raises(ValueError, match="Invalid CycloneDX SBOM"):
        collect_release_assets(artifacts, source_sboms)


def test_collect_release_assets_rejects_empty_dependency_sbom(tmp_path: Path) -> None:
    artifacts, source_sboms = _create_complete_release(tmp_path)
    dependency_sbom = next(artifacts.rglob(REQUIRED_DEPENDENCY_SBOMS[0]))
    dependency_sbom.write_text(CYCLONEDX_DOCUMENT, encoding="utf-8")

    with pytest.raises(ValueError, match="contains no components"):
        collect_release_assets(artifacts, source_sboms)


def test_release_uses_platform_sboms_without_reattesting() -> None:
    workflow = yaml.safe_load((REPO_ROOT / ".github/workflows/release.yml").read_text())
    upload = workflow["jobs"]["upload-artifacts"]
    steps = {step["name"]: step for step in upload["steps"]}

    assert upload["permissions"] == {"actions": "read", "contents": "write"}
    assert steps["Download artifacts"]["uses"] == "./.github/actions/download-all-artifacts"
    assert steps["Download artifacts"]["with"] == {
        "head-sha": "${{ github.sha }}",
        "workflows": "Linux,Windows,MacOS,Android,iOS",
        "event": "workflow_dispatch",
        "runs-file": "release-build-runs.json",
        "strict-runs": "true",
        "output-dir": "artifacts",
        "artifact-prefixes": "QGroundControl,sbom-",
    }
    step_names = list(steps)
    assert step_names.index("Generate source SBOM (CycloneDX)") < step_names.index(
        "Download artifacts"
    )
    assert (
        "${{ runner.temp }}/sbom-source.spdx.json"
        in steps["Generate source SBOM (SPDX)"]["with"]["output-file"]
    )
    assert "release_assets.py" in steps["Validate release assets"]["run"]
    assert '"${RUNNER_TEMP}/sbom-source.spdx.json"' in steps["Validate release assets"]["run"]
    assert "gh release upload" in steps["Upload to Release"]["run"]
    assert not any("attest-sbom" in str(step.get("uses", "")) for step in upload["steps"])


def test_attestation_actions_publish_checksum_and_resolved_sbom_output() -> None:
    ci_workflow = yaml.safe_load((REPO_ROOT / ".github/workflows/ci-scripts.yml").read_text())
    ci_steps = ci_workflow["jobs"]["test-ci-scripts"]["steps"]
    checkout = next(step for step in ci_steps if step["name"] == "Checkout")
    sparse_checkout = set(checkout["with"]["sparse-checkout"].splitlines())
    assert ".github" in sparse_checkout or ".github/actions/attest-and-upload" in sparse_checkout
    assert ".github" in sparse_checkout or ".github/actions/attest-sbom" in sparse_checkout

    upload_action = yaml.safe_load(
        (REPO_ROOT / ".github/actions/attest-and-upload/action.yml").read_text()
    )
    upload_steps = {step["name"]: step for step in upload_action["runs"]["steps"]}
    checksum = upload_steps["Verify or create SHA-256 checksum"]
    dependency_sbom = upload_steps["Generate CPM dependency SBOM"]
    dependency_attestation = upload_steps["Attest CPM dependency SBOM"]
    save = upload_steps["Save artifact"]
    aws_checksum = upload_steps["Upload checksum to AWS"]

    assert 'attest_helper.py" checksum' in checksum["run"]
    assert 'generate_cpm_sbom.py"' in dependency_sbom["run"]
    assert "--require-components" in dependency_sbom["run"]
    assert dependency_sbom["env"]["SBOM_PATH"] == (
        "${{ format('{0}/{1}.dependencies.cdx.json', steps.src.outputs.parent, "
        "inputs.subject-name || inputs.package-name) }}"
    )
    assert dependency_attestation["uses"] == "actions/attest@v4"
    assert dependency_attestation["with"] == {
        "subject-path": "${{ steps.src.outputs.path }}",
        "sbom-path": "${{ steps.dependency-sbom.outputs.path }}",
    }
    assert "${{ steps.checksum.outputs.path }}" in save["with"]["path"]
    assert "${{ steps.dependency-sbom.outputs.path }}" in save["with"]["path"]
    assert aws_checksum["with"]["artifact-path"] == "${{ steps.checksum.outputs.path }}"
    assert "github.event_name == 'workflow_dispatch'" in aws_checksum["if"]
    assert "github.ref_type == 'tag'" in aws_checksum["if"]

    sbom_action = yaml.safe_load((REPO_ROOT / ".github/actions/attest-sbom/action.yml").read_text())
    assert sbom_action["outputs"]["sbom-path"]["value"] == "${{ steps.check.outputs.sbom-path }}"


def test_release_rejects_manifest_from_another_commit(tmp_path):
    artifacts, sboms = _create_complete_release(tmp_path)
    with pytest.raises(ValueError, match="commit does not match"):
        collect_release_assets(artifacts, sboms, head_sha="different")


def test_release_rejects_manifest_for_another_payload(tmp_path):
    artifacts, sboms = _create_complete_release(tmp_path)
    package = artifacts / PACKAGE_PATHS[0]
    manifest = package.with_name(package.name + ".build.json")
    data = json.loads(manifest.read_text())
    data["artifact"]["sha256"] = "a" * 64
    manifest.write_text(json.dumps(data))
    with pytest.raises(ValueError, match="does not match package"):
        collect_release_assets(artifacts, sboms)
