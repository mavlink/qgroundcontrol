"""Android SDK/NDK environment setup contracts."""

from __future__ import annotations

import subprocess
import sys
from typing import TYPE_CHECKING

import android_sdk_helper as mod
import pytest

if TYPE_CHECKING:
    from pathlib import Path


def _setup(monkeypatch: pytest.MonkeyPatch, tmp_path: Path, runner_os: str) -> tuple[Path, Path]:
    sdk = tmp_path / "sdk"
    (sdk / "ndk" / "27.0.12077973").mkdir(parents=True)
    workspace = tmp_path / "workspace"
    (workspace / "android").mkdir(parents=True)
    github_env = tmp_path / "github-env"
    monkeypatch.setenv("ANDROID_SDK_ROOT", str(sdk))
    monkeypatch.setenv("GITHUB_ENV", str(github_env))
    monkeypatch.setenv("RUNNER_OS", runner_os)
    monkeypatch.setattr(
        sys,
        "argv",
        ["prog", "--ndk-version", "27.0.12077973", "--workspace", str(workspace)],
    )
    return sdk, github_env


def test_missing_sdk_or_ndk_is_rejected(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch, capsys: pytest.CaptureFixture[str]
) -> None:
    monkeypatch.delenv("ANDROID_SDK_ROOT", raising=False)
    monkeypatch.setattr(sys, "argv", ["prog", "--ndk-version", "27.0.12077973"])
    with pytest.raises(SystemExit) as error:
        mod.main()
    assert error.value.code == 1 and "ANDROID_SDK_ROOT not set" in capsys.readouterr().out

    sdk = tmp_path / "sdk"
    sdk.mkdir()
    monkeypatch.setenv("ANDROID_SDK_ROOT", str(sdk))
    with pytest.raises(SystemExit) as error:
        mod.main()
    assert error.value.code == 1 and "NDK path not found" in capsys.readouterr().out


def test_linux_and_windows_use_platform_commands_and_export_ndk(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    for runner_os in ("Linux", "Windows"):
        case_dir = tmp_path / runner_os
        sdk, github_env = _setup(monkeypatch, case_dir, runner_os)
        if runner_os == "Windows":
            sdkmanager = sdk / "cmdline-tools" / "latest" / "bin" / "sdkmanager.bat"
            sdkmanager.parent.mkdir(parents=True)
            sdkmanager.write_text("")
        calls: list[list[str]] = []
        monkeypatch.setattr(
            subprocess,
            "run",
            lambda command, calls=calls, **_kwargs: (
                calls.append(list(command)) or subprocess.CompletedProcess(command, 0)
            ),
        )
        mod.main()
        exported = github_env.read_text()
        assert all(
            name in exported for name in ("ANDROID_NDK_ROOT=", "ANDROID_NDK_HOME=", "ANDROID_NDK=")
        )
        if runner_os == "Linux":
            assert calls[0] == ["sdkmanager", "--update"]
            assert calls[1][0].endswith("/android/gradlew")
        else:
            assert calls[0][0].endswith("sdkmanager.bat")
            assert calls[1][0].endswith("gradlew.bat")


def test_windows_sdkmanager_uses_latest_versioned_install_or_errors(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch, capsys: pytest.CaptureFixture[str]
) -> None:
    sdk, _ = _setup(monkeypatch, tmp_path, "Windows")
    for version in ("9.0", "10.0"):
        executable = sdk / "cmdline-tools" / version / "bin" / "sdkmanager.bat"
        executable.parent.mkdir(parents=True)
        executable.write_text("")
    assert mod._find_sdkmanager(str(sdk)).replace("\\", "/").endswith("10.0/bin/sdkmanager.bat")

    for executable in sdk.rglob("sdkmanager.bat"):
        executable.unlink()
    with pytest.raises(SystemExit) as error:
        mod.main()
    assert error.value.code == 1 and "sdkmanager.bat not found" in capsys.readouterr().out


def test_subprocess_failures_propagate(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    _setup(monkeypatch, tmp_path, "Linux")
    monkeypatch.setattr("time.sleep", lambda *_args, **_kwargs: None)

    def fail(command: list[str], **_kwargs) -> subprocess.CompletedProcess[str]:
        raise subprocess.CalledProcessError(returncode=2, cmd=command)

    monkeypatch.setattr(subprocess, "run", fail)
    with pytest.raises(subprocess.CalledProcessError):
        mod.main()
