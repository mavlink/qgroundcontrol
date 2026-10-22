"""Tests for android_sdk_helper.py."""

from __future__ import annotations

import subprocess
from typing import TYPE_CHECKING, Any

import pytest
from ci.android import android_sdk_helper as mod

from .._helpers import REPO_ROOT, get_steps, load_action, load_workflow

if TYPE_CHECKING:
    from pathlib import Path


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
        [
            "prog",
            "--ndk-version",
            "27.0.12077973",
            "--platform",
            "36",
            "--build-tools",
            "36.0.0",
            "--workspace",
            str(workspace),
        ],
    )
    return gh_env, sdk_root


def test_missing_sdk_root_exits(monkeypatch, capsys) -> None:
    monkeypatch.delenv("ANDROID_SDK_ROOT", raising=False)
    monkeypatch.setattr(
        "sys.argv",
        [
            "prog",
            "--ndk-version",
            "27.0.12077973",
            "--platform",
            "36",
            "--build-tools",
            "36.0.0",
        ],
    )
    with pytest.raises(SystemExit) as exc:
        mod.main()
    assert exc.value.code == 1
    assert "ANDROID_SDK_ROOT not set" in capsys.readouterr().out


def test_missing_ndk_path_exits(monkeypatch, tmp_path: Path, capsys) -> None:
    sdk_root = tmp_path / "sdk"
    sdk_root.mkdir()  # SDK exists but NDK subdir doesn't
    monkeypatch.setenv("ANDROID_SDK_ROOT", str(sdk_root))
    monkeypatch.setenv("RUNNER_OS", "Linux")
    monkeypatch.setattr(
        "sys.argv",
        [
            "prog",
            "--ndk-version",
            "27.0.12077973",
            "--platform",
            "36",
            "--build-tools",
            "36.0.0",
        ],
    )
    monkeypatch.setattr(
        subprocess,
        "run",
        lambda cmd, **kw: subprocess.CompletedProcess(cmd, 0),
    )
    with pytest.raises(SystemExit) as exc:
        mod.main()
    assert exc.value.code == 1
    assert "NDK path not found after installation" in capsys.readouterr().out


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
    assert calls[0] == [
        "sdkmanager",
        "platform-tools",
        "platforms;android-36",
        "build-tools;36.0.0",
    ]
    assert calls[1] == ["sdkmanager", "ndk;27.0.12077973"]
    assert calls[2][-1] == "--version"
    assert calls[2][0].endswith("/android/gradlew")


def test_windows_uses_bat_paths(monkeypatch, tmp_path: Path) -> None:
    _, sdk_root = _setup_env(monkeypatch, tmp_path, runner_os="Windows")
    sdkmanager = sdk_root / "cmdline-tools" / "latest" / "bin" / "sdkmanager.bat"
    sdkmanager.parent.mkdir(parents=True)
    sdkmanager.write_text("")
    calls: list[list[str]] = []
    monkeypatch.setattr(
        subprocess,
        "run",
        lambda cmd, **kw: calls.append(list(cmd)) or subprocess.CompletedProcess(cmd, 0),
    )

    mod.main()
    assert calls[0][0] == str(sdkmanager)
    assert calls[1] == [str(sdkmanager), "ndk;27.0.12077973"]
    assert calls[2][0].endswith("gradlew.bat")


def test_gradle_probe_retains_timeout(monkeypatch, tmp_path: Path) -> None:
    _setup_env(monkeypatch, tmp_path)
    calls: list[tuple[list[str], dict[str, Any]]] = []

    def run_with_retry(command: list[str], **kwargs: Any) -> subprocess.CompletedProcess[str]:
        calls.append((command, kwargs))
        return subprocess.CompletedProcess(command, 0)

    monkeypatch.setattr(mod, "run_with_retry", run_with_retry)

    mod.main()

    assert calls[-1][0][-1] == "--version"
    assert calls[-1][1]["timeout"] == 300


def test_windows_prefers_versioned_when_no_latest(monkeypatch, tmp_path: Path) -> None:
    _, sdk_root = _setup_env(monkeypatch, tmp_path, runner_os="Windows")
    for version in ("9.0", "10.0", "custom"):
        bat = sdk_root / "cmdline-tools" / version / "bin" / "sdkmanager.bat"
        bat.parent.mkdir(parents=True)
        bat.write_text("")
    monkeypatch.setattr(
        subprocess,
        "run",
        lambda cmd, **kw: subprocess.CompletedProcess(cmd, 0),
    )

    assert (
        mod._find_sdkmanager(str(sdk_root)).replace("\\", "/").endswith("10.0/bin/sdkmanager.bat")
    )


def test_windows_missing_sdkmanager_exits(monkeypatch, tmp_path: Path, capsys) -> None:
    _setup_env(monkeypatch, tmp_path, runner_os="Windows")
    with pytest.raises(SystemExit) as exc:
        mod.main()
    assert exc.value.code == 1
    assert "sdkmanager.bat not found" in capsys.readouterr().out


def test_subprocess_failure_propagates(monkeypatch, tmp_path: Path) -> None:
    _setup_env(monkeypatch, tmp_path)
    monkeypatch.setattr("time.sleep", lambda *_a, **_k: None)

    def fake_run(cmd: list[str], **kw: Any) -> subprocess.CompletedProcess:
        raise subprocess.CalledProcessError(returncode=2, cmd=cmd)

    monkeypatch.setattr(subprocess, "run", fake_run)
    with pytest.raises(subprocess.CalledProcessError):
        mod.main()


def test_package_retry_removes_partial_ndk(monkeypatch, tmp_path: Path) -> None:
    ndk_path = tmp_path / "sdk" / "ndk" / "27.0.12077973"
    ndk_path.mkdir(parents=True)
    partial = ndk_path / "partial.zip"
    partial.write_bytes(b"incomplete")
    calls = 0

    def fake_run(cmd: list[str], **kw: Any) -> subprocess.CompletedProcess:
        nonlocal calls
        calls += 1
        if calls == 1:
            raise subprocess.CalledProcessError(returncode=1, cmd=cmd)
        ndk_path.mkdir(parents=True)
        return subprocess.CompletedProcess(cmd, 0)

    monkeypatch.setattr(subprocess, "run", fake_run)
    monkeypatch.setattr("time.sleep", lambda *_a, **_k: None)

    mod._install_ndk("sdkmanager", "27.0.12077973", ndk_path)

    assert calls == 2
    assert ndk_path.is_dir()
    assert not partial.exists()


def test_ndk_retry_cleanup_failure_propagates(monkeypatch, tmp_path: Path) -> None:
    ndk_path = tmp_path / "sdk" / "ndk" / "27.0.12077973"
    ndk_path.mkdir(parents=True)
    monkeypatch.setattr(
        subprocess,
        "run",
        lambda cmd, **kw: (_ for _ in ()).throw(
            subprocess.CalledProcessError(returncode=1, cmd=cmd)
        ),
    )
    monkeypatch.setattr(mod.shutil, "rmtree", lambda _path: (_ for _ in ()).throw(OSError("busy")))

    with pytest.raises(OSError, match="busy"):
        mod._install_ndk("sdkmanager", "27.0.12077973", ndk_path)


def test_qt_android_action_targets_first_installed_abi() -> None:
    action = load_action("qt-android")
    installs = [
        step for step in action["runs"]["steps"] if step.get("id", "").startswith("qt-android-")
    ]

    # android_matrix.py emits only these ABIs; the first installed one supplies the target root.
    assert [step["with"]["arch"] for step in installs] == [
        "android_arm64_v8a",
        "android_armv7",
        "android_x86_64",
    ]
    assert action["outputs"]["target_qt_root_dir"]["value"] == (
        "${{ " + " || ".join(f"steps.{step['id']}.outputs.qt_root_dir" for step in installs) + " }}"
    )
    for step in installs:
        assert step["with"]["export-env"] == "false"


def test_debug_keystore_signs_with_throwaway_key_and_exports_qt_env(
    monkeypatch, tmp_path: Path
) -> None:
    gh_env = tmp_path / "gh_env"
    gh_env.write_text("")
    monkeypatch.setenv("GITHUB_ENV", str(gh_env))
    monkeypatch.setenv("RUNNER_TEMP", str(tmp_path))
    monkeypatch.setattr(mod.shutil, "which", lambda name: f"/jdk/bin/{name}")
    calls: list[list[str]] = []
    monkeypatch.setattr(
        subprocess,
        "run",
        lambda cmd, **kw: calls.append(list(cmd)) or subprocess.CompletedProcess(cmd, 0),
    )

    mod.main(["debug-keystore"])

    keystore = (tmp_path / "debug.keystore").as_posix()
    (keytool,) = calls
    assert keytool[:4] == ["/jdk/bin/keytool", "-genkey", "-v", "-keystore"]
    assert keytool[4] == keystore
    options = dict(zip(keytool[5::2], keytool[6::2], strict=True))
    assert options == {
        "-storepass": "android",
        "-alias": "androiddebugkey",
        "-keypass": "android",
        "-keyalg": "RSA",
        "-keysize": "2048",
        "-validity": "10000",
        "-dname": "CN=Android Debug,O=Android,C=US",
    }
    assert gh_env.read_text().splitlines() == [
        f"QT_ANDROID_KEYSTORE_PATH={keystore}",
        "QT_ANDROID_KEYSTORE_ALIAS=androiddebugkey",
        "QT_ANDROID_KEYSTORE_STORE_PASS=android",
        "QT_ANDROID_KEYSTORE_KEY_PASS=android",
    ]


def test_debug_keystore_failure_exports_nothing(monkeypatch, tmp_path: Path) -> None:
    gh_env = tmp_path / "gh_env"
    gh_env.write_text("")
    monkeypatch.setenv("GITHUB_ENV", str(gh_env))
    monkeypatch.setattr(mod.shutil, "which", lambda name: name)

    def fail(cmd: list[str], **kw: Any) -> subprocess.CompletedProcess:
        raise subprocess.CalledProcessError(returncode=1, cmd=cmd)

    monkeypatch.setattr(subprocess, "run", fail)
    with pytest.raises(subprocess.CalledProcessError):
        mod.main(["debug-keystore", "--keystore", str(tmp_path / "custom.keystore")])
    assert gh_env.read_text() == ""


def test_debug_keystore_requires_a_location_and_keytool(
    monkeypatch, tmp_path: Path, capsys
) -> None:
    monkeypatch.delenv("RUNNER_TEMP", raising=False)
    with pytest.raises(SystemExit) as exc:
        mod.main(["debug-keystore"])
    assert exc.value.code == 2

    monkeypatch.setattr(mod.shutil, "which", lambda _name: None)
    with pytest.raises(SystemExit) as exc:
        mod.main(["debug-keystore", "--keystore", str(tmp_path / "debug.keystore")])
    assert exc.value.code == 1
    assert "keytool not found" in capsys.readouterr().out


def test_android_build_delegates_sdk_location_signing_and_apk_copy_to_helpers() -> None:
    action = load_action("qt-android")
    names = [step["name"] for step in action["runs"]["steps"]]
    location = action["runs"]["steps"][names.index("Set Android SDK location")]
    assert names.index("Set Android SDK location") < names.index("Setup Android Environment")
    assert location["if"] == "runner.os != 'macOS'"
    assert 'ANDROID_SDK_ROOT=${RUNNER_TEMP}/android-sdk" >> "$GITHUB_ENV"' in location["run"]

    workflow_text = (REPO_ROOT / ".github/workflows/android.yml").read_text()
    steps = {step["name"]: step for step in get_steps(load_workflow("android.yml"), "build")}
    assert "ANDROID_SDK_ROOT" not in workflow_text
    keystore = steps["Create Debug Keystore"]
    # The release keystore password resolves empty on pull requests, so every PR leg signs with it.
    assert keystore["if"] == "${{ env.QT_ANDROID_KEYSTORE_STORE_PASS == '' || !matrix.primary }}"
    assert keystore["run"].endswith('android_sdk_helper.py" debug-keystore')
    copy = " ".join(steps["Prepare Artifact"]["run"].replace("\\\n", " ").split())
    assert 'find_artifact.py" --build-dir "${RUNNER_TEMP}/build/android-build"' in copy
    assert "--pattern '*.apk' --top-level" in copy
    assert '--copy-to "${RUNNER_TEMP}/build/${PACKAGE}.apk"' in copy
