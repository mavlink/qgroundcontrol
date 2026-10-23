"""Changed-region clang-format checks through analyze.py and the pre-commit hook."""

from __future__ import annotations

import os
import shutil
from unittest.mock import patch

import analyze
import pytest
from analyzers import clang_format

from ._helpers import commit_files, completed, git

LEGACY = "#include <vector>\n\nint  legacy( ){return 1;}\n"
LEGACY_PATH = "src/with spaces.cc"


@pytest.fixture(autouse=True)
def _no_pre_commit_refs(monkeypatch) -> None:
    for key in list(os.environ):
        if key.startswith("PRE_COMMIT_"):
            monkeypatch.delenv(key)


@pytest.fixture
def format_repo(git_repo, monkeypatch):
    if shutil.which("clang-format") is None:
        pytest.skip("clang-format required")
    commit_files(git_repo, {".clang-format": "BasedOnStyle: LLVM\n", LEGACY_PATH: LEGACY}, "base")
    monkeypatch.chdir(git_repo)
    monkeypatch.setattr(analyze, "find_repo_root", lambda: git_repo)
    monkeypatch.setattr("analyzers.clang_format.probe_version", lambda _: (1, 0))
    monkeypatch.setattr("analyzers.clang_format.uv_lock_version", lambda _: "1.0")
    return git_repo


def run_analyze(monkeypatch, *args: str, base: str = "master") -> int:
    monkeypatch.setenv("PR_BASE_SHA", base)
    return analyze.main(["--tool", "clang-format", *args])


def run_hook(monkeypatch, *args: str, base: str | None = None) -> int:
    if base:
        monkeypatch.setenv("PRE_COMMIT_FROM_REF", base)
        monkeypatch.setenv("PRE_COMMIT_TO_REF", "HEAD")
    return clang_format.main(list(args))


@pytest.fixture(params=["analyze", "hook"])
def check(request, format_repo, monkeypatch):
    """Run one entry point over the changed files; the hook receives them like pre-commit."""

    def run(*files: str, fix: bool = False, base: str | None = None) -> int:
        flags = ["--fix"] if fix else []
        if request.param == "analyze":
            return run_analyze(monkeypatch, *flags, base=base or "master")
        return run_hook(monkeypatch, *flags, *files, base=base)

    run.entry_point = request.param  # type: ignore[attr-defined]
    return run


def test_unrelated_legacy_formatting_is_ignored(format_repo, check) -> None:
    source = format_repo / LEGACY_PATH
    changed = "#include <string>\n" + LEGACY
    source.write_text(changed)
    assert check(LEGACY_PATH) == 0
    assert check(LEGACY_PATH, fix=True) == 0
    assert source.read_text() == changed


def test_fix_formats_only_changed_lines(format_repo, check) -> None:
    source = format_repo / LEGACY_PATH
    source.write_text(LEGACY + "\nint  added( ){return 2;}\n")
    assert check(LEGACY_PATH) == 1
    assert check(LEGACY_PATH, fix=True) == 0
    assert source.read_text().startswith(LEGACY)
    assert "int  added" not in source.read_text()
    assert check(LEGACY_PATH) == 0


def test_committed_changes_since_merge_base_are_checked(format_repo, check) -> None:
    git(format_repo, "checkout", "-q", "-b", "feature")
    source = format_repo / LEGACY_PATH
    source.write_text("#include <string>\n" + LEGACY)
    git(format_repo, "commit", "-q", "-am", "Add include")
    assert check(LEGACY_PATH, base="master") == 0
    source.write_text(source.read_text() + "\nint  added( ){return 2;}\n")
    git(format_repo, "commit", "-q", "-am", "Add badly formatted function")
    assert check(LEGACY_PATH, base="master") == 1


@pytest.mark.parametrize("staged", [False, True])
def test_new_file_is_checked_completely(format_repo, check, staged) -> None:
    source = format_repo / "src" / "new.cc"
    source.write_text("int  added( ){return 2;}\n")
    if staged:
        git(format_repo, "add", "src/new.cc")
    assert check("src/new.cc") == 1
    assert check("src/new.cc", fix=True) == 0
    assert source.read_text() == "int added() { return 2; }\n"
    assert check("src/new.cc") == 0


def test_coding_style_example_is_checked_completely(format_repo, check) -> None:
    example = "tools/coding-style/Example.cc"
    source = format_repo / example
    commit_files(format_repo, {example: LEGACY}, "Add example")
    source.write_text("#include <string>\n" + LEGACY)
    assert check(example) == 1
    assert check(example, fix=True) == 0
    assert "int legacy() { return 1; }" in source.read_text()


def test_deletion_only_change_is_skipped(format_repo, check) -> None:
    source = format_repo / LEGACY_PATH
    source.write_text(LEGACY.removeprefix("#include <vector>\n"))
    assert check(LEGACY_PATH) == 0


def test_invalid_base_fails_closed(format_repo, check, capfd) -> None:
    (format_repo / LEGACY_PATH).write_text("#include <string>\n" + LEGACY)
    if check.entry_point == "analyze":
        # Without a usable base analyze.py checks whole files, including the legacy formatting.
        assert check(base="missing-ref") == 1
        assert "checking everything" in capfd.readouterr().err
    else:
        assert check(LEGACY_PATH, base="missing-ref") == 1
        assert "clang-format check failed" in capfd.readouterr().err


def test_analyze_all_and_explicit_paths_check_whole_files(format_repo, monkeypatch) -> None:
    assert run_analyze(monkeypatch) == 0
    assert run_analyze(monkeypatch, "--all") == 1
    assert run_analyze(monkeypatch, LEGACY_PATH) == 1


def test_hook_checks_explicit_unchanged_file_completely(format_repo, monkeypatch) -> None:
    assert run_hook(monkeypatch, LEGACY_PATH) == 1


def test_hook_checks_whole_file_before_first_commit(git_repo, monkeypatch) -> None:
    if shutil.which("clang-format") is None:
        pytest.skip("clang-format required")
    (git_repo / "first.cc").write_text("int  added( ){return 2;}\n")
    monkeypatch.chdir(git_repo)
    assert run_hook(monkeypatch, "first.cc") == 1


def test_fix_execution_failure_is_not_success(tmp_path) -> None:
    failure = completed(returncode=1, stderr="permission denied")
    with (
        patch("analyzers.clang_format.run_captured", return_value=failure),
        patch("analyzers.clang_format.run_git", return_value=failure),
    ):
        result = clang_format.ClangFormatAnalyzer(tmp_path, tmp_path)._run_fix(
            {tmp_path / "file.cc": []}
        )
    assert not result.passed
    assert result.execution_error
