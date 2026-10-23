"""Analysis executable selection for analysis.yml."""

from __future__ import annotations

from typing import TYPE_CHECKING

import pytest
from ci.runner import analysis_tools

if TYPE_CHECKING:
    from pathlib import Path

    from pytest_subprocess import FakeProcess

SHA256 = "a" * 64


@pytest.fixture
def ci_env(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> dict[str, Path]:
    paths = {
        "RUNNER_TEMP": tmp_path / "temp",
        "GITHUB_WORKSPACE": tmp_path / "workspace",
        "GITHUB_PATH": tmp_path / "github_path",
    }
    paths["RUNNER_TEMP"].mkdir()
    for name, path in paths.items():
        monkeypatch.setenv(name, str(path))
    monkeypatch.setenv("LLVM_VERSION", "21")
    monkeypatch.setenv("CLANG_TIDY_VERSION", "22.1.0")
    monkeypatch.setenv("CLANG_TIDY_SHA256", SHA256)
    return paths


def _commands(ci_env: dict[str, Path]) -> dict[str, list[str]]:
    temp, workspace = ci_env["RUNNER_TEMP"], ci_env["GITHUB_WORKSPACE"]
    clang_tidy = str(temp / "clang-tidy/clang_tidy/data/bin/clang-tidy")
    return {
        "uv": [
            "uv",
            "pip",
            "install",
            "--quiet",
            "--require-hashes",
            "--no-deps",
            "--only-binary",
            ":all:",
            "--target",
            str(temp / "clang-tidy"),
            "-r",
            str(temp / "clang-tidy-requirements.txt"),
        ],
        "tidy_version": [clang_tidy, "--version"],
        "verify_config": [
            clang_tidy,
            f"--config-file={workspace / '.clang-tidy'}",
            "--verify-config",
        ],
        "clazy_version": [str(workspace / ".cache/clazy/bin/clazy-standalone"), "--version"],
    }


def test_pinned_clang_tidy_goes_ahead_of_llvm_and_clazy_after_it(
    ci_env: dict[str, Path], fp: FakeProcess, monkeypatch: pytest.MonkeyPatch
) -> None:
    monkeypatch.setenv("ANALYSIS_TOOLS", "clang-tidy clazy")
    commands = _commands(ci_env)
    for command in commands.values():
        fp.register(command)

    assert analysis_tools.main([]) == 0

    assert list(fp.calls) == list(commands.values())
    requirements = ci_env["RUNNER_TEMP"] / "clang-tidy-requirements.txt"
    assert (
        requirements.read_text(encoding="utf-8") == f"clang-tidy==22.1.0 --hash=sha256:{SHA256}\n"
    )
    # Later $GITHUB_PATH lines take precedence, so clang-tidy's wheel shadows LLVM's own.
    assert ci_env["GITHUB_PATH"].read_text(encoding="utf-8").splitlines() == [
        "/usr/lib/llvm-21/bin",
        str(ci_env["RUNNER_TEMP"] / "clang-tidy/clang_tidy/data/bin"),
        str(ci_env["GITHUB_WORKSPACE"] / ".cache/clazy/bin"),
    ]


def test_clazy_alone_needs_no_clang_tidy_pin(
    ci_env: dict[str, Path], fp: FakeProcess, monkeypatch: pytest.MonkeyPatch
) -> None:
    monkeypatch.setenv("ANALYSIS_TOOLS", "clazy")
    monkeypatch.delenv("CLANG_TIDY_VERSION")
    monkeypatch.delenv("CLANG_TIDY_SHA256")
    fp.register(_commands(ci_env)["clazy_version"])

    assert analysis_tools.main([]) == 0

    assert ci_env["GITHUB_PATH"].read_text(encoding="utf-8").splitlines() == [
        "/usr/lib/llvm-21/bin",
        str(ci_env["GITHUB_WORKSPACE"] / ".cache/clazy/bin"),
    ]


def test_invalid_clang_tidy_config_fails_before_clang_tidy_reaches_path(
    ci_env: dict[str, Path],
    fp: FakeProcess,
    monkeypatch: pytest.MonkeyPatch,
    capsys: pytest.CaptureFixture[str],
) -> None:
    monkeypatch.setenv("ANALYSIS_TOOLS", "clang-tidy")
    commands = _commands(ci_env)
    fp.register(commands["uv"])
    fp.register(commands["tidy_version"])
    fp.register(commands["verify_config"], returncode=1)

    assert analysis_tools.main([]) == 1

    assert "::error::Selecting analysis executables failed" in capsys.readouterr().out
    assert ci_env["GITHUB_PATH"].read_text(encoding="utf-8").splitlines() == [
        "/usr/lib/llvm-21/bin"
    ]


@pytest.mark.parametrize("missing", ["LLVM_VERSION", "CLANG_TIDY_VERSION", "CLANG_TIDY_SHA256"])
def test_missing_toolchain_inputs_fail_before_installing(
    ci_env: dict[str, Path],
    fp: FakeProcess,
    monkeypatch: pytest.MonkeyPatch,
    capsys: pytest.CaptureFixture[str],
    missing: str,
) -> None:
    monkeypatch.setenv("ANALYSIS_TOOLS", "clang-tidy clazy")
    monkeypatch.setenv(missing, "")

    assert analysis_tools.main([]) == 1

    assert f"::error::Missing analysis toolchain inputs: {missing}" in capsys.readouterr().out
    assert not list(fp.calls)
    assert not ci_env["GITHUB_PATH"].exists()
