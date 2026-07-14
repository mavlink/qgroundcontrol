"""Contracts for VM provisioning reliability."""

from __future__ import annotations

from _helpers import REPO_ROOT


def test_vagrant_apt_uses_https_and_whole_command_retries() -> None:
    vagrantfile = (REPO_ROOT / "deploy/vagrant/Vagrantfile").read_text(encoding="utf-8")

    assert "apt_retry()" in vagrantfile
    assert "apt_retry sudo apt-get update" in vagrantfile
    assert "apt_retry sudo DEBIAN_FRONTEND=noninteractive apt-get" in vagrantfile
    assert "https://archive.ubuntu.com/ubuntu" in vagrantfile
    assert "https://security.ubuntu.com/ubuntu" in vagrantfile
