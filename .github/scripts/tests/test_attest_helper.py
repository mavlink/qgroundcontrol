"""Release-attestation gating and checksum contracts."""

from __future__ import annotations

import sys
from hashlib import sha256
from typing import TYPE_CHECKING

import attest_helper
import pytest

if TYPE_CHECKING:
    from pathlib import Path


def _run_main(
    monkeypatch: pytest.MonkeyPatch, args: list[str], *, event_name: str = "push"
) -> dict[str, str]:
    outputs: dict[str, str] = {}
    monkeypatch.setenv("GITHUB_EVENT_NAME", event_name)
    monkeypatch.setattr(attest_helper, "write_github_output", outputs.update)
    monkeypatch.setattr(sys, "argv", ["attest_helper.py", *args])
    attest_helper.main()
    return outputs


def test_check_skips_prs_and_resolves_sbom_paths(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    subject = tmp_path / "sub" / "artifact.zip"
    subject.parent.mkdir()
    subject.touch()
    base_args = [
        "check",
        "--subject-path",
        str(subject),
        "--subject-name",
        "my-artifact",
        "--runner-temp",
        str(tmp_path),
    ]

    assert _run_main(monkeypatch, base_args, event_name="pull_request") == {"skip": "true"}
    for sbom_format, suffix in (("spdx-json", "spdx.json"), ("cyclonedx-json", "cdx.json")):
        outputs = _run_main(monkeypatch, [*base_args, "--sbom-format", sbom_format])
        assert outputs == {
            "skip": "false",
            "scan-path": str(subject.parent),
            "sbom-path": str(tmp_path / f"my-artifact.sbom.{suffix}"),
        }


def test_resolve_path_prefers_override_and_rejects_missing(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    default = tmp_path / "default.zip"
    override = tmp_path / "override.zip"
    default.touch()
    override.touch()

    assert _run_main(monkeypatch, ["resolve-path", "--default", str(default)]) == {
        "path": str(default)
    }
    assert _run_main(
        monkeypatch,
        ["resolve-path", "--override", str(override), "--default", str(default)],
    ) == {"path": str(override)}
    with pytest.raises(SystemExit) as error:
        _run_main(monkeypatch, ["resolve-path", "--default", str(tmp_path / "missing")])
    assert error.value.code == 1


def test_checksum_creates_and_normalizes_valid_sidecars(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    artifact = tmp_path / "QGroundControl-installer-AMD64-ARM64.exe"
    artifact.write_bytes(b"release artifact")
    digest = sha256(artifact.read_bytes()).hexdigest()
    sidecar = tmp_path / f"{artifact.name}.sha256"

    outputs = _run_main(monkeypatch, ["checksum", "--source-path", str(artifact)])
    assert outputs == {"path": str(sidecar)}
    assert sidecar.read_text(encoding="utf-8") == f"{digest}  {artifact.name}\n"

    for line_ending in ("\n", "\r\n"):
        sidecar.write_bytes(
            f"{digest}  QGroundControl-installer-AMD64-arm64.exe".encode() + line_ending.encode()
        )
        _run_main(monkeypatch, ["checksum", "--source-path", str(artifact)])
        assert sidecar.read_text(encoding="utf-8") == f"{digest}  {artifact.name}\n"


def test_checksum_rejects_mismatch(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    artifact = tmp_path / "artifact.zip"
    artifact.write_bytes(b"release artifact")
    (tmp_path / "artifact.zip.sha256").write_text(f"{'0' * 64}  artifact.zip\n")

    with pytest.raises(SystemExit) as error:
        _run_main(monkeypatch, ["checksum", "--source-path", str(artifact)])
    assert error.value.code == 1
