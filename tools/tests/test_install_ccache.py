"""Tests for setup/install_ccache.py."""

from __future__ import annotations

from typing import TYPE_CHECKING

import pytest
from setup import install_ccache

if TYPE_CHECKING:
    from pathlib import Path


@pytest.fixture
def release(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> dict[str, object]:
    """Fake the verified download; the extracted binary reports *version*."""
    state: dict[str, object] = {"version": install_ccache.pinned_release("ccache").version}

    def fetch(url: str, sha256: str, member: str, staging: Path) -> Path:
        state["fetched"] = (url, sha256, member)
        binary = staging / "ccache"
        binary.write_text(f"#!/bin/sh\necho 'ccache version {state['version']}'\n")
        return binary

    monkeypatch.setattr(install_ccache, "fetch_release_member", fetch)
    monkeypatch.setattr(install_ccache, "host_arch", lambda: "aarch64")
    return state


def test_installs_the_pinned_release_for_the_host_architecture(tmp_path: Path, release) -> None:
    pin = install_ccache.pinned_release("ccache")
    stem = f"ccache-{pin.version}-linux-aarch64-glibc"

    assert install_ccache.install(tmp_path) == tmp_path / "bin/ccache"

    assert release["fetched"] == (
        f"{pin.url}/{stem}.tar.xz",
        pin.sha256["linux-aarch64-glibc"],
        f"{stem}/ccache",
    )
    assert (tmp_path / "bin/ccache").stat().st_mode & 0o111


def test_a_binary_reporting_another_version_fails(tmp_path: Path, release, capsys) -> None:
    release["version"] = "0.0.1"
    assert install_ccache.main(["--prefix", str(tmp_path)]) == 1
    assert "Installing ccache failed" in capsys.readouterr().err
