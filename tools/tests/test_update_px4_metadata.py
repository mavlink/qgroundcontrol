"""Tests for tools/update_px4_metadata.py."""

from __future__ import annotations

import json
import lzma
from typing import TYPE_CHECKING

import pytest
import update_px4_metadata as mod

if TYPE_CHECKING:
    from pathlib import Path

AIRFRAMES = (
    b'<airframes>\n  <version>1</version>\n  <airframe_group name="Quadrotor x"/>\n</airframes>\n'
)
PARAMETERS = '{"parameters": [{"name": "MC_ROLL_P", "type": "Float"}], "version": 1}'


@pytest.fixture
def repo(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> Path:
    for path in (mod.AIRFRAMES_FILE, mod.PARAMETERS_FILE):
        (tmp_path / path).parent.mkdir(parents=True)
        (tmp_path / path).write_text("bundled\n", encoding="utf-8")
    monkeypatch.setattr(mod, "REPO_ROOT", tmp_path)
    return tmp_path


def _serve(monkeypatch: pytest.MonkeyPatch, responses: dict[str, bytes]) -> list[tuple[str, int]]:
    calls: list[tuple[str, int]] = []

    def fake_fetch(url: str, *, timeout: float, attempts: int) -> bytes:
        calls.append((url, attempts))
        return responses[url]

    monkeypatch.setattr(mod, "fetch_bytes", fake_fetch)
    return calls


def test_writes_validated_metadata_unchanged(repo: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    calls = _serve(
        monkeypatch,
        {
            mod.AIRFRAMES_URL: AIRFRAMES,
            mod.PARAMETERS_URL: lzma.compress(PARAMETERS.encode(), format=lzma.FORMAT_XZ),
        },
    )

    assert mod.main([]) == 0

    assert calls == [
        (mod.AIRFRAMES_URL, mod.FETCH_ATTEMPTS),
        (mod.PARAMETERS_URL, mod.FETCH_ATTEMPTS),
    ]
    assert (repo / mod.AIRFRAMES_FILE).read_bytes() == AIRFRAMES
    assert (repo / mod.PARAMETERS_FILE).read_text(encoding="utf-8") == PARAMETERS


@pytest.mark.parametrize(
    ("airframes", "parameters", "message"),
    [
        (b"<html><body>502 Bad Gateway</body></html>", PARAMETERS, "root is ['html']"),
        (AIRFRAMES[:40], PARAMETERS, "not well-formed"),
        (
            b'<!DOCTYPE airframes [<!ENTITY x "y">]><airframes>&x;</airframes>',
            PARAMETERS,
            "must not declare a DTD",
        ),
        (AIRFRAMES, json.dumps({"parameters": []}), "no parameters list"),
        (AIRFRAMES, "[]", "no parameters list"),
        (AIRFRAMES, None, "not xz-compressed JSON"),
    ],
)
def test_invalid_downloads_leave_the_bundled_files_alone(
    repo: Path,
    monkeypatch: pytest.MonkeyPatch,
    capsys: pytest.CaptureFixture[str],
    airframes: bytes,
    parameters: str | None,
    message: str,
) -> None:
    compressed = (
        lzma.compress(parameters.encode(), format=lzma.FORMAT_XZ)
        if parameters is not None
        else b"<html>not found</html>"
    )
    _serve(monkeypatch, {mod.AIRFRAMES_URL: airframes, mod.PARAMETERS_URL: compressed})

    assert mod.main([]) == 1

    (annotation,) = [line for line in capsys.readouterr().out.splitlines() if "::error::" in line]
    assert annotation.startswith("::error::PX4 metadata not updated: ")
    assert message in annotation
    for path in (mod.AIRFRAMES_FILE, mod.PARAMETERS_FILE):
        assert (repo / path).read_text(encoding="utf-8") == "bundled\n"


def test_truncated_xz_stream_is_rejected() -> None:
    compressed = lzma.compress(PARAMETERS.encode(), format=lzma.FORMAT_XZ)
    with pytest.raises(ValueError, match="not xz-compressed JSON"):
        mod.validate_parameters(compressed[:-8])


def test_fetch_failures_fail_without_writing(repo: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    def failing_fetch(url: str, *, timeout: float, attempts: int) -> bytes:
        raise RuntimeError(f"Failed to fetch {url} after {attempts} attempts")

    monkeypatch.setattr(mod, "fetch_bytes", failing_fetch)

    assert mod.main([]) == 1
    assert (repo / mod.AIRFRAMES_FILE).read_text(encoding="utf-8") == "bundled\n"
