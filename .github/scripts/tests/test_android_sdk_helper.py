"""Tests for android_sdk_helper.py."""

from __future__ import annotations

import subprocess
from pathlib import Path
from typing import Any

import pytest

import android_sdk_helper as mod


def _setup_env(monkeypatch, tmp_path: Path, *, runner_os: str = "Linux") -> tuple[Path, Path]:
    sdk_root = tmp_path / "sdk"
    ndk_dir = sdk_root / "ndk" / "27.0.12077973"
    ndk_dir.mkdir(parents=True)
    workspace = tmp_path / "workspace"
    (workspace / "android").mkdir(parents=True)
    gh_env = tmp_path / "gh_env"
    gh_env.write_text("")
    monkeypatch.setenv("ANDROID_SDK_ROOT", str(sdk_root))
    monkeypatch.setenv("GITHUB_ENV", str(gh_env))
    monkeypatch.setenv("RUNNER_OS", runner_os)
    monkeypatch.setattr(
        "sys.argv",
        ["prog", "--ndk-version", "27.0.12077973", "--workspace", str(workspace)],
    )
    return gh_env, sdk_root


def test_missing_sdk_root_exits(monkeypatch, capsys) -> None:
    monkeypatch.delenv("ANDROID_SDK_ROOT", raising=False)
    monkeypatch.setattr("sys.argv", ["prog", "--ndk-version", "27.0.12077973"])
    with pytest.raises(SystemExit) as exc:
        mod.main()
    assert exc.value.code == 1
    assert "ANDROID_SDK_ROOT not set" in capsys.readouterr().err


def test_missing_ndk_path_exits(monkeypatch, tmp_path: Path, capsys) -> None:
    sdk_root = tmp_path / "sdk"
    sdk_root.mkdir()  # SDK exists but NDK subdir doesn't
    monkeypatch.setenv("ANDROID_SDK_ROOT", str(sdk_root))
    monkeypatch.setattr("sys.argv", ["prog", "--ndk-version", "27.0.12077973"])
    with pytest.raises(SystemExit) as exc:
        mod.main()
    assert exc.value.code == 1
    assert "NDK path not found" in capsys.readouterr().err


def test_unix_invokes_sdkmanager_and_gradlew(monkeypatch, tmp_path: Path) -> None:
    gh_env, _ = _setup_env(monkeypatch, tmp_path, runner_os="Linux")
    calls: list[list[str]] = []
    monkeypatch.setattr(
        subprocess,
        "run",
        lambda cmd, **kw: calls.append(list(cmd)) or subprocess.CompletedProcess(cmd, 0),
    )

    mod.main()
    env = gh_env.read_text()
    assert "ANDROID_NDK_ROOT=" in env
    assert "ANDROID_NDK_HOME=" in env
    assert "ANDROID_NDK=" in env
    assert calls[0] == ["sdkmanager", "--update"]
    assert calls[1][-1] == "--version"
    assert calls[1][0].endswith("/android/gradlew")


def test_windows_uses_bat_paths(monkeypatch, tmp_path: Path) -> None:
    _setup_env(monkeypatch, tmp_path, runner_os="Windows")
    calls: list[list[str]] = []
    monkeypatch.setattr(
        subprocess,
        "run",
        lambda cmd, **kw: calls.append(list(cmd)) or subprocess.CompletedProcess(cmd, 0),
    )

    mod.main()
    assert calls[0][0].endswith("sdkmanager.bat")
    assert calls[1][0].endswith("gradlew.bat")


def test_subprocess_failure_propagates(monkeypatch, tmp_path: Path) -> None:
    _setup_env(monkeypatch, tmp_path)

    def fake_run(cmd: list[str], **kw: Any) -> subprocess.CompletedProcess:
        raise subprocess.CalledProcessError(returncode=2, cmd=cmd)

    monkeypatch.setattr(subprocess, "run", fake_run)
    with pytest.raises(subprocess.CalledProcessError):
        mod.main()
