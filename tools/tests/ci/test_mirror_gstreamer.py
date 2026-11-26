"""Tests for mirror_gstreamer.py."""

from __future__ import annotations

import json
from typing import TYPE_CHECKING

import pytest
from ci.gstreamer import mirror_gstreamer
from ci.gstreamer.mirror_gstreamer import (
    Artifact,
    _expected_sha,
    artifacts_for,
    resolve_platforms,
)
from common.build_config import get_build_config_value
from common.gstreamer import mirror_url, pinned_checksum, platform_packages

from .._helpers import REPO_ROOT, get_step, get_steps, load_workflow

if TYPE_CHECKING:
    from pathlib import Path

PLATFORMS = list(platform_packages())


def _use_versions(
    monkeypatch: pytest.MonkeyPatch, tmp_path: Path, versions: dict[str, str]
) -> None:
    """Point CONFIG_FILE at the repository's build-config.json with *versions* as gstreamer.version."""
    config = json.loads((REPO_ROOT / ".github/build-config.json").read_text(encoding="utf-8"))
    config["gstreamer"]["version"] = versions
    path = tmp_path / "build-config.json"
    path.write_text(json.dumps(config), encoding="utf-8")
    monkeypatch.setenv("CONFIG_FILE", str(path))


def test_android_single_artifact() -> None:
    arts = artifacts_for("android", "1.28.3")
    assert len(arts) == 1
    assert arts[0].filename == "gstreamer-1.0-android-universal-1.28.3.tar.xz"
    assert (
        arts[0].s3_key()
        == "dependencies/gstreamer/android/gstreamer-1.0-android-universal-1.28.3.tar.xz"
    )


def test_macos_has_runtime_and_devel() -> None:
    names = sorted(a.filename for a in artifacts_for("macos", "1.28.3"))
    assert names == [
        "gstreamer-1.0-1.28.3-universal.pkg",
        "gstreamer-1.0-devel-1.28.3-universal.pkg",
    ]
    assert {a.s3_dir for a in artifacts_for("macos", "1.28.3")} == {"macos"}


def test_windows_has_both_arches() -> None:
    names = sorted(a.filename for a in artifacts_for("windows", "1.28.3"))
    assert names == [
        "gstreamer-1.0-msvc-arm64-1.28.3.exe",
        "gstreamer-1.0-msvc-x86_64-1.28.3.exe",
    ]


def test_ios_package() -> None:
    (art,) = artifacts_for("ios", "1.28.8")
    assert art.filename == "gstreamer-1.28.8-xcframework.tar.xz"
    assert art.url == f"https://gstreamer.freedesktop.org/data/pkg/ios/1.28.8/{art.filename}"


def test_unknown_platform_raises() -> None:
    with pytest.raises(ValueError, match="Unknown platform"):
        artifacts_for("plugin", "1.28.3")


def test_resolve_platforms_all() -> None:
    assert resolve_platforms("all") == PLATFORMS
    assert resolve_platforms("") == PLATFORMS


def test_resolve_platforms_subset_preserves_order() -> None:
    assert resolve_platforms("windows, android") == ["windows", "android"]


def test_resolve_platforms_rejects_unknown() -> None:
    with pytest.raises(ValueError, match="Unknown platform"):
        resolve_platforms("android,plugin")


def test_artifacts_are_uploaded_where_the_build_reads_the_mirror() -> None:
    # cmake/GStreamer/Packages.cmake falls back to mirror_url(); test_gstreamer.py holds those two together.
    bucket_url = f"https://{mirror_gstreamer.S3_BUCKET}.s3.us-west-2.amazonaws.com"
    for platform in PLATFORMS:
        for art in artifacts_for(platform, "1.28.8"):
            assert mirror_url(art.package, "1.28.8") == f"{bucket_url}/{art.s3_key()}"


def test_s3_key_uses_url_basename() -> None:
    art = Artifact(
        "android", "https://example/data/pkg/android/1.28.3/file-1.28.3.tar.xz", "android"
    )
    assert art.s3_key() == "dependencies/gstreamer/android/file-1.28.3.tar.xz"


def test_pinned_checksum_is_authoritative(monkeypatch) -> None:
    def no_sidecar(url: str, **_kwargs) -> str:
        raise AssertionError(f"fetched {url}")

    monkeypatch.setattr("common.gstreamer.read_url_text", no_sidecar)
    version = get_build_config_value("gstreamer.version.sdk")
    (art,) = artifacts_for("android", version)
    assert _expected_sha(art, version) == pinned_checksum(version, "android")


def test_unpinned_version_uses_sidecar(monkeypatch) -> None:
    sidecar = "AB" * 32
    monkeypatch.setattr(
        "common.gstreamer.read_url_text", lambda url, **_kwargs: f"{sidecar}  file.tar.xz\n"
    )
    (art,) = artifacts_for("android", "1.99.0")
    assert _expected_sha(art, "1.99.0") == sidecar.lower()


def test_main_reports_errors_as_escaped_annotations(capsys: pytest.CaptureFixture[str]) -> None:
    assert mirror_gstreamer.main(["--version", "1.28.3", "--platforms", "bad\nplatform"]) == 1
    out = capsys.readouterr().out
    assert out.startswith("::error::Unknown platform(s): bad%0Aplatform; valid: ")
    assert out.count("\n") == 1


def test_main_summarizes_each_mirrored_file(
    monkeypatch: pytest.MonkeyPatch, tmp_path: Path, gh_summary: Path
) -> None:
    monkeypatch.setattr(mirror_gstreamer, "mirror_artifact", lambda artifact, **kw: "uploaded")
    assert (
        mirror_gstreamer.main(
            [
                "--version",
                "1.28.3",
                "--platforms",
                "android",
                "--work-dir",
                str(tmp_path),
                "--dry-run",
            ]
        )
        == 0
    )
    summary = gh_summary.read_text(encoding="utf-8")
    assert summary.startswith("### GStreamer mirror\n\n| Item | Value |")
    assert "| Mode | dry run |" in summary
    assert "| Files | 1 |" in summary
    assert "| Platform | File | Status |" in summary
    assert "| uploaded |" in summary


def test_options_default_to_the_workflow_environment(
    monkeypatch: pytest.MonkeyPatch, tmp_path: Path
) -> None:
    monkeypatch.setenv("RUNNER_TEMP", str(tmp_path))
    for name in ("VERSION", "PLATFORMS", "DRY_RUN", "FORCE"):
        monkeypatch.delenv(f"MIRROR_{name}", raising=False)
    args = mirror_gstreamer.parse_args([])
    assert (args.version, args.platforms, args.work_dir) == ("", "all", str(tmp_path))
    assert (args.dry_run, args.force) == (False, False)

    # mirror-gstreamer.yml passes its inputs as MIRROR_* (booleans as true/false).
    for name, value in {"VERSION": "1.28.3", "PLATFORMS": "ios", "DRY_RUN": "true"}.items():
        monkeypatch.setenv(f"MIRROR_{name}", value)
    monkeypatch.setenv("MIRROR_FORCE", "false")
    args = mirror_gstreamer.parse_args([])
    assert (args.version, args.platforms, args.dry_run, args.force) == (
        "1.28.3",
        "ios",
        True,
        False,
    )
    # An empty workflow input falls back to the defaults.
    monkeypatch.setenv("MIRROR_VERSION", "")
    monkeypatch.setenv("MIRROR_PLATFORMS", "")
    assert mirror_gstreamer.parse_args([]).version == ""
    assert mirror_gstreamer.parse_args(["--force"]).platforms == "all"


def test_version_is_required_without_a_build_config_version(
    monkeypatch: pytest.MonkeyPatch, tmp_path: Path, capsys: pytest.CaptureFixture[str]
) -> None:
    _use_versions(monkeypatch, tmp_path, {})
    monkeypatch.delenv("MIRROR_VERSION", raising=False)
    assert mirror_gstreamer.main(["--platforms", "android", "--work-dir", str(tmp_path)]) == 1
    assert (
        "build-config.json has no gstreamer.version.sdk; pass --version" in capsys.readouterr().out
    )


def test_workflow_passes_its_inputs_as_mirror_variables() -> None:
    steps = get_steps(load_workflow("mirror-gstreamer.yml"), "mirror")
    step = get_step(steps, "Mirror artifacts")
    assert step["run"] == 'python3 "${GITHUB_WORKSPACE}/tools/ci/gstreamer/mirror_gstreamer.py"'
    # No AWS_DEFAULT_REGION: aws-credentials exports the region, and dry runs make no AWS calls.
    assert step["env"] == {
        "MIRROR_VERSION": "${{ inputs.version }}",
        "MIRROR_PLATFORMS": "${{ inputs.platforms }}",
        "MIRROR_DRY_RUN": "${{ inputs.dry_run }}",
        "MIRROR_FORCE": "${{ inputs.force }}",
    }
    assert get_step(steps, "Configure AWS Credentials")["if"] == "${{ !inputs.dry_run }}"


def test_workflow_mirrors_build_config_changes_on_master() -> None:
    workflow = load_workflow("mirror-gstreamer.yml")
    assert workflow["on"]["push"] == {
        "branches": ["master"],
        "paths": [".github/build-config.json"],
    }
    # A push has no inputs: every platform's build-config version is mirrored and uploaded.
    assert resolve_platforms("") == PLATFORMS
    assert workflow["jobs"]["mirror"]["if"] == (
        "github.event_name == 'workflow_dispatch' || github.repository == 'mavlink/qgroundcontrol'"
    )


def test_main_mirrors_the_sdk_version_of_build_config(
    monkeypatch: pytest.MonkeyPatch, tmp_path: Path, gh_summary: Path
) -> None:
    _use_versions(monkeypatch, tmp_path, {"minimum": "1.20.0", "sdk": "1.28.7"})
    monkeypatch.delenv("MIRROR_VERSION", raising=False)
    mirrored: list[tuple[str, str]] = []

    def record(artifact: Artifact, *, version: str, **_kwargs: object) -> str:
        mirrored.append((artifact.filename, version))
        return "dry-run"

    monkeypatch.setattr(mirror_gstreamer, "mirror_artifact", record)
    argv = ["--platforms", "android,ios", "--work-dir", str(tmp_path), "--dry-run"]
    assert mirror_gstreamer.main(argv) == 0
    assert mirrored == [
        ("gstreamer-1.0-android-universal-1.28.7.tar.xz", "1.28.7"),
        ("gstreamer-1.28.7-xcframework.tar.xz", "1.28.7"),
    ]
