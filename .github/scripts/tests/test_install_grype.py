"""Grype bootstrap integrity, retry, tool-cache and workflow contracts."""

from __future__ import annotations

import io
import shutil
import subprocess
import tarfile
from email.message import Message
from types import SimpleNamespace
from urllib.error import HTTPError

import install_grype
import pytest
import yaml
from _helpers import REPO_ROOT
from common.io import sha256_file


@pytest.fixture
def release(tmp_path, monkeypatch):
    archive = tmp_path / "release.tar.gz"
    content = b"#!/bin/sh\necho 'Version: 0.110.0'\n"
    with tarfile.open(archive, "w:gz") as tar:
        member = tarfile.TarInfo("grype")
        member.size = len(content)
        tar.addfile(member, io.BytesIO(content))
    digest = sha256_file(archive)
    monkeypatch.setattr(install_grype, "SHA256", dict.fromkeys(("x86_64", "aarch64"), digest))
    return archive


@pytest.mark.parametrize(
    "arch,asset,node", [("x86_64", "amd64", "x64"), ("aarch64", "arm64", "arm64")]
)
def test_direct_asset_retries_and_populates_scan_action_cache(
    tmp_path, monkeypatch, release, arch, asset, node
):
    calls = []
    sleeps = []

    def download(url, destination, **kwargs):
        calls.append(url)
        if len(calls) < 3:
            raise HTTPError(url, 504, "Gateway Timeout", Message(), None)
        shutil.copyfile(release, destination)

    monkeypatch.setattr("common.net.download_file", download)
    monkeypatch.setattr("common.net.time", SimpleNamespace(sleep=sleeps.append))
    binary = install_grype.install(tmp_path / "cache", arch)
    assert (
        calls
        == [
            f"https://github.com/anchore/grype/releases/download/v{install_grype.VERSION}/"
            f"grype_{install_grype.VERSION}_linux_{asset}.tar.gz"
        ]
        * 3
    )
    assert sleeps == [10, 10]
    assert binary == tmp_path / "cache/grype" / install_grype.VERSION / node / "grype"
    assert binary.stat().st_mode & 0o111
    assert binary.parent.with_name(f"{node}.complete").is_file()
    assert install_grype.install(tmp_path / "cache", arch) == binary
    assert len(calls) == 3


@pytest.mark.parametrize("failure", ["checksum", "execution", "missing-binary"])
def test_failed_install_never_publishes_cache_marker(tmp_path, monkeypatch, release, failure):
    if failure == "missing-binary":
        with tarfile.open(release, "w:gz"):
            pass
        monkeypatch.setattr(install_grype, "SHA256", {"x86_64": sha256_file(release)})
    if failure == "checksum":
        monkeypatch.setattr(install_grype, "SHA256", {"x86_64": "0" * 64})
    monkeypatch.setattr(
        install_grype,
        "download_with_retry",
        lambda url, destination, **kwargs: shutil.copyfile(release, destination),
    )
    if failure == "execution":

        def fail(command, **kwargs):
            raise subprocess.CalledProcessError(1, command)

        monkeypatch.setattr(install_grype.subprocess, "run", fail)
    with pytest.raises((RuntimeError, subprocess.CalledProcessError)):
        install_grype.install(tmp_path / "cache", "x86_64")
    assert not list((tmp_path / "cache").rglob("*.complete"))


def test_incomplete_cache_is_repaired_and_cli_exports_exact_version(tmp_path, monkeypatch, release):
    cache = tmp_path / "cache"
    binary = cache / "grype" / install_grype.VERSION / "x64/grype"
    binary.parent.mkdir(parents=True)
    binary.write_text("incomplete")
    monkeypatch.setattr(
        install_grype,
        "download_with_retry",
        lambda url, destination, **kwargs: shutil.copyfile(release, destination),
    )
    monkeypatch.setattr(install_grype, "host_arch", lambda: "x86_64")
    monkeypatch.setattr(install_grype, "is_linux", lambda: True)
    monkeypatch.setenv("RUNNER_TOOL_CACHE", str(cache))
    output = tmp_path / "outputs"
    monkeypatch.setenv("GITHUB_OUTPUT", str(output))
    monkeypatch.setattr("sys.argv", ["install_grype.py"])
    install_grype.main()
    assert "Version:" in binary.read_text()
    assert f"version={install_grype.VERSION}\n" in output.read_text()
    assert f"cmd={binary}\n" in output.read_text()


def test_both_scans_reuse_verified_installer_version():
    workflow = yaml.safe_load((REPO_ROOT / ".github/workflows/docker.yml").read_text())
    steps = workflow["jobs"]["build"]["steps"]
    installer = next(step for step in steps if step.get("id") == "install-grype")
    assert installer["run"] == "python3 .github/scripts/install_grype.py"
    assert not installer.get("continue-on-error", False)
    scans = [step for step in steps if step.get("uses") == "anchore/scan-action@v7"]
    assert len(scans) == 2
    for scan in scans:
        assert steps.index(installer) < steps.index(scan)
        assert scan["with"]["grype-version"] == "${{ steps.install-grype.outputs.version }}"
    assert not any("download-grype" in step.get("uses", "") for step in steps)
