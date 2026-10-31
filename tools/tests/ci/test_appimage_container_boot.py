"""Tests for appimage_container_boot.py."""

from __future__ import annotations

import shutil
import stat
import subprocess
from typing import TYPE_CHECKING

import pytest
from ci.packaging import appimage_container_boot
from ci.packaging.appimage_container_boot import (
    CONTAINER_SCRIPT,
    RUNTIME_PACKAGES,
    docker_command,
)

if TYPE_CHECKING:
    from pathlib import Path

IMAGE = "public.ecr.aws/docker/library/ubuntu:24.04"
OS_RELEASE = 'PRETTY_NAME="Ubuntu 24.04.3 LTS"\nBooting /appimage/QGroundControl-x86_64.AppImage\n'


@pytest.fixture
def appimage(tmp_path: Path) -> Path:
    path = tmp_path / "artifact" / "QGroundControl-x86_64.AppImage"
    path.parent.mkdir()
    path.write_bytes(b"\x7fELF")
    path.chmod(0o644)
    return path


class FakeDocker:
    """Record docker commands; answer docker run with a canned result or a timeout."""

    def __init__(self, returncode: int = 0, output: str = OS_RELEASE, *, hang: bool = False):
        self.returncode = returncode
        self.output = output
        self.hang = hang
        self.calls: list[list[str]] = []

    def __call__(self, cmd, **kwargs):
        self.calls.append(list(cmd))
        if cmd[:2] == ["docker", "kill"]:
            return subprocess.CompletedProcess(cmd, 0, "", "")
        assert cmd[:2] == ["docker", "run"]
        assert kwargs["stderr"] is subprocess.STDOUT
        if self.hang:
            raise subprocess.TimeoutExpired(cmd, kwargs["timeout"], output=self.output)
        return subprocess.CompletedProcess(cmd, self.returncode, self.output, None)


def _main(monkeypatch, appimage: Path, docker: FakeDocker) -> int:
    monkeypatch.setattr(subprocess, "run", docker)
    return appimage_container_boot.main(["--appimage", str(appimage), "--image", IMAGE])


def test_container_gets_the_appimage_read_only_and_only_the_runtime_packages(appimage):
    cmd = docker_command(appimage, IMAGE, "qgc-boot", 90)

    assert cmd[:4] == ["docker", "run", "--rm", "--init"]
    assert cmd[cmd.index("--name") + 1] == "qgc-boot"
    assert cmd[cmd.index("--volume") + 1] == f"{appimage.parent.resolve()}:/appimage:ro"
    environment = [cmd[index + 1] for index, part in enumerate(cmd) if part == "--env"]
    assert environment == ["QGC_BOOT_TIMEOUT=90", "QT_FORCE_STDERR_LOGGING=1"]
    script_at = cmd.index(IMAGE) + 1
    assert cmd[script_at : script_at + 3] == ["bash", "-c", CONTAINER_SCRIPT]
    assert cmd[script_at + 3 :] == ["boot", appimage.name, *RUNTIME_PACKAGES]


def test_runtime_packages_bring_no_gstreamer_or_qt():
    assert not [
        package
        for package in RUNTIME_PACKAGES
        if package.startswith(("gstreamer", "libgstreamer", "libqt", "qt6", "libglib", "libfuse"))
    ]


def test_container_script_boots_the_appimage_without_fuse_under_xvfb():
    assert "unset GST_PLUGIN_PATH GST_PLUGIN_PATH_1_0 GST_PLUGIN_SYSTEM_PATH" in CONTAINER_SCRIPT
    assert "GST_REGISTRY GST_REGISTRY_1_0" in CONTAINER_SCRIPT
    assert 'xvfb-run -a "$appimage" --appimage-extract-and-run' in CONTAINER_SCRIPT
    assert "--simple-boot-test" in CONTAINER_SCRIPT


@pytest.mark.skipif(shutil.which("bash") is None, reason="needs bash")
def test_container_script_is_valid_bash():
    result = subprocess.run(
        ["bash", "-n", "-c", CONTAINER_SCRIPT], capture_output=True, text=True, check=False
    )
    assert result.returncode == 0, result.stderr


def test_a_passing_boot_is_summarized(monkeypatch, appimage, capsys, gh_summary):
    docker = FakeDocker()

    assert _main(monkeypatch, appimage, docker) == 0

    assert appimage.stat().st_mode & stat.S_IXUSR
    (run,) = docker.calls
    assert run[run.index("--env") + 1] == "QGC_BOOT_TIMEOUT=120"
    assert f"AppImage boot in {IMAGE} passed" in capsys.readouterr().out
    summary = gh_summary.read_text(encoding="utf-8")
    assert summary.startswith(f"### AppImage boot in {IMAGE}\n\n| Item | Value |")
    assert "| Result | passed |" in summary
    assert "| Distribution | Ubuntu 24.04.3 LTS |" in summary
    assert "Last" not in summary


@pytest.mark.parametrize(
    ("returncode", "result"),
    [
        (90, "failed to install the runtime packages"),
        (91, "failed: the runtime packages installed GStreamer or Qt"),
        (124, "timed out after 120s"),
        (125, "failed: docker run exited 125"),
        (139, "failed (exit code 139)"),
    ],
)
def test_failures_name_their_cause(monkeypatch, appimage, capsys, gh_summary, returncode, result):
    docker = FakeDocker(returncode, OS_RELEASE + "QGC crashed\n")

    assert _main(monkeypatch, appimage, docker) == 1

    assert (
        f"::error title=AppImage boot::AppImage boot in {IMAGE} {result}" in capsys.readouterr().out
    )
    summary = gh_summary.read_text(encoding="utf-8")
    assert f"| Result | {result} |" in summary
    assert "QGC crashed" in summary


def test_a_hung_container_is_killed(monkeypatch, appimage, gh_summary):
    docker = FakeDocker(output="Booting\n", hang=True)

    assert _main(monkeypatch, appimage, docker) == 1

    run, kill = docker.calls
    assert kill == ["docker", "kill", run[run.index("--name") + 1]]
    assert "| Result | timed out after 120s |" in gh_summary.read_text(encoding="utf-8")


def test_missing_appimage_fails(tmp_path, monkeypatch, capsys):
    docker = FakeDocker()

    assert _main(monkeypatch, tmp_path / "absent.AppImage", docker) == 1
    assert "AppImage not found" in capsys.readouterr().out
    assert not docker.calls
