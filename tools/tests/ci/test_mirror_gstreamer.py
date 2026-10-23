"""Tests for mirror_gstreamer.py."""

from __future__ import annotations

from typing import TYPE_CHECKING

import pytest
from ci.gstreamer import mirror_gstreamer
from ci.gstreamer.mirror_gstreamer import (
    PLATFORMS,
    Artifact,
    _expected_sha,
    artifacts_for,
    resolve_platforms,
)
from common.gstreamer import pinned_checksum

from .._helpers import get_step, get_steps, load_workflow

if TYPE_CHECKING:
    from pathlib import Path


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


def test_windows_rejects_pre_1_28() -> None:
    with pytest.raises(ValueError, match="requires version"):
        artifacts_for("windows", "1.26.5")


def test_ios_devel_pkg() -> None:
    (art,) = artifacts_for("ios", "1.28.3")
    assert art.filename == "gstreamer-1.0-devel-1.28.3-ios-universal.pkg"
    assert art.url.startswith("https://gstreamer.freedesktop.org/data/pkg/ios/")


def test_unknown_platform_raises() -> None:
    with pytest.raises(ValueError, match="Unknown platform"):
        artifacts_for("plugin", "1.28.3")


def test_resolve_platforms_all() -> None:
    assert resolve_platforms("all") == list(PLATFORMS)
    assert resolve_platforms("") == list(PLATFORMS)


def test_resolve_platforms_subset_preserves_order() -> None:
    assert resolve_platforms("windows, android") == ["windows", "android"]


def test_resolve_platforms_rejects_unknown() -> None:
    with pytest.raises(ValueError, match="Unknown platform"):
        resolve_platforms("android,plugin")


def test_s3_key_uses_url_basename() -> None:
    art = Artifact(
        "android", "https://example/data/pkg/android/1.28.3/file-1.28.3.tar.xz", "android"
    )
    assert art.s3_key() == "dependencies/gstreamer/android/file-1.28.3.tar.xz"


def test_pinned_checksum_is_authoritative(monkeypatch) -> None:
    def no_sidecar(url: str, **_kwargs) -> str:
        raise AssertionError(f"fetched {url}")

    monkeypatch.setattr(mirror_gstreamer, "read_url_text", no_sidecar)
    (art,) = artifacts_for("android", "1.28.4")
    assert _expected_sha(art, "1.28.4") == pinned_checksum("1.28.4", "android")


def test_unpinned_version_uses_sidecar(monkeypatch) -> None:
    sidecar = "AB" * 32
    monkeypatch.setattr(
        mirror_gstreamer, "read_url_text", lambda url, **_kwargs: f"{sidecar}  file.tar.xz\n"
    )
    (art,) = artifacts_for("android", "0.0.1")
    assert _expected_sha(art, "0.0.1") == sidecar.lower()


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


def test_options_default_to_the_workflow_environment_and_build_config(
    monkeypatch: pytest.MonkeyPatch, tmp_path: Path
) -> None:
    config = tmp_path / "build-config.json"
    config.write_text('{"gstreamer": {"version": {"default": "1.28.9"}}}', encoding="utf-8")
    monkeypatch.setenv("CONFIG_FILE", str(config))
    monkeypatch.setenv("RUNNER_TEMP", str(tmp_path))
    for name in ("VERSION", "PLATFORMS", "DRY_RUN", "FORCE"):
        monkeypatch.delenv(f"MIRROR_{name}", raising=False)
    args = mirror_gstreamer.parse_args([])
    assert (args.version, args.platforms, args.work_dir) == ("1.28.9", "all", str(tmp_path))
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
    assert mirror_gstreamer.parse_args([]).version == "1.28.9"
    assert mirror_gstreamer.parse_args(["--force"]).platforms == "all"


def test_version_is_required_without_a_build_config_default(
    monkeypatch: pytest.MonkeyPatch, tmp_path: Path, capsys: pytest.CaptureFixture[str]
) -> None:
    config = tmp_path / "build-config.json"
    config.write_text('{"gstreamer": {}}', encoding="utf-8")
    monkeypatch.setenv("CONFIG_FILE", str(config))
    monkeypatch.delenv("MIRROR_VERSION", raising=False)
    with pytest.raises(SystemExit) as exc:
        mirror_gstreamer.parse_args([])
    assert exc.value.code == 2
    assert "build-config.json has no gstreamer.version.default" in capsys.readouterr().err


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
