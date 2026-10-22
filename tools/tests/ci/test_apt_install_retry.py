"""Exercise bounded apt retries and cache cleanup without installing packages."""

from __future__ import annotations

import os
import subprocess
from pathlib import Path
from typing import TYPE_CHECKING

import pytest
from ci.runner import apt_install_retry

if TYPE_CHECKING:
    from pytest_subprocess import FakeProcess

ARCHIVE_SOURCES = """Types: deb
URIs: https://archive.ubuntu.com/ubuntu/
Suites: noble noble-updates noble-backports
Components: main restricted universe multiverse
Signed-By: /usr/share/keyrings/ubuntu-archive-keyring.gpg

Types: deb
URIs: https://security.ubuntu.com/ubuntu/
Suites: noble-security
Components: main restricted universe multiverse
Signed-By: /usr/share/keyrings/ubuntu-archive-keyring.gpg
"""
PORTS_SOURCES = """Types: deb
URIs: http://ports.ubuntu.com/ubuntu-ports/
Suites: noble noble-updates noble-backports
Components: main universe

Types: deb
URIs: http://ports.ubuntu.com/ubuntu-ports/
Suites: noble-security
Components: main universe
"""
LEGACY_SOURCES = """# deb http://archive.ubuntu.com/ubuntu/ jammy-proposed main
deb http://archive.ubuntu.com/ubuntu/ jammy main restricted
deb-src http://archive.ubuntu.com/ubuntu/ jammy-updates main
deb [arch=amd64 signed-by=/usr/share/keyrings/u.gpg] https://archive.ubuntu.com/ubuntu jammy-backports main
deb http://security.ubuntu.com/ubuntu jammy-security main
deb http://archive.ubuntu.com/ubuntu jammy-security universe
deb http://us.archive.ubuntu.com/ubuntu jammy main
deb http://archive.ubuntu.com.example.org/ubuntu jammy main
"""
OTHER_SOURCES = {
    "hashicorp.list": "deb [signed-by=/k.gpg] https://apt.releases.hashicorp.com noble main\n",
    "mirrors.sources": "Types: deb\nURIs: mirror+file:/etc/apt/apt-mirrors.txt\nSuites: noble\n",
}


@pytest.fixture(autouse=True)
def _outside_runson(monkeypatch):
    monkeypatch.delenv("RUNS_ON_AWS_REGION", raising=False)


@pytest.fixture
def apt_dir(tmp_path, monkeypatch):
    root = tmp_path / "etc-apt"
    (root / "sources.list.d").mkdir(parents=True)
    (root / "sources.list").write_text(LEGACY_SOURCES)
    (root / "sources.list.d" / "ubuntu.sources").write_text(ARCHIVE_SOURCES)
    for name, text in OTHER_SOURCES.items():
        (root / "sources.list.d" / name).write_text(text)
    monkeypatch.setattr(apt_install_retry, "_resolves", lambda host: True)
    monkeypatch.setattr(
        apt_install_retry, "_write_root_file", lambda path, text: path.write_text(text)
    )
    monkeypatch.setenv("RUNS_ON_AWS_REGION", "us-west-2")
    return root


def _snapshot(root: Path) -> dict[str, str]:
    return {
        str(path.relative_to(root)): path.read_text() for path in root.rglob("*") if path.is_file()
    }


def _write_executable(path: Path, content: str) -> None:
    path.write_text(content, encoding="utf-8")
    path.chmod(0o755)


def test_update_retries_finish_before_progressive_install_retries(
    tmp_path: Path, monkeypatch
) -> None:
    fake_bin = tmp_path / "bin"
    fake_bin.mkdir()
    call_log = tmp_path / "calls.log"
    update_count = tmp_path / "update-count"
    install_count = tmp_path / "install-count"

    _write_executable(
        fake_bin / "sudo",
        """#!/usr/bin/env bash
set -euo pipefail
printf '%s\n' "$*" >> "${APT_RETRY_CALL_LOG}"
command_name="$1"
shift
case "${command_name}" in
  mkdir|chmod|rm) command "${command_name}" "$@" ;;
  chown|apt-get) exit 0 ;;
  timeout)
    if [[ " $* " == *" update "* ]]; then
      count=0
      [[ -f "${APT_UPDATE_COUNT}" ]] && read -r count < "${APT_UPDATE_COUNT}"
      count=$((count + 1))
      printf '%s\n' "${count}" > "${APT_UPDATE_COUNT}"
      ((count >= 2))
    elif [[ " $* " == *" install "* ]]; then
      count=0
      [[ -f "${APT_INSTALL_COUNT}" ]] && read -r count < "${APT_INSTALL_COUNT}"
      count=$((count + 1))
      printf '%s\n' "${count}" > "${APT_INSTALL_COUNT}"
      ((count >= 3))
    fi
    ;;
esac
""",
    )
    monkeypatch.setenv("APT_INSTALL_COUNT", str(install_count))
    monkeypatch.setenv("APT_RETRY_CALL_LOG", str(call_log))
    monkeypatch.setenv("APT_UPDATE_COUNT", str(update_count))
    monkeypatch.setenv("PATH", f"{fake_bin}{os.pathsep}{os.environ['PATH']}")
    monkeypatch.setattr(Path, "home", lambda: tmp_path / "home")
    delays = []
    monkeypatch.setattr(apt_install_retry.time, "sleep", delays.append)

    assert apt_install_retry.main(["--update", "--autoclean", "fake-package"]) == 0
    assert delays == [15, 15, 30]
    calls = call_log.read_text(encoding="utf-8").splitlines()
    operations = [
        operation
        for call in calls
        for operation in ("update", "install")
        if f" {operation} " in f" {call} "
    ]
    assert operations == ["update", "update", "install", "install", "install"]


@pytest.mark.parametrize("failure", [None, "update", "install", "autoclean"])
def test_failures_preserve_timeout_policy_and_cache_ownership(tmp_path, monkeypatch, failure):
    monkeypatch.setattr(Path, "home", lambda: tmp_path)
    calls = []
    delays = []
    monkeypatch.setattr(apt_install_retry.time, "sleep", delays.append)

    def run(command, *, check, **kwargs):
        calls.append(command)
        code = 100 if failure in command else 0
        if check and code:
            raise subprocess.CalledProcessError(code, command)
        return subprocess.CompletedProcess(command, code)

    monkeypatch.setattr(apt_install_retry.subprocess, "run", run)
    result = apt_install_retry.main(
        ["--update", "--autoclean", "--no-install-recommends", "--install-timeout", "42", "pkg"]
    )
    assert result == (1 if failure in ("update", "install") else 0)
    attempts = [command for command in calls if command[:2] == ["sudo", "timeout"]]
    updates = [command for command in attempts if "update" in command]
    installs = [command for command in attempts if "install" in command]
    assert len(updates) == (3 if failure == "update" else 1)
    assert len(installs) == (0 if failure == "update" else 3 if failure == "install" else 1)
    assert delays == ([15, 30] if failure in ("update", "install") else [])
    for command in attempts:
        assert command[:6] == [
            "sudo",
            "timeout",
            "-k",
            "30",
            "300" if "update" in command else "42",
            "apt-get",
        ]
        assert "Acquire::Retries=3" in command
        assert "Acquire::http::Timeout=30" in command
        assert "Acquire::https::Timeout=30" in command
        assert "DPkg::Lock::Timeout=300" in command
        assert f"Dir::Cache::Archives={tmp_path}/.cache/qgc-apt-archives" in command
        assert "APT::Keep-Downloaded-Packages=true" in command
    for command in installs:
        assert command[-4:] == ["install", "-y", "--no-install-recommends", "pkg"]
    assert "autoclean" in calls[-3]
    assert calls[-2] == ["sudo", "rm", "-rf", str(tmp_path / ".cache/qgc-apt-archives/partial")]
    assert calls[-1] == [
        "sudo",
        "chown",
        "-R",
        f"{os.getuid()}:{os.getgid()}",
        str(tmp_path / ".cache/qgc-apt-archives"),
    ]


@pytest.mark.parametrize("args", [[], ["--install-timeout", "0", "pkg"], ["--unknown", "pkg"]])
def test_invalid_arguments_fail_before_running_commands(args, fp: FakeProcess, capsys):
    with pytest.raises(SystemExit) as exited:
        apt_install_retry.main(args)
    assert exited.value.code == 2
    assert "error:" in capsys.readouterr().err
    assert not fp.calls


def test_ec2_mirror_rewrites_only_non_security_ubuntu_archive_entries(apt_dir):
    before = _snapshot(apt_dir)
    originals = apt_install_retry.use_ec2_mirror(apt_dir)
    after = _snapshot(apt_dir)

    assert originals == {
        apt_dir / "sources.list": LEGACY_SOURCES,
        apt_dir / "sources.list.d" / "ubuntu.sources": ARCHIVE_SOURCES,
    }
    mirror = "http://us-west-2.ec2.archive.ubuntu.com/ubuntu"
    assert after["sources.list.d/ubuntu.sources"] == ARCHIVE_SOURCES.replace(
        "https://archive.ubuntu.com/ubuntu", mirror
    )
    assert after["sources.list"].splitlines() == [
        "# deb http://archive.ubuntu.com/ubuntu/ jammy-proposed main",
        f"deb {mirror}/ jammy main restricted",
        f"deb-src {mirror}/ jammy-updates main",
        f"deb [arch=amd64 signed-by=/usr/share/keyrings/u.gpg] {mirror} jammy-backports main",
        "deb http://security.ubuntu.com/ubuntu jammy-security main",
        "deb http://archive.ubuntu.com/ubuntu jammy-security universe",
        "deb http://us.archive.ubuntu.com/ubuntu jammy main",
        "deb http://archive.ubuntu.com.example.org/ubuntu jammy main",
    ]
    for name in OTHER_SOURCES:
        assert after[f"sources.list.d/{name}"] == before[f"sources.list.d/{name}"]

    assert apt_install_retry.use_ec2_mirror(apt_dir) == {}
    assert _snapshot(apt_dir) == after


def test_ec2_mirror_moves_arm64_ports_but_not_security(apt_dir):
    sources = apt_dir / "sources.list.d" / "ubuntu.sources"
    sources.write_text(PORTS_SOURCES)
    apt_install_retry.use_ec2_mirror(apt_dir)
    stanzas = sources.read_text().split("\n\n")
    assert "URIs: http://us-west-2.ec2.ports.ubuntu.com/ubuntu-ports/" in stanzas[0]
    assert stanzas[1] == PORTS_SOURCES.split("\n\n")[1]


def test_region_without_ports_mirror_keeps_ports_sources(apt_dir, monkeypatch):
    monkeypatch.setattr(apt_install_retry, "_resolves", lambda host: "ports" not in host)
    sources = apt_dir / "sources.list.d" / "ubuntu.sources"
    sources.write_text(PORTS_SOURCES)
    originals = apt_install_retry.use_ec2_mirror(apt_dir)
    assert sources not in originals
    assert sources.read_text() == PORTS_SOURCES


@pytest.mark.parametrize("region", [None, "", "us-west-2\ndeb http://evil", "../x", "US-WEST-2"])
def test_ec2_mirror_is_a_no_op_off_runson(apt_dir, monkeypatch, region):
    if region is None:
        monkeypatch.delenv("RUNS_ON_AWS_REGION")
    else:
        monkeypatch.setenv("RUNS_ON_AWS_REGION", region)
    before = _snapshot(apt_dir)
    assert apt_install_retry.use_ec2_mirror(apt_dir) == {}
    assert _snapshot(apt_dir) == before


def test_write_failure_restores_original_sources(apt_dir, monkeypatch, capsys):
    before = _snapshot(apt_dir)

    def write(path, text):
        if path.suffix == ".sources" and "ec2" in text:
            path.write_text("partial")
            raise subprocess.CalledProcessError(1, ["sudo", "tee", str(path)])
        path.write_text(text)

    monkeypatch.setattr(apt_install_retry, "_write_root_file", write)
    assert apt_install_retry.use_ec2_mirror(apt_dir) == {}
    assert _snapshot(apt_dir) == before
    assert "::warning::Keeping the original apt sources" in capsys.readouterr().out


@pytest.mark.parametrize("mirror_works", [True, False])
def test_failed_mirror_update_falls_back_to_original_sources(
    apt_dir, tmp_path, monkeypatch, mirror_works
):
    monkeypatch.setattr(apt_install_retry, "APT_DIR", apt_dir)
    monkeypatch.setattr(Path, "home", lambda: tmp_path)
    monkeypatch.setattr(apt_install_retry.time, "sleep", lambda _seconds: None)
    sources = apt_dir / "sources.list.d" / "ubuntu.sources"
    updates = []

    def run(command, *, check, **kwargs):
        if command[:2] == ["sudo", "timeout"] and "update" in command:
            updates.append(sources.read_text())
            code = 0 if mirror_works or len(updates) > 1 else 100
            return subprocess.CompletedProcess(command, code)
        return subprocess.CompletedProcess(command, 0)

    monkeypatch.setattr(apt_install_retry.subprocess, "run", run)
    assert apt_install_retry.main(["--update"]) == 0
    mirrored = ARCHIVE_SOURCES.replace(
        "https://archive.ubuntu.com", "http://us-west-2.ec2.archive.ubuntu.com"
    )
    if mirror_works:
        assert updates == [mirrored]
        assert sources.read_text() == mirrored
    else:
        assert updates == [mirrored, ARCHIVE_SOURCES]
        assert sources.read_text() == ARCHIVE_SOURCES


def test_update_only_mode_installs_nothing(tmp_path, monkeypatch):
    monkeypatch.setattr(Path, "home", lambda: tmp_path)
    calls = []

    def run(command, *, check, **kwargs):
        calls.append(command)
        return subprocess.CompletedProcess(command, 0)

    monkeypatch.setattr(apt_install_retry.subprocess, "run", run)
    assert apt_install_retry.main(["--update"]) == 0
    apt_calls = [command for command in calls if command[:2] == ["sudo", "timeout"]]
    assert [command[-2:] for command in apt_calls] == [["update", "-qq"]]
