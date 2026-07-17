"""Android failure-artifact collection contracts."""

from __future__ import annotations

import json
import subprocess
from typing import TYPE_CHECKING

import android_collect_diagnostics as mod

if TYPE_CHECKING:
    from pathlib import Path

    import pytest


def test_emulator_online_requires_device_state() -> None:
    assert mod.emulator_online("List of devices attached\nemulator-5554\tdevice\n") is True
    assert mod.emulator_online("List of devices attached\nemulator-5554\toffline\n") is False
    assert mod.emulator_online("List of devices attached\n") is False


def test_build_artifacts_copy_logs_boot_output_and_valid_or_invalid_settings(
    tmp_path: Path,
) -> None:
    build = tmp_path / "build"
    output = tmp_path / "output"
    build.mkdir()
    output.mkdir()
    (build / "qgc-build.log").write_text("build output")
    settings = build / "android-QGroundControl-deployment-settings.json"
    settings.write_text('{"k": "v"}')
    boot = tmp_path / "qgc_emulator_boot.log"
    boot.write_text("boot output")

    mod.collect_build_artifacts(output, build, boot)
    assert (output / "qgc-build.log").read_text() == "build output"
    assert json.loads(
        (output / "android-QGroundControl-deployment-settings.pretty.json").read_text()
    ) == {"k": "v"}
    assert (output / boot.name).read_text() == "boot output"
    assert not (output / "qgc-build-retry.log").exists()

    settings.write_text("{not valid json}")
    mod.collect_build_artifacts(output, build, None)
    assert (output / "android-QGroundControl-deployment-settings.error.txt").exists()


def test_gstreamer_diagnostics_filter_relevant_load_errors(tmp_path: Path) -> None:
    missing = tmp_path / "missing.txt"
    mod.extract_gstreamer_diagnostics(missing, tmp_path)
    assert not (tmp_path / "gstreamer-logcat.txt").exists()

    logcat = tmp_path / "logcat.txt"
    logcat.write_text(
        "I activity unrelated line\n"
        "E gstreamer plugin load failure\n"
        "E dlopen failed for libgstcoreelements.so\n"
    )
    mod.extract_gstreamer_diagnostics(logcat, tmp_path)
    gstreamer = (tmp_path / "gstreamer-logcat.txt").read_text()
    load_errors = (tmp_path / "gstreamer-load-errors.txt").read_text()
    assert "gstreamer plugin load failure" in gstreamer
    assert "dlopen failed" in gstreamer and "unrelated" not in gstreamer
    assert "dlopen failed" in load_errors and "plugin load failure" not in load_errors


def test_avd_collection_preserves_only_text_diagnostics(tmp_path: Path) -> None:
    output = tmp_path / "output"
    avd = tmp_path / "avd" / "qgc-ci.avd"
    output.mkdir()
    avd.mkdir(parents=True)
    for name in ("config.ini", "emulator-output.log"):
        (avd / name).write_text(name)
    (avd / "userdata.img").write_bytes(b"binary")
    mod.collect_avd_files(output, tmp_path / "avd")
    assert (output / "avd" / "qgc-ci.avd" / "config.ini").exists()
    assert (output / "avd" / "qgc-ci.avd" / "emulator-output.log").exists()
    assert not (output / "avd" / "qgc-ci.avd" / "userdata.img").exists()
    mod.collect_avd_files(output, tmp_path / "missing-avd")


def test_adb_skip_reasons_and_main_missing_inputs(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    monkeypatch.setattr(mod.shutil, "which", lambda _name: None)
    mod.collect_adb_diagnostics(tmp_path)
    assert "adb not found" in (tmp_path / "adb-skipped.txt").read_text()

    offline = subprocess.CompletedProcess(
        [], 0, stdout="List of devices attached\nemulator-5554\toffline\n", stderr=""
    )
    monkeypatch.setattr(mod.shutil, "which", lambda _name: "/usr/bin/adb")
    monkeypatch.setattr(mod, "_run", lambda *_args, **_kwargs: offline)
    mod.collect_adb_diagnostics(tmp_path)
    assert "No online emulator" in (tmp_path / "adb-skipped.txt").read_text()

    monkeypatch.setattr(mod.shutil, "which", lambda _name: None)
    output = tmp_path / "main-output"
    assert mod.main(["--out-dir", str(output), "--build-dir", str(tmp_path / "build")]) == 0
    assert output.is_dir()
