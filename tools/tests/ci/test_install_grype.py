"""Grype bootstrap integrity, retry, tool-cache and workflow contracts."""

from __future__ import annotations

import io
import shutil
import subprocess
import tarfile
from email.message import Message
from types import SimpleNamespace
from urllib.error import HTTPError

import pytest
from ci.security import install_grype
from common.build_config import load_build_config
from common.io import sha256_file

from .._helpers import load_workflow, read_github_file

PIN = load_build_config()["tools"]["grype"]
VERSION = PIN["version"]


@pytest.fixture
def release(tmp_path):
    archive = tmp_path / "release.tar.gz"
    content = f"#!/bin/sh\necho 'Version: {VERSION}'\n".encode()
    with tarfile.open(archive, "w:gz") as tar:
        member = tarfile.TarInfo("grype")
        member.size = len(content)
        tar.addfile(member, io.BytesIO(content))
    return archive


def _pin(archive):
    return {**PIN, "sha256": dict.fromkeys(("x86_64", "aarch64"), sha256_file(archive))}


@pytest.mark.parametrize(
    "arch,asset,node", [("x86_64", "amd64", "x64"), ("aarch64", "arm64", "arm64")]
)
def test_direct_asset_retries_and_populates_scan_action_cache(
    tmp_path, monkeypatch, release, arch, asset, node
):
    calls = []
    sleeps = []

    url = (
        f"https://github.com/anchore/grype/releases/download/v{VERSION}/"
        f"grype_{VERSION}_linux_{asset}.tar.gz"
    )

    def urlopen(request, **kwargs):
        calls.append(request.full_url)
        if len(calls) < 3:
            raise HTTPError(request.full_url, 504, "Gateway Timeout", Message(), None)
        return release.open("rb")

    monkeypatch.setattr("common.net.urllib.request.urlopen", urlopen)
    monkeypatch.setattr("common.net.time", SimpleNamespace(sleep=sleeps.append))
    binary = install_grype.install(tmp_path / "cache", arch, _pin(release))
    assert calls == [url] * 3
    assert len(sleeps) == 2
    assert binary == tmp_path / "cache/grype" / VERSION / node / "grype"
    assert binary.stat().st_mode & 0o111
    assert binary.parent.with_name(f"{node}.complete").is_file()
    assert install_grype.install(tmp_path / "cache", arch, _pin(release)) == binary
    assert len(calls) == 3


@pytest.mark.parametrize("failure", ["checksum", "execution", "missing-binary"])
def test_failed_install_never_publishes_cache_marker(tmp_path, monkeypatch, release, failure):
    if failure == "missing-binary":
        with tarfile.open(release, "w:gz"):
            pass
    pin = {**PIN, "sha256": {"x86_64": "0" * 64}} if failure == "checksum" else _pin(release)

    def download(url, destination, *, expected_sha256, **kwargs):
        if sha256_file(release) != expected_sha256:
            raise RuntimeError("SHA-256 mismatch")
        shutil.copyfile(release, destination)

    monkeypatch.setattr("common.net.download_file", download)
    if failure == "execution":

        def fail(command, **kwargs):
            raise subprocess.CalledProcessError(1, command)

        monkeypatch.setattr(install_grype.subprocess, "run", fail)
    with pytest.raises((RuntimeError, subprocess.CalledProcessError)):
        install_grype.install(tmp_path / "cache", "x86_64", pin)
    assert not list((tmp_path / "cache").rglob("*.complete"))


def test_incomplete_cache_is_repaired_and_cli_exports_exact_version(
    tmp_path, monkeypatch, release, gh_output
):
    cache = tmp_path / "cache"
    binary = cache / "grype" / VERSION / "x64/grype"
    binary.parent.mkdir(parents=True)
    binary.write_text("incomplete")
    digests = []

    def download(url, destination, *, expected_sha256, **kwargs):
        digests.append(expected_sha256)
        shutil.copyfile(release, destination)

    monkeypatch.setattr("common.net.download_file", download)
    monkeypatch.setattr(install_grype, "host_arch", lambda: "x86_64")
    monkeypatch.setattr(install_grype, "is_linux", lambda: True)
    monkeypatch.setenv("RUNNER_TOOL_CACHE", str(cache))
    assert install_grype.main([]) == 0
    assert "Version:" in binary.read_text()
    assert read_github_file(gh_output) == {"version": VERSION, "cmd": str(binary)}
    assert digests == [PIN["sha256"]["x86_64"]]


def test_both_scans_reuse_verified_installer_version():
    workflow = load_workflow("docker.yml")
    steps = workflow["jobs"]["build"]["steps"]
    installer = next(step for step in steps if step.get("id") == "install-grype")
    assert installer["run"] == "python3 tools/ci/security/install_grype.py"
    assert not installer.get("continue-on-error", False)
    scans = [step for step in steps if step.get("uses") == "anchore/scan-action@v7"]
    assert len(scans) == 2
    for scan in scans:
        assert steps.index(installer) < steps.index(scan)
        assert scan["with"]["grype-version"] == "${{ steps.install-grype.outputs.version }}"
    assert not any("download-grype" in step.get("uses", "") for step in steps)
