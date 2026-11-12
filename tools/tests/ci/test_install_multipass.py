"""Multipass bootstrap readiness and error handling."""

import subprocess
from pathlib import Path

import pytest
from ci.runner import install_multipass

from .._helpers import get_steps, load_workflow

INSTALL = ["sudo", "snap", "install", "multipass"]


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


def _failing_installs(calls, failures):
    """A subprocess.run whose first *failures* `snap install multipass` calls fail."""

    def run(command, **kwargs):
        calls.append(command)
        failed = command == INSTALL and calls.count(INSTALL) <= failures
        return subprocess.CompletedProcess(command, 1 if failed else 0)

    return run


@pytest.mark.parametrize(("argv", "status"), [(["--allow-store-outage"], 0), ([], 1)])
def test_a_snap_store_outage_fails_unless_the_job_allows_it(
    calls, monkeypatch, tmp_path, capsys, argv, status
):
    output = tmp_path / "output"
    monkeypatch.setenv("GITHUB_OUTPUT", str(output))
    monkeypatch.setattr(Path, "is_socket", lambda _: True)
    attempts = install_multipass.STORE_ATTEMPTS
    monkeypatch.setattr(install_multipass.subprocess, "run", _failing_installs(calls, attempts))
    assert install_multipass.main([*argv, "--allow-bridge-forwarding"]) == status
    # Nothing follows the last attempt: no refresh, no daemon wait, no firewall change.
    assert calls.count(INSTALL) == attempts
    assert calls[-1] == INSTALL
    captured = capsys.readouterr()
    if status:
        assert not output.exists()
        assert "Multipass installation failed" in captured.err
    else:
        assert output.read_text(encoding="utf-8") == "installed=false\n"
        assert "::warning title=Multipass build skipped::" in captured.out


def test_a_store_command_that_fails_once_is_tried_again(calls, monkeypatch, tmp_path, capsys):
    output = tmp_path / "output"
    monkeypatch.setenv("GITHUB_OUTPUT", str(output))
    monkeypatch.setattr(Path, "is_socket", lambda _: True)
    monkeypatch.setattr(install_multipass.subprocess, "run", _failing_installs(calls, 1))
    assert install_multipass.main(["--allow-store-outage"]) == 0
    assert calls.count(INSTALL) == 2
    assert output.read_text(encoding="utf-8") == "installed=true\n"
    assert "Multipass build skipped" not in capsys.readouterr().out


def test_allowing_a_store_outage_excuses_no_other_failure(calls, monkeypatch, tmp_path):
    output = tmp_path / "output"
    monkeypatch.setenv("GITHUB_OUTPUT", str(output))
    monkeypatch.setattr(Path, "is_socket", lambda _: False)
    assert install_multipass.main(["--allow-store-outage"]) == 1
    assert not output.exists()


def test_the_multipass_job_builds_only_when_multipass_was_installed():
    # A step that ignored the output would fail on a store outage, and one that read another
    # output's name would skip the build for good while the job stayed green.
    steps = get_steps(load_workflow("vm-builds.yml"), "multipass-build")
    install = next(step for step in steps if "install_multipass.py" in str(step.get("run")))
    assert "--allow-store-outage" in install["run"]
    later = steps[steps.index(install) + 1 :]
    condition = f"steps.{install['id']}.outputs.installed == 'true'"
    assert later
    assert all(step.get("if") == condition for step in later)
