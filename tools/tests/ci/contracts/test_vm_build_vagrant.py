"""Exercise the Vagrant VM build lifecycle without Vagrant, libvirt, or host mutations."""

from __future__ import annotations

import os
import subprocess
import sys
import time
from pathlib import Path
from typing import TYPE_CHECKING

import pytest

from ..._helpers import REPO_ROOT, load_script

if TYPE_CHECKING:
    from types import ModuleType

ARTIFACT = "/home/vagrant/shadow_build/QGroundControl-x86_64.AppImage"


@pytest.fixture
def script(monkeypatch: pytest.MonkeyPatch) -> ModuleType:
    module = load_script("deploy/vagrant/run_vagrant.py")
    # main() installs a SIGTERM handler; keep it out of the pytest process.
    monkeypatch.setattr(module.signal, "signal", lambda *_: None)
    return module


class FakeVagrant:
    """Stand in for run_vagrant.vagrant(); `failures` maps a subcommand to what it raises."""

    def __init__(
        self, failures: dict[str, BaseException] | None = None, state: str = "not_created"
    ) -> None:
        self.state = state
        self.failures = failures or {}
        self.calls: list[tuple[tuple[str, ...], float]] = []

    def __call__(self, *arguments: str, timeout: float, capture: bool = False) -> str:
        self.calls.append((arguments, timeout))
        if failure := self.failures.get(arguments[0]):
            raise failure
        if arguments[0] == "status":
            return f"1700000000,default,metadata,provider,libvirt\n1700000000,default,state,{self.state}\n"
        if arguments[0] == "ssh-config":
            return "Host default\n  HostName 192.0.2.10\n"
        return ""

    def commands(self) -> list[str]:
        return [arguments[0] for arguments, _ in self.calls]


@pytest.fixture
def host(script: ModuleType, monkeypatch: pytest.MonkeyPatch) -> list[list[str]]:
    """Record ssh/scp calls; the guest reports ARTIFACT as the build output."""
    calls: list[list[str]] = []

    def run(command: list[str], **kwargs: object) -> subprocess.CompletedProcess[str]:
        calls.append(command)
        if command[0] == "ssh":
            config = Path(command[command.index("-F") + 1])
            assert config.read_text() == "Host default\n  HostName 192.0.2.10\n"
        return subprocess.CompletedProcess(command, 0, f"{ARTIFACT}\n")

    monkeypatch.setattr(script.subprocess, "run", run)
    return calls


@pytest.mark.parametrize(
    ("vagrant", "expected_rc", "destroyed"),
    [
        (FakeVagrant({"validate": subprocess.CalledProcessError(3, "validate")}), 3, False),
        (FakeVagrant(state="running"), 1, False),
        (FakeVagrant({"up": subprocess.CalledProcessError(7, "up")}), 7, True),
        (FakeVagrant({"up": subprocess.CalledProcessError(-9, "up")}), 1, True),
        (FakeVagrant({"up": subprocess.TimeoutExpired("up", 1)}), 1, True),
        (FakeVagrant({"ssh-config": subprocess.CalledProcessError(5, "ssh-config")}), 5, True),
        (FakeVagrant(), 0, True),
    ],
)
def test_cleanup_requires_attempted_startup(
    script: ModuleType,
    host: list[list[str]],
    monkeypatch: pytest.MonkeyPatch,
    tmp_path: Path,
    vagrant: FakeVagrant,
    expected_rc: int,
    destroyed: bool,
) -> None:
    monkeypatch.setattr(script, "vagrant", vagrant)
    assert script.build(tmp_path / "out", 60) == expected_rc
    assert ("destroy" in vagrant.commands()) is destroyed
    assert ("up" in vagrant.commands()) is destroyed
    if destroyed:
        assert vagrant.commands()[-1] == "destroy"
        assert vagrant.calls[-1] == (("destroy", "-f"), script.DESTROY_TIMEOUT)


def test_successful_build_validates_then_copies_recorded_artifact(
    script: ModuleType, host: list[list[str]], monkeypatch: pytest.MonkeyPatch, tmp_path: Path
) -> None:
    vagrant = FakeVagrant()
    monkeypatch.setattr(script, "vagrant", vagrant)
    output = tmp_path / "output with spaces"
    assert script.build(output, 123) == 0
    assert vagrant.calls == [
        (("validate", "--ignore-provider"), script.COMMAND_TIMEOUT),
        (("status", "--machine-readable"), script.COMMAND_TIMEOUT),
        (("up", "--provider=libvirt"), 123),
        (("ssh-config",), script.COMMAND_TIMEOUT),
        (("destroy", "-f"), script.DESTROY_TIMEOUT),
    ]
    ssh, scp = host
    config = ssh[ssh.index("-F") + 1]
    assert ssh == ["ssh", "-F", config, "default", "cat", "/home/vagrant/qgc-appimage-path"]
    assert scp == ["scp", "-F", config, f"default:{ARTIFACT}", f"{output}/"]
    # The SSH configuration lives in a private temporary directory, removed after the copy.
    assert not config.startswith("/tmp/vagrant-ssh-config")
    assert not Path(config).exists()
    assert output.is_dir()


@pytest.mark.parametrize(
    "recorded", ["", "relative.AppImage", f"{ARTIFACT}\n{ARTIFACT}", "/home/vagrant/qgc.tar"]
)
def test_invalid_artifact_record_fails_and_destroys(
    script: ModuleType, monkeypatch: pytest.MonkeyPatch, tmp_path: Path, recorded: str
) -> None:
    calls: list[list[str]] = []
    monkeypatch.setattr(
        script.subprocess,
        "run",
        lambda command, **_: (
            calls.append(command) or subprocess.CompletedProcess(command, 0, recorded)
        ),
    )
    vagrant = FakeVagrant()
    monkeypatch.setattr(script, "vagrant", vagrant)
    assert script.build(tmp_path, 60) == 1
    assert [call[0] for call in calls] == ["ssh"]
    assert vagrant.commands()[-1] == "destroy"


@pytest.mark.parametrize(
    ("up_failure", "expected_rc"), [(None, 1), (subprocess.CalledProcessError(7, "up"), 7)]
)
def test_cleanup_failure_is_surfaced(
    script: ModuleType,
    host: list[list[str]],
    monkeypatch: pytest.MonkeyPatch,
    capsys: pytest.CaptureFixture[str],
    tmp_path: Path,
    up_failure: BaseException | None,
    expected_rc: int,
) -> None:
    failures: dict[str, BaseException] = {"destroy": subprocess.CalledProcessError(2, "destroy")}
    if up_failure:
        failures["up"] = up_failure
    monkeypatch.setattr(script, "vagrant", FakeVagrant(failures))
    assert script.build(tmp_path, 60) == expected_rc
    assert "VM cleanup failed" in capsys.readouterr().err


@pytest.mark.parametrize("interrupt", [KeyboardInterrupt(), SystemExit(143)])
def test_interrupted_build_still_destroys(
    script: ModuleType, monkeypatch: pytest.MonkeyPatch, tmp_path: Path, interrupt: BaseException
) -> None:
    vagrant = FakeVagrant({"up": interrupt})
    monkeypatch.setattr(script, "vagrant", vagrant)
    with pytest.raises(type(interrupt)):
        script.build(tmp_path, 60)
    assert vagrant.commands()[-1] == "destroy"


def test_sigterm_unwinds_like_an_interrupt(script: ModuleType) -> None:
    with pytest.raises(SystemExit) as error:
        script._exit_on_sigterm(15, None)
    assert error.value.code == 143


def test_main_builds_into_output_dir_with_up_timeout(
    script: ModuleType, monkeypatch: pytest.MonkeyPatch, tmp_path: Path
) -> None:
    handlers: list[object] = []
    monkeypatch.setattr(script.signal, "signal", lambda signum, handler: handlers.append(handler))
    builds: list[tuple[Path, float]] = []
    monkeypatch.setattr(
        script, "build", lambda output, timeout: builds.append((output, timeout)) or 0
    )
    monkeypatch.setenv("OUTPUT_DIR", str(tmp_path / "artifacts"))
    monkeypatch.setenv("QGC_VM_UP_TIMEOUT", "123")
    assert script.main(["build"]) == 0
    monkeypatch.delenv("QGC_VM_UP_TIMEOUT")
    assert script.main([]) == 0
    assert builds == [
        (tmp_path / "artifacts", 123.0),
        (tmp_path / "artifacts", float(script.UP_TIMEOUT)),
    ]
    assert handlers == [script._exit_on_sigterm] * 2


@pytest.mark.parametrize(("up_rc", "expected_rc"), [(0, 0), (6, 6)])
def test_cli_copies_artifact_and_destroys_started_vm(
    tmp_path: Path, up_rc: int, expected_rc: int
) -> None:
    log = tmp_path / "calls"
    tools = tmp_path / "bin"
    tools.mkdir()
    record = (
        f"#!{sys.executable}\nimport os, sys\n"
        "with open(os.environ['CALL_LOG'], 'a') as log:\n"
        "    log.write(' '.join(sys.argv[1:]) + '\\n')\n"
    )
    fakes = {
        "sg": "command = sys.argv[3]\n"
        "if command.startswith('vagrant status'): print('1,default,state,not_created')\n"
        "if command == 'vagrant ssh-config': print('Host default')\n"
        f"if command.startswith('vagrant up'): sys.exit({up_rc})\n",
        "ssh": f"print('{ARTIFACT}')\n",
        "scp": "open(sys.argv[-1] + 'QGroundControl-x86_64.AppImage', 'w').close()\n",
    }
    for name, body in fakes.items():
        (tools / name).write_text(record + body)
        (tools / name).chmod(0o755)
    output = tmp_path / "artifacts"
    result = subprocess.run(
        [sys.executable, str(REPO_ROOT / "deploy/vagrant/run_vagrant.py"), "build"],
        env={
            **os.environ,
            "PATH": f"{tools}:{os.environ['PATH']}",
            "CALL_LOG": str(log),
            "OUTPUT_DIR": str(output),
        },
        capture_output=True,
        text=True,
        check=False,
    )
    assert result.returncode == expected_rc, result.stderr
    calls = log.read_text().splitlines()
    assert calls[0] == "libvirt -c vagrant validate --ignore-provider"
    assert "libvirt -c vagrant up --provider=libvirt" in calls
    assert calls[-1] == "libvirt -c vagrant destroy -f"
    assert (output / "QGroundControl-x86_64.AppImage").is_file() is (up_rc == 0)


def _fake_sg(directory: Path, body: str) -> None:
    sg = directory / "sg"
    sg.write_text(
        f'#!/bin/sh\nprintf "%s\\n" "$@" "$PWD" "$QGC_CI" "$VAGRANT_DEFAULT_PROVIDER" > "$SG_LOG"\n{body}'
    )
    sg.chmod(0o755)


def test_vagrant_runs_under_libvirt_group_in_project_dir(
    script: ModuleType, monkeypatch: pytest.MonkeyPatch, tmp_path: Path
) -> None:
    _fake_sg(tmp_path, 'echo "captured output"\n')
    log = tmp_path / "sg.log"
    monkeypatch.setenv("PATH", f"{tmp_path}:{os.environ['PATH']}")
    monkeypatch.setenv("SG_LOG", str(log))
    monkeypatch.delenv("QGC_CI", raising=False)
    output = script.vagrant("up", "--provider=libvirt", timeout=30, capture=True)
    assert output == "captured output\n"
    assert log.read_text().splitlines() == [
        "libvirt",
        "-c",
        "vagrant up --provider=libvirt",
        str(REPO_ROOT / "deploy/vagrant"),
        "1",
        "libvirt",
    ]
    # An explicit QGC_CI still reaches the Vagrantfile.
    monkeypatch.setenv("QGC_CI", "")
    script.vagrant("validate", timeout=30)
    assert log.read_text().splitlines()[4] == ""


def test_vagrant_failure_raises_with_exit_code(
    script: ModuleType, monkeypatch: pytest.MonkeyPatch, tmp_path: Path
) -> None:
    _fake_sg(tmp_path, "exit 4\n")
    monkeypatch.setenv("PATH", f"{tmp_path}:{os.environ['PATH']}")
    monkeypatch.setenv("SG_LOG", str(tmp_path / "sg.log"))
    with pytest.raises(subprocess.CalledProcessError) as error:
        script.vagrant("destroy", "-f", timeout=30)
    assert error.value.returncode == 4


def _alive(pid: int) -> bool:
    try:
        stat = Path(f"/proc/{pid}/stat").read_text()
    except FileNotFoundError:
        return False
    return stat.rsplit(")", 1)[1].split()[0] != "Z"


@pytest.mark.skipif(not Path("/proc").is_dir(), reason="needs /proc to inspect descendants")
def test_vagrant_timeout_stops_every_descendant(
    script: ModuleType, monkeypatch: pytest.MonkeyPatch, tmp_path: Path
) -> None:
    pid_file = tmp_path / "descendant.pid"
    _fake_sg(tmp_path, f'sleep 30 &\necho $! > "{pid_file}"\nwait\n')
    monkeypatch.setenv("PATH", f"{tmp_path}:{os.environ['PATH']}")
    monkeypatch.setenv("SG_LOG", str(tmp_path / "sg.log"))
    monkeypatch.setattr(script, "KILL_GRACE", 5)
    started = time.monotonic()
    with pytest.raises(subprocess.TimeoutExpired):
        script.vagrant("up", timeout=1)
    # SIGTERM to the whole group ends sg's shell at once rather than after the grace period.
    assert time.monotonic() - started < 1 + script.KILL_GRACE
    descendant = int(pid_file.read_text())
    deadline = time.monotonic() + 5
    while _alive(descendant) and time.monotonic() < deadline:
        time.sleep(0.05)
    assert not _alive(descendant)


@pytest.fixture
def install_calls(
    script: ModuleType, monkeypatch: pytest.MonkeyPatch
) -> list[tuple[list[str], dict[str, object]]]:
    calls: list[tuple[list[str], dict[str, object]]] = []

    def run(command: list[str], **kwargs: object) -> subprocess.CompletedProcess[str]:
        calls.append((command, kwargs))
        return subprocess.CompletedProcess(command, 0, "-----BEGIN PGP PUBLIC KEY BLOCK-----\n")

    monkeypatch.setattr(script.subprocess, "run", run)
    monkeypatch.setattr(
        script.platform,
        "freedesktop_os_release",
        lambda: {"VERSION_CODENAME": "derivative", "UBUNTU_CODENAME": "noble"},
    )
    monkeypatch.setattr(script.getpass, "getuser", lambda: "runner")
    return calls


def test_install_bounds_hashicorp_key_download(
    script: ModuleType, install_calls: list[tuple[list[str], dict[str, object]]]
) -> None:
    assert script.main(["install"]) == 0
    (curl, curl_options), (gpg, gpg_options) = install_calls[:2]
    assert curl[0] == "curl"
    assert curl[-1] == "https://apt.releases.hashicorp.com/gpg"
    flags = " ".join(curl)
    for option in (
        "--fail",
        "--retry 5",
        "--retry-delay 2",
        "--retry-max-time 120",
        "--connect-timeout 10",
        "--max-time 30",
    ):
        assert option in flags
    assert curl_options["check"] is True
    assert curl_options["timeout"] == script.KEY_TIMEOUT
    assert gpg[:2] == ["sudo", "gpg"]
    assert "--dearmor" in gpg
    assert gpg[-2:] == ["-o", "/usr/share/keyrings/hashicorp-archive-keyring.gpg"]
    assert gpg_options["input"] == "-----BEGIN PGP PUBLIC KEY BLOCK-----\n"


def test_install_uses_hashicorp_repo_and_shared_apt_retry(
    script: ModuleType, install_calls: list[tuple[list[str], dict[str, object]]]
) -> None:
    assert script.main(["install"]) == 0
    commands = [command for command, _ in install_calls]
    sources, sources_options = install_calls[2]
    assert sources == ["sudo", "tee", "/etc/apt/sources.list.d/hashicorp.list"]
    assert sources_options["input"] == (
        "deb [signed-by=/usr/share/keyrings/hashicorp-archive-keyring.gpg] "
        "https://apt.releases.hashicorp.com noble main\n"
    )
    apt = commands[3]
    assert apt[1] == str(REPO_ROOT / "tools/ci/runner/apt_install_retry.py")
    assert apt[2:6] == ["--update", "--no-install-recommends", "--label", "Vagrant and libvirt"]
    assert {"vagrant", "qemu-kvm", "libvirt-daemon-system", "libvirt-dev", "ruby-dev"} <= set(apt)
    assert "ovmf" in apt
    assert commands[4:] == [
        ["sudo", "systemctl", "enable", "--now", "libvirtd"],
        ["sudo", "usermod", "-aG", "libvirt,kvm", "runner"],
        ["sudo", "-u", "runner", "vagrant", "plugin", "install", "vagrant-libvirt"],
    ]
    assert all(options.get("check") is True for _, options in install_calls)
    # Everything but apt_install_retry.py, which bounds apt itself under sudo, has a timeout.
    assert all("timeout" in options for command, options in install_calls if command != apt)


def test_install_stops_when_key_download_fails(
    script: ModuleType,
    install_calls: list[tuple[list[str], dict[str, object]]],
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    def run(command: list[str], **kwargs: object) -> subprocess.CompletedProcess[str]:
        install_calls.append((command, kwargs))
        raise subprocess.CalledProcessError(22, command)

    monkeypatch.setattr(script.subprocess, "run", run)
    assert script.main(["install"]) == 1
    assert [command[0] for command, _ in install_calls] == ["curl"]


def test_install_requires_an_ubuntu_codename(
    script: ModuleType,
    install_calls: list[tuple[list[str], dict[str, object]]],
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    monkeypatch.setattr(script.platform, "freedesktop_os_release", lambda: {"ID": "debian"})
    assert script.main(["install"]) == 1
    assert not any("tee" in command for command, _ in install_calls)
