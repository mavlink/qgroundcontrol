"""Tests for android_build_retry.py."""

from __future__ import annotations

from typing import TYPE_CHECKING

import android_build_retry as mod

if TYPE_CHECKING:
    from pathlib import Path


def test_detect_truncation_matches_qt_error(tmp_path: Path) -> None:
    log = tmp_path / "build.log"
    log.write_text(
        "[ 92%] linking ...\n"
        "Invalid json file: /tmp/build/android-QGroundControl-deployment-settings.json. "
        "Reason: unterminated object\n"
        "[ 93%] more output\n"
    )
    assert mod.detect_truncation(log) is True


def test_detect_truncation_ignores_other_errors(tmp_path: Path) -> None:
    log = tmp_path / "build.log"
    log.write_text(
        "ninja: error: '/tmp/foo': No such file\n"
        "Reason: unterminated object\n"  # right-half-only; missing prefix
    )
    assert mod.detect_truncation(log) is False


def test_detect_truncation_handles_non_utf8_bytes(tmp_path: Path) -> None:
    log = tmp_path / "build.log"
    log.write_bytes(
        b"\xff\xfe garbage prefix\n"
        b"Invalid json file: dir/android-QGroundControl-deployment-settings.json. "
        b"Reason: unterminated object\n"
    )
    assert mod.detect_truncation(log) is True


def test_clean_settings_json_removes_file(tmp_path: Path) -> None:
    path = tmp_path / "settings.json"
    path.write_text("{ truncated")
    mod.clean_settings_json(path)
    assert not path.exists()


def test_clean_settings_json_noop_when_missing(tmp_path: Path) -> None:
    mod.clean_settings_json(tmp_path / "absent.json")


def test_main_missing_log_returns_error(tmp_path: Path, capsys) -> None:
    rc = mod.main(["--build-dir", str(tmp_path), "--build-type", "Debug"])
    assert rc == 1
    assert "is missing" in capsys.readouterr().out


def test_main_non_retriable_returns_error(tmp_path: Path, capsys) -> None:
    (tmp_path / "qgc-build.log").write_text("ninja: error: unrelated\n")
    rc = mod.main(["--build-dir", str(tmp_path), "--build-type", "Debug"])
    assert rc == 1
    assert "non-retriable" in capsys.readouterr().out


def test_main_truncation_invokes_retry(tmp_path: Path, monkeypatch) -> None:
    (tmp_path / "qgc-build.log").write_text(
        "Invalid json file: /tmp/android-QGroundControl-deployment-settings.json. "
        "Reason: unterminated object\n"
    )
    settings = tmp_path / "android-QGroundControl-deployment-settings.json"
    settings.write_text("{ truncated")

    captured: dict[str, object] = {}

    def fake_retry(build_dir: Path, build_type: str, retry_log: Path) -> int:
        captured["build_dir"] = build_dir
        captured["build_type"] = build_type
        captured["retry_log"] = retry_log
        return 0

    monkeypatch.setattr(mod, "retry_build", fake_retry)
    rc = mod.main(["--build-dir", str(tmp_path), "--build-type", "Release"])
    assert rc == 0
    assert captured["build_dir"] == tmp_path
    assert captured["build_type"] == "Release"
    assert captured["retry_log"] == tmp_path / "qgc-build-retry.log"
    assert not settings.exists()


def test_main_propagates_retry_exit_code(tmp_path: Path, monkeypatch) -> None:
    (tmp_path / "qgc-build.log").write_text(
        "Invalid json file: x/android-QGroundControl-deployment-settings.json. "
        "Reason: unterminated object\n"
    )
    monkeypatch.setattr(mod, "retry_build", lambda *a, **kw: 2)
    rc = mod.main(["--build-dir", str(tmp_path), "--build-type", "Debug"])
    assert rc == 2
