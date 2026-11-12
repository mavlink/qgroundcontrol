"""Docker and VM builds create their AppImage without appimagelint, as CI's cmake-configure does."""

from __future__ import annotations

import os
from pathlib import Path

import pytest

from ..._helpers import REPO_ROOT, load_script

FLAG = "-DQGC_RUN_APPIMAGELINT=OFF"


@pytest.mark.parametrize("build_type", ["Release", "MinSizeRel"])
def test_docker_native_build_configures_without_appimagelint(
    monkeypatch: pytest.MonkeyPatch, build_type: str
) -> None:
    monkeypatch.syspath_prepend(str(REPO_ROOT / "deploy/docker"))
    script = load_script("deploy/docker/entrypoint.py")
    # build() exports this variable; monkeypatch restores it.
    monkeypatch.setenv("APPIMAGE_EXTRACT_AND_RUN", "")
    for name in ("ANDROID_SDK_ROOT", "SYSROOT", "JOBS"):
        monkeypatch.delenv(name, raising=False)
    calls: list[list[str]] = []
    monkeypatch.setattr(script.subprocess, "run", lambda command, **kwargs: calls.append(command))

    script.build(build_type)

    assert FLAG in calls[0]


def test_vm_build_configures_without_appimagelint(
    monkeypatch: pytest.MonkeyPatch, tmp_path: Path
) -> None:
    script = load_script("deploy/multipass/build_in_vm.py")
    monkeypatch.chdir(tmp_path)
    monkeypatch.setenv("BUILD_DIR", str(tmp_path))
    monkeypatch.setenv("QT_OUT", str(tmp_path / "Qt"))
    monkeypatch.setenv("QGC_PYTHON_ENV", "/opt/qgc-venv")
    monkeypatch.delenv("BUILD_TYPE", raising=False)
    # main() changes both; monkeypatch restores them.
    monkeypatch.setenv("PATH", os.environ["PATH"])
    monkeypatch.delenv("QT_ROOT_DIR", raising=False)
    monkeypatch.setattr(Path, "home", lambda: tmp_path)
    (tmp_path / "QGroundControl.AppImage").touch()
    calls: list[list[str]] = []
    monkeypatch.setattr(script.subprocess, "run", lambda command, **kwargs: calls.append(command))

    assert script.main(["--skip-dependencies"]) == 0

    (configure,) = (call for call in calls if call[0].endswith("/bin/qt-cmake"))
    assert FLAG in configure
