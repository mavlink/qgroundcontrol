"""Exercise the formatting hook against real Git diffs and clang-format."""

from __future__ import annotations

import os
import shutil
import subprocess
import sys
from pathlib import Path

import pytest

SCRIPT = Path(__file__).resolve().parents[1] / "check_clang_format.py"
LEGACY = "#include <vector>\n\nint  legacy( ){return 1;}\n"

pytestmark = pytest.mark.skipif(
    shutil.which("clang-format") is None, reason="clang-format required"
)


def git(repo, *args):
    return subprocess.run(
        ["git", *args],
        cwd=repo,
        check=True,
        capture_output=True,
        text=True,
    ).stdout.strip()


@pytest.fixture
def repo(tmp_path):
    git(tmp_path, "init")
    git(tmp_path, "config", "user.name", "Test")
    git(tmp_path, "config", "user.email", "test@example.invalid")
    (tmp_path / ".clang-format").write_text("BasedOnStyle: LLVM\n")
    (tmp_path / "with spaces.cpp").write_text(LEGACY)
    git(tmp_path, "add", ".")
    git(tmp_path, "commit", "-m", "Initial fixture")
    return tmp_path


def run_hook(repo, *args, base=None):
    env = {key: value for key, value in os.environ.items() if not key.startswith("PRE_COMMIT_")}
    if base:
        env.update(PRE_COMMIT_FROM_REF=base, PRE_COMMIT_TO_REF="HEAD")
    return subprocess.run(
        [sys.executable, str(SCRIPT), *args],
        cwd=repo,
        env=env,
        capture_output=True,
        text=True,
        check=False,
    )


def test_include_fix_preserves_unrelated_legacy_formatting(repo):
    source = repo / "with spaces.cpp"
    changed = "#include <string>\n" + LEGACY
    source.write_text(changed)
    result = run_hook(repo, source.name)
    assert result.returncode == 0, result.stderr
    assert source.read_text() == changed


def test_bad_changed_code_fails_and_fix_preserves_legacy_code(repo):
    source = repo / "with spaces.cpp"
    source.write_text(LEGACY + "\nint  added( ){return 2;}\n")
    assert run_hook(repo, source.name).returncode == 1
    result = run_hook(repo, "--fix", source.name)
    assert result.returncode == 0, result.stderr
    assert source.read_text().startswith(LEGACY)
    assert run_hook(repo, source.name).returncode == 0


@pytest.mark.parametrize("staged", [False, True])
def test_new_files_are_checked_completely(repo, staged):
    source = repo / "new.cpp"
    source.write_text("int  added( ){return 2;}\n")
    if staged:
        git(repo, "add", source.name)
    assert run_hook(repo, source.name).returncode == 1
    assert run_hook(repo, "--fix", source.name).returncode == 0
    assert run_hook(repo, source.name).returncode == 0


def test_unchanged_file_is_checked_completely(repo):
    assert run_hook(repo, "with spaces.cpp").returncode == 1


def test_deletion_only_does_not_reformat_legacy_code(repo):
    source = repo / "with spaces.cpp"
    source.write_text(LEGACY.removeprefix("#include <vector>\n"))
    assert run_hook(repo, source.name).returncode == 0


def test_committed_pr_changes_use_merge_base(repo):
    base = git(repo, "rev-parse", "HEAD")
    source = repo / "with spaces.cpp"
    source.write_text("#include <string>\n" + LEGACY)
    git(repo, "add", source.name)
    git(repo, "commit", "-m", "Add include")
    result = run_hook(repo, source.name, base=base)
    assert result.returncode == 0, result.stderr
    source.write_text(source.read_text() + "\nint  added( ){return 2;}\n")
    git(repo, "add", source.name)
    git(repo, "commit", "-m", "Add badly formatted function")
    assert run_hook(repo, source.name, base=base).returncode == 1


def test_invalid_comparison_fails_closed(repo):
    result = run_hook(repo, "with spaces.cpp", base="missing-ref")
    assert result.returncode == 1
    assert "clang-format check failed" in result.stderr


def test_first_commit_checks_whole_file(tmp_path):
    git(tmp_path, "init")
    source = tmp_path / "first.cpp"
    source.write_text("int  added( ){return 2;}\n")
    assert run_hook(tmp_path, source.name).returncode == 1
