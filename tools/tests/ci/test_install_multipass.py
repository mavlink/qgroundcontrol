"""Multipass bootstrap readiness and error handling."""

import subprocess
from pathlib import Path

import pytest
from ci.runner import install_multipass


@pytest.fixture
def calls(monkeypatch):
    result = []
    monkeypatch.setattr(install_multipass.shutil, "which", lambda _: None)
    monkeypatch.setattr(install_multipass.time, "sleep", lambda _: None)
    monkeypatch.setattr(
        install_multipass.subprocess,
        "run",
        lambda command, **kwargs: result.append(command) or subprocess.CompletedProcess(command, 0),
    )
    return result


def test_missing_socket_stops_before_permissions_or_manifest(calls, monkeypatch):
    monkeypatch.setattr(Path, "is_socket", lambda _: False)
    assert install_multipass.main([]) == 1
    assert ["sudo", "systemctl", "enable", "--now", "snapd.socket"] in calls
    assert not any("chmod" in call or "find" in call for call in calls)


def test_existing_snap_is_refreshed_and_waits_for_catalog(calls, monkeypatch):
    monkeypatch.setattr(install_multipass.shutil, "which", lambda _: "/usr/bin/snap")
    monkeypatch.setattr(Path, "is_socket", lambda _: True)
    assert install_multipass.main([]) == 0
    assert not any("apt-get" in call for call in calls)
    assert ["sudo", "snap", "refresh", "multipass"] in calls
    assert calls[-1] == ["sudo", "multipass", "find"]


def test_bad_secondary_remote_is_bounded_and_warns(calls, monkeypatch, capsys):
    monkeypatch.setattr(Path, "is_socket", lambda _: True)

    def run(command, **kwargs):
        calls.append(command)
        return subprocess.CompletedProcess(command, 1 if "find" in command else 0)

    monkeypatch.setattr(install_multipass.subprocess, "run", run)
    assert install_multipass.main([]) == 0
    assert calls.count(["sudo", "multipass", "find"]) == install_multipass.CATALOG_ATTEMPTS
    assert "::warning::" in capsys.readouterr().out


FORWARDING = [
    ["sudo", "iptables", "-P", "FORWARD", "ACCEPT"],
    ["sudo", "sysctl", "-w", "net.ipv4.ip_forward=1"],
]


@pytest.mark.parametrize(
    ("argv", "forwarding"), [([], []), (["--allow-bridge-forwarding"], FORWARDING)]
)
def test_bridge_forwarding_is_opened_only_on_request_after_install(
    calls, monkeypatch, argv, forwarding
):
    monkeypatch.setattr(Path, "is_socket", lambda _: True)
    assert install_multipass.main(argv) == 0
    assert [call for call in calls if call in FORWARDING] == forwarding
    if forwarding:
        assert calls[-2:] == FORWARDING
        assert calls.index(["sudo", "snap", "install", "multipass"]) < calls.index(FORWARDING[0])


def test_bridge_forwarding_failure_fails_the_install(calls, monkeypatch):
    monkeypatch.setattr(Path, "is_socket", lambda _: True)

    def run(command, **kwargs):
        calls.append(command)
        return subprocess.CompletedProcess(command, 0)

    def failing_iptables(command, **kwargs):
        if command[:2] == ["sudo", "iptables"]:
            raise subprocess.CalledProcessError(4, command)
        return run(command, **kwargs)

    monkeypatch.setattr(install_multipass.subprocess, "run", failing_iptables)
    assert install_multipass.main(["--allow-bridge-forwarding"]) == 1
    assert ["sudo", "sysctl", "-w", "net.ipv4.ip_forward=1"] not in calls
