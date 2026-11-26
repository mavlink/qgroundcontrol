"""Tests for tools/update_gstreamer.py."""

from __future__ import annotations

import copy
import hashlib
import json
from email.message import Message
from typing import TYPE_CHECKING
from urllib.error import HTTPError

import pytest
import update_gstreamer
from common.gstreamer import package_url, platform_packages
from validate_configs import validate_build_config

from ._helpers import REPO_ROOT

if TYPE_CHECKING:
    from pathlib import Path

VERSION = "1.30.1"
PLATFORMS = platform_packages()
PACKAGES = [package for packages in PLATFORMS.values() for package in packages]


def _sidecar_url(package: str) -> str:
    return f"{package_url(package, VERSION)}.sha256sum"


def _digest(package: str) -> str:
    return hashlib.sha256(_sidecar_url(package).encode()).hexdigest()


@pytest.fixture
def config(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> Path:
    """A copy of the repository's build-config.json for the script to edit."""
    path = tmp_path / "build-config.json"
    path.write_bytes((REPO_ROOT / ".github/build-config.json").read_bytes())
    monkeypatch.setenv("CONFIG_FILE", str(path))
    return path


def _serve(monkeypatch: pytest.MonkeyPatch, overrides: dict[str, str | None]) -> None:
    """Answer each package's ``.sha256sum`` request; an override replaces the text, None is a 404."""
    sidecars: dict[str, str | None] = {
        _sidecar_url(package): f"{_digest(package)}  package\n" for package in PACKAGES
    }
    sidecars.update({_sidecar_url(package): text for package, text in overrides.items()})

    def read(url: str, **_kwargs: object) -> str:
        text = sidecars.get(url)
        if text is None:
            raise HTTPError(url, 404, "Not Found", Message(), None)
        return text

    monkeypatch.setattr("common.gstreamer.read_url_text", read)


def test_pins_the_release_for_every_platform(config: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    expected = json.loads(config.read_text(encoding="utf-8"))
    expected["gstreamer"]["version"].update(dict.fromkeys(PLATFORMS, VERSION))
    expected["gstreamer"]["checksums"] = {
        VERSION: {package: _digest(package) for package in PACKAGES}
    }
    _serve(monkeypatch, {})

    assert update_gstreamer.main([VERSION]) == 0

    # The whole file, in the layout the pretty-format-json hook keeps: only the pins changed.
    text = config.read_text(encoding="utf-8")
    assert text == json.dumps(expected, indent=2, ensure_ascii=False) + "\n"
    schema = json.loads((REPO_ROOT / ".github/build-config.schema.json").read_text())
    assert validate_build_config(copy.deepcopy(expected), schema) == []


@pytest.mark.parametrize("sidecar", [None, "<html>503 Service Unavailable</html>\n"])
def test_a_release_without_every_package_is_not_pinned(
    config: Path,
    monkeypatch: pytest.MonkeyPatch,
    capsys: pytest.CaptureFixture[str],
    sidecar: str | None,
) -> None:
    before = config.read_bytes()
    _serve(monkeypatch, {"windows_msvc_arm64": sidecar})

    assert update_gstreamer.main([VERSION]) == 1

    assert config.read_bytes() == before
    error = capsys.readouterr().err
    assert f"No checksum for the windows_msvc_arm64 package of GStreamer {VERSION}" in error


@pytest.mark.parametrize("version", ["1.30", "v1.30.1", "1.30.1-rc1"])
def test_only_release_versions_are_accepted(config: Path, version: str) -> None:
    before = config.read_bytes()
    with pytest.raises(SystemExit):
        update_gstreamer.main([version])
    assert config.read_bytes() == before
