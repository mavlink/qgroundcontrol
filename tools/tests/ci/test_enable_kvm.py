"""Runner KVM access setup without touching udev."""

from __future__ import annotations

import subprocess
from typing import TYPE_CHECKING

import pytest
from ci.runner import enable_kvm

if TYPE_CHECKING:
    from pathlib import Path


@pytest.fixture
def calls(monkeypatch: pytest.MonkeyPatch) -> list[tuple[list[str], dict[str, object]]]:
    result: list[tuple[list[str], dict[str, object]]] = []

    def run(command: list[str], **kwargs: object) -> subprocess.CompletedProcess[str]:
        result.append((command, kwargs))
        return subprocess.CompletedProcess(command, 0)

    monkeypatch.setattr(enable_kvm.subprocess, "run", run)
    return result


def test_opens_kvm_and_waits_for_udev(
    calls: list[tuple[list[str], dict[str, object]]],
    monkeypatch: pytest.MonkeyPatch,
    tmp_path: Path,
) -> None:
    kvm = tmp_path / "kvm"
    kvm.touch()
    monkeypatch.setattr(enable_kvm, "KVM", kvm)
    assert enable_kvm.main([]) == 0
    assert [command for command, _ in calls] == [
        ["sudo", "tee", "/etc/udev/rules.d/99-kvm4all.rules"],
        ["sudo", "udevadm", "control", "--reload-rules"],
        ["sudo", "udevadm", "trigger", "--settle", "--name-match=kvm"],
    ]
    assert calls[0][1]["input"] == (
        'KERNEL=="kvm", GROUP="kvm", MODE="0666", OPTIONS+="static_node=kvm"\n'
    )
    assert all(options["check"] is True and options["timeout"] for _, options in calls)


def test_missing_kvm_fails_before_sudo(
    calls: list[tuple[list[str], dict[str, object]]],
    monkeypatch: pytest.MonkeyPatch,
    tmp_path: Path,
    capsys: pytest.CaptureFixture[str],
) -> None:
    monkeypatch.setattr(enable_kvm, "KVM", tmp_path / "kvm")
    assert enable_kvm.main([]) == 1
    assert not calls
    assert "nested virtualization" in capsys.readouterr().err


@pytest.mark.parametrize(
    "error", [subprocess.CalledProcessError(1, "udevadm"), subprocess.TimeoutExpired("udevadm", 60)]
)
def test_udev_failure_fails_the_step(
    monkeypatch: pytest.MonkeyPatch, tmp_path: Path, error: subprocess.SubprocessError
) -> None:
    kvm = tmp_path / "kvm"
    kvm.touch()
    monkeypatch.setattr(enable_kvm, "KVM", kvm)

    def run(command: list[str], **_: object) -> subprocess.CompletedProcess[str]:
        if "trigger" in command:
            raise error
        return subprocess.CompletedProcess(command, 0)

    monkeypatch.setattr(enable_kvm.subprocess, "run", run)
    assert enable_kvm.main([]) == 1
