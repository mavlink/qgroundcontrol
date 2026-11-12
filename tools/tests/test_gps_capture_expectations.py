"""Tests for test/GPS/Core/Protocols/fixtures/capture_expectations.py: where each capture comes from."""

from __future__ import annotations

import sys
import types
from typing import TYPE_CHECKING

import pytest

from ._helpers import commit_files, git, load_script

if TYPE_CHECKING:
    from pathlib import Path
    from types import ModuleType

SCRIPT = "test/GPS/Core/Protocols/fixtures/capture_expectations.py"
# The decoders the script pins for its expectations, which the fixture oracle installs and these tests do not use.
DECODERS = {
    "pynmeagps": "NMEAReader",
    "pynmeagps.nmeahelpers": "dmm2ddd",
    "pyrtcm": "RTCMReader",
    "pyubx2": "UBXReader",
}
LOG = "0123456789abcdef"


@pytest.fixture
def script(monkeypatch: pytest.MonkeyPatch) -> ModuleType:
    for name, attribute in DECODERS.items():
        module = types.ModuleType(name)
        setattr(module, attribute, None)
        monkeypatch.setitem(sys.modules, name, module)
    return load_script(SCRIPT)


@pytest.fixture
def upstream(tmp_path: Path, isolated_git: None) -> tuple[Path, dict[str, str]]:
    """A directory with the checkout of one upstream project, and the commit it is at."""
    project = tmp_path / "upstream" / "gpsd"
    project.mkdir(parents=True)
    git(project, "init", "-q")
    return project.parent, {"gpsd": commit_files(project, {"test/receiver.log": LOG}, "log")}


@pytest.fixture
def fixtures(tmp_path: Path, script: ModuleType, monkeypatch: pytest.MonkeyPatch) -> Path:
    """A fixture directory with a slice of the upstream log and a capture that a user sent."""
    directory = tmp_path / "fixtures"
    directory.mkdir()
    monkeypatch.setattr(
        script,
        "CAPTURES",
        {
            "gpsd-slice.cap": (directory, "gpsd", "test/receiver.log", 4, 6),
            "user-receiver.cap": (directory,),
        },
    )
    (directory / "gpsd-slice.cap").write_text(LOG[4:10])
    (directory / "user-receiver.cap").write_text("what the user's receiver sent")
    return directory


def test_capture_without_upstream_source_passes_the_check(
    script: ModuleType, upstream: tuple[Path, dict[str, str]], fixtures: Path
) -> None:
    script.check_sources(upstream[0], check=True, commits=upstream[1])


def test_rewriting_keeps_capture_without_upstream_source(
    script: ModuleType, upstream: tuple[Path, dict[str, str]], fixtures: Path
) -> None:
    (fixtures / "gpsd-slice.cap").write_text("stale")

    script.check_sources(upstream[0], check=False, commits=upstream[1])

    assert (fixtures / "gpsd-slice.cap").read_text() == LOG[4:10]
    assert (fixtures / "user-receiver.cap").read_text() == "what the user's receiver sent"


def test_slice_that_differs_from_its_upstream_file_fails_the_check(
    script: ModuleType, upstream: tuple[Path, dict[str, str]], fixtures: Path
) -> None:
    (fixtures / "gpsd-slice.cap").write_text("456789"[::-1])

    with pytest.raises(SystemExit, match=r"gpsd-slice\.cap differs from bytes 4 to 10 of gpsd"):
        script.check_sources(upstream[0], check=True, commits=upstream[1])


@pytest.mark.parametrize("check", [True, False])
def test_missing_capture_without_upstream_source_is_reported(
    script: ModuleType, upstream: tuple[Path, dict[str, str]], fixtures: Path, check: bool
) -> None:
    (fixtures / "user-receiver.cap").unlink()

    with pytest.raises(SystemExit, match=r"user-receiver\.cap is missing.*no upstream"):
        script.check_sources(upstream[0], check=check, commits=upstream[1])


def test_every_capture_of_the_table_is_a_slice_or_names_only_its_directory(
    script: ModuleType,
) -> None:
    for name, (directory, *source) in script.CAPTURES.items():
        assert (directory / name).is_file(), name
        assert len(source) in (0, 4), name
