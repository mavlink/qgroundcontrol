"""Tests for validate_runner_configs.py."""

from __future__ import annotations

import subprocess
from pathlib import Path

import pytest
from ci.runner import validate_runner_configs as vrc

PIN = {
    "repository": "owner/tool",
    "version": "1.2.3",
    "sha256": {"linux-x86_64": "a" * 64, "darwin-aarch64": "b" * 64},
}


@pytest.fixture(autouse=True)
def repo_root(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> Path:
    monkeypatch.setattr(vrc, "REPO_ROOT", tmp_path)
    return tmp_path


@pytest.mark.parametrize(
    ("linux", "macos", "arch", "expected"),
    [(True, False, "x86_64", "linux-x86_64"), (False, True, "aarch64", "darwin-aarch64")],
)
def test_platform_key(monkeypatch, linux, macos, arch, expected) -> None:
    monkeypatch.setattr(vrc, "is_linux", lambda: linux)
    monkeypatch.setattr(vrc, "is_macos", lambda: macos)
    monkeypatch.setattr(vrc, "host_arch", lambda: arch)
    assert vrc.platform_key() == expected


def test_platform_key_rejects_other_systems(monkeypatch) -> None:
    monkeypatch.setattr(vrc, "is_linux", lambda: False)
    monkeypatch.setattr(vrc, "is_macos", lambda: False)
    with pytest.raises(SystemExit, match="Linux and macOS"):
        vrc.platform_key()


def test_ensure_packer_downloads_the_pinned_zip_once(monkeypatch, repo_root: Path) -> None:
    calls: list[tuple[str, str, str]] = []

    def fake_fetch(url: str, sha256: str, member: str, staging: Path) -> Path:
        calls.append((url, sha256, member))
        source = staging / member
        source.write_text("packer")
        return source

    monkeypatch.setattr(vrc, "fetch_release_member", fake_fetch)
    binary = vrc.ensure_packer(PIN, "linux-x86_64")

    assert binary == repo_root / ".cache/tools/packer/1.2.3/linux-x86_64/packer"
    assert binary.read_text() == "packer"
    assert binary.stat().st_mode & 0o111
    assert calls == [
        (
            "https://releases.hashicorp.com/packer/1.2.3/packer_1.2.3_linux_amd64.zip",
            "a" * 64,
            "packer",
        )
    ]
    # Cached: no second download.
    assert vrc.ensure_packer(PIN, "linux-x86_64") == binary
    assert len(calls) == 1


def test_ensure_roc_downloads_the_pinned_binary(monkeypatch, repo_root: Path) -> None:
    calls: list[tuple[str, str | None]] = []

    def fake_download(url: str, dest: Path, *, expected_sha256: str | None = None) -> None:
        calls.append((url, expected_sha256))
        dest.write_text("roc")

    monkeypatch.setattr(vrc, "download_file", fake_download)
    binary = vrc.ensure_roc(PIN, "darwin-aarch64")

    assert binary == repo_root / ".cache/tools/roc/1.2.3/darwin-aarch64/roc"
    assert binary.read_text() == "roc"
    assert binary.stat().st_mode & 0o111
    assert calls == [
        ("https://github.com/owner/tool/releases/download/v1.2.3/roc_v1.2.3_darwin_arm64", "b" * 64)
    ]
    assert not list(binary.parent.glob(".*.partial"))


def _record_runs(monkeypatch, returncodes: list[int]) -> list[dict]:
    runs: list[dict] = []

    def fake_run(command, *, cwd, check, env=None):
        runs.append({"command": command, "cwd": cwd, "env": env})
        return subprocess.CompletedProcess(command, returncodes[len(runs) - 1])

    monkeypatch.setattr(vrc.subprocess, "run", fake_run)
    return runs


def test_validate_packer_checks_format_inits_and_validates(monkeypatch) -> None:
    runs = _record_runs(monkeypatch, [0, 0, 0])
    assert vrc.validate_packer(Path("/bin/packer")) == 0
    assert [run["command"][1:] for run in runs] == [
        ["fmt", "-check", vrc.PACKER_TEMPLATE],
        ["init", vrc.PACKER_TEMPLATE],
        ["validate", vrc.PACKER_TEMPLATE],
    ]
    assert all(run["cwd"] == vrc.RUNNER_IMAGES for run in runs)
    assert runs[-1]["env"]["PKR_VAR_source_ref"] == "0" * 40


def test_validate_packer_stops_at_the_first_failure(monkeypatch) -> None:
    runs = _record_runs(monkeypatch, [3, 0, 0])
    assert vrc.validate_packer(Path("/bin/packer")) == 3
    assert len(runs) == 1


def test_lint_runs_on_lints_every_config_and_reports_the_first_failure(
    monkeypatch, repo_root
) -> None:
    runs = _record_runs(monkeypatch, [0, 2])
    assert vrc.lint_runs_on(Path("/bin/roc")) == 2
    assert [run["command"][1:] for run in runs] == [["lint", path] for path in vrc.RUNS_ON_CONFIGS]
    assert all(run["cwd"] == repo_root for run in runs)


@pytest.mark.parametrize(("check", "tool"), [("packer", "packer"), ("runs-on", "roc")])
def test_main_runs_the_selected_check(monkeypatch, check: str, tool: str) -> None:
    monkeypatch.setattr(vrc, "platform_key", lambda: "linux-x86_64")
    monkeypatch.setattr(vrc, "load_build_config", lambda: {"tools": {"packer": PIN, "roc": PIN}})
    monkeypatch.setattr(vrc, "ensure_packer", lambda pin, key: Path(f"/bin/packer-{key}"))
    monkeypatch.setattr(vrc, "ensure_roc", lambda pin, key: Path(f"/bin/roc-{key}"))
    ran: list[Path] = []
    monkeypatch.setattr(vrc, "validate_packer", lambda binary: ran.append(binary) or 0)
    monkeypatch.setattr(vrc, "lint_runs_on", lambda binary: ran.append(binary) or 0)

    assert vrc.main([check]) == 0
    assert ran == [Path(f"/bin/{tool}-linux-x86_64")]
