"""Tests for tools/pre_commit.py."""

from __future__ import annotations

import argparse
import fnmatch
import json
import os
import re
from typing import TYPE_CHECKING
from unittest.mock import patch

import pre_commit
import pytest
from pre_commit import (
    DOCS_FULL_SCAN_PATHSPECS,
    DOCS_HOOKS,
    build_precommit_args,
    changed_refs,
    extract_hook_lines,
    file_selection_args,
    parse_args,
    select_docs_files,
    summarize_output,
)

from ._helpers import REPO_ROOT, completed, get_steps, git, load_workflow, load_yaml

if TYPE_CHECKING:
    import subprocess
    from pathlib import Path

HOOK_OUTPUT = "lint........................Failed\nformat......................Passed\n"


def test_summarize_output_counts_states() -> None:
    passed, failed, skipped = summarize_output(
        "hook-a........................Passed\nhook-b........................Failed\nhook-c........................Skipped\n"
    )
    assert (passed, failed, skipped) == (1, 1, 1)


def test_extract_hook_lines_strips_ansi() -> None:
    lines = extract_hook_lines("\x1b[31mhook-a........................Failed\x1b[0m\n")
    assert lines == ["hook-a........................Failed"]


def test_build_precommit_args_all_files() -> None:
    args = parse_args([])
    with patch("pre_commit.get_default_branch_ref", return_value="master"):
        assert file_selection_args(args) == ["--all-files"]
    assert build_precommit_args(["--all-files"]) == [
        "prek",
        "run",
        "--show-diff-on-failure",
        "--color=always",
        "--all-files",
    ]


def test_build_precommit_args_places_hook_before_options() -> None:
    assert build_precommit_args(["--files", "a.md"], "vale") == [
        "prek",
        "run",
        "vale",
        "--show-diff-on-failure",
        "--color=always",
        "--files",
        "a.md",
    ]


def test_build_precommit_args_changed_mode() -> None:
    args = parse_args(["--changed"])
    with patch("pre_commit.get_default_branch_ref", return_value="master"):
        assert file_selection_args(args) == ["--from-ref", "master", "--to-ref", "HEAD"]


def test_build_precommit_args_changed_main_branch() -> None:
    args = parse_args(["--changed"])
    with patch("pre_commit.get_default_branch_ref", return_value="main"):
        assert file_selection_args(args) == ["--from-ref", "main", "--to-ref", "HEAD"]


def test_build_precommit_args_changed_no_ref() -> None:
    args = parse_args(["--changed"])
    with patch("pre_commit.get_default_branch_ref", return_value=None):
        assert file_selection_args(args) == ["--all-files"]


def test_new_branch_push_checks_all_files() -> None:
    with patch.dict(os.environ, {"PR_BASE_SHA": "0" * 40}):
        assert file_selection_args(parse_args(["--changed"])) == ["--all-files"]


def test_changed_files_use_pr_head_instead_of_merge_checkout() -> None:
    with patch.dict(os.environ, {"PR_BASE_SHA": "base-sha", "PR_HEAD_SHA": "head-sha"}):
        selection = file_selection_args(parse_args(["--changed"]))
    assert selection == ["--from-ref", "base-sha", "--to-ref", "head-sha"]


def test_summary_counts_colored_hook_results_and_ignores_diagnostics():
    output = (
        "format...........\x1b[41mFailed\x1b[0m\n"
        "python...........\x1b[42mPassed\x1b[0m\n"
        "qml..............(no files)\x1b[46mSkipped\x1b[0m\n"
        "Failed to load a file\n"
    )
    assert summarize_output(output) == (1, 1, 1)


def _write(repo: Path, path: str, content: str) -> None:
    target = repo / path
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(content, encoding="utf-8")


ENGLISH_PAGES = ["docs/en/guide/new page.md", "docs/en/index.md"]


@pytest.mark.parametrize(
    ("changed", "expected"),
    [
        (".typos.toml", ENGLISH_PAGES),
        (".vale.ini", ENGLISH_PAGES),
        (".markdownlint.yaml", ENGLISH_PAGES),
        (".markdownlintignore", ENGLISH_PAGES),
        (".pre-commit-config.yaml", ENGLISH_PAGES),
        (".github/workflows/docs-lint.yml", ENGLISH_PAGES),
        ("tools/pre_commit.py", ENGLISH_PAGES),
        ("docs/en/guide/new page.md", ["docs/en/guide/new page.md"]),
        ("docs/en/index.md", ["docs/en/index.md"]),
        ("docs/en/image.svg", []),
        ("docs/fr/index.md", []),
        (None, ENGLISH_PAGES),
    ],
)
def test_docs_selection_rechecks_every_page_after_config_changes(
    git_repo: Path, monkeypatch: pytest.MonkeyPatch, changed: str | None, expected: list[str]
) -> None:
    for path in [*ENGLISH_PAGES, "docs/fr/index.md"]:
        _write(git_repo, path, "# Original\n")
    git(git_repo, "add", ".")
    git(git_repo, "commit", "-qm", "Base")
    base = git(git_repo, "rev-parse", "HEAD")
    if changed:
        _write(git_repo, changed, "# Changed\n")
        git(git_repo, "add", ".")
        git(git_repo, "commit", "-qm", "Change")

    monkeypatch.chdir(git_repo)
    assert select_docs_files((base, "HEAD") if changed else None) == expected


def test_docs_selection_ignores_deleted_pages(
    git_repo: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    for path in ENGLISH_PAGES:
        _write(git_repo, path, "# Original\n")
    git(git_repo, "add", ".")
    git(git_repo, "commit", "-qm", "Base")
    base = git(git_repo, "rev-parse", "HEAD")
    git(git_repo, "rm", "-q", "docs/en/index.md")
    git(git_repo, "commit", "-qm", "Delete")

    monkeypatch.chdir(git_repo)
    assert select_docs_files((base, "HEAD")) == []


def test_changed_refs_diff_a_pr_merge_checkout_against_its_base_tip(
    git_repo: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    _write(git_repo, "a.txt", "base\n")
    git(git_repo, "add", ".")
    git(git_repo, "commit", "-qm", "Fork point")
    fork_point = git(git_repo, "rev-parse", "HEAD")
    git(git_repo, "checkout", "-qb", "pr")
    _write(git_repo, "pr.txt", "pr\n")
    git(git_repo, "add", ".")
    git(git_repo, "commit", "-qm", "PR change")
    pr_head = git(git_repo, "rev-parse", "HEAD")
    git(git_repo, "checkout", "-q", "master")
    _write(git_repo, "a.txt", "newer base\n")
    git(git_repo, "commit", "-qam", "Newer base")
    base_tip = git(git_repo, "rev-parse", "HEAD")
    git(git_repo, "merge", "-q", "--no-ff", "-m", "Merge PR", pr_head)

    monkeypatch.chdir(git_repo)
    monkeypatch.setenv("PR_BASE_SHA", fork_point)
    monkeypatch.setenv("PR_HEAD_SHA", pr_head)
    assert changed_refs(argparse.Namespace(changed=True)) == (base_tip, "HEAD")

    git(git_repo, "checkout", "-q", pr_head)
    assert changed_refs(argparse.Namespace(changed=True)) == (fork_point, pr_head)


@pytest.fixture
def fake_precommit(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> list[list[str]]:
    """Run main() against tmp_path, recording prek invocations."""
    commands: list[list[str]] = []

    def fake_run(command: list[str], **_kwargs: object) -> subprocess.CompletedProcess:
        commands.append(command)
        if command[:2] == ["prek", "run"]:
            failed = len(command) > 2 and command[2] == "typos"
            return completed(HOOK_OUTPUT if failed else "", returncode=int(failed))
        return completed()

    monkeypatch.setattr(pre_commit, "repo_root", lambda: tmp_path)
    monkeypatch.setattr(pre_commit, "ensure_precommit_available", lambda: True)
    monkeypatch.setattr(pre_commit, "run_captured", fake_run)
    return commands


def test_docs_runs_each_hook_on_selected_pages(
    fake_precommit: list[list[str]], monkeypatch: pytest.MonkeyPatch
) -> None:
    monkeypatch.setattr(pre_commit, "select_docs_files", lambda refs: ["docs/en/a.md"])

    assert pre_commit.main(["--docs"]) == 1
    assert fake_precommit == [
        build_precommit_args(["--files", "docs/en/a.md"], hook) for hook in DOCS_HOOKS
    ]


def test_docs_hooks_exist_and_vale_syncs_before_linting() -> None:
    repos = load_yaml(".pre-commit-config.yaml")
    hooks = [(repo["repo"], hook) for repo in repos["repos"] for hook in repo["hooks"]]
    assert set(DOCS_HOOKS) <= {hook["id"] for _, hook in hooks}
    # `prek run vale` selects both hooks in order; one repo builds one environment.
    vale = [(repo, hook) for repo, hook in hooks if hook["id"] == "vale"]
    assert {repo for repo, _ in vale} == {"https://github.com/errata-ai/vale"}
    (sync, lint) = (hook for _, hook in vale)
    assert (sync.get("alias"), sync["entry"]) == ("vale-sync", "vale sync")
    assert "alias" not in lint
    assert lint["verbose"] is True


def test_install_takes_hook_types_from_config(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    calls: list[list[str]] = []
    monkeypatch.setattr(pre_commit, "repo_root", lambda: tmp_path)
    monkeypatch.setattr("qgc_tools.python_env.tool_command", lambda name, group: [name])
    monkeypatch.setattr(pre_commit.subprocess, "run", lambda command, **_: calls.append(command))

    assert pre_commit.main(["--install"]) == 0
    assert calls == [["prek", "install"]]


def _precommit_config() -> dict:
    return load_yaml(".pre-commit-config.yaml")


def _commit_msg_hook() -> dict:
    (hook,) = (
        hook
        for repo in _precommit_config()["repos"]
        for hook in repo["hooks"]
        if hook["id"] == "conventional-commit-msg"
    )
    return hook


def test_only_the_commit_message_hook_runs_at_commit_msg() -> None:
    config = _precommit_config()
    assert config["default_install_hook_types"] == ["pre-commit", "commit-msg"]
    assert "commit-msg" not in config["default_stages"]
    commit_msg_hooks = [
        hook["id"]
        for repo in config["repos"]
        for hook in repo["hooks"]
        if "commit-msg" in hook.get("stages", [])
    ]
    assert commit_msg_hooks == ["conventional-commit-msg"]


@pytest.mark.parametrize(
    ("message", "valid"),
    [
        ("feat: add x\n", True),
        ("fix(Vehicle): guard null\n\nBody text\n", True),
        ("feat!: drop y\n", True),
        ("refactor(Comms)!: rework z\n", True),
        ("deps(deps): update renovate\n", True),
        ("Merge branch 'master' into topic\n", True),
        ('Revert "feat: add x"\n', True),
        ("fixup! feat: add x\n", True),
        ("Update stuff\n", False),
        ("feat:nospace\n", False),
        ("feature: x\n", False),
        ("Fix: capitalized type\n", False),
        ("feat(): empty scope\n", False),
        ("Update stuff\n\nfeat: only in the body\n", False),
    ],
)
def test_conventional_commit_msg_hook(message: str, valid: bool) -> None:
    hook = _commit_msg_hook()
    assert (hook["language"], hook["args"]) == ("pygrep", ["--multiline"])
    # pygrep --multiline compiles with MULTILINE | DOTALL and fails the commit on a match.
    pattern = re.compile(hook["entry"], re.MULTILINE | re.DOTALL)
    assert (pattern.search(message) is None) == valid


def test_conventional_commit_types_match_release_rules() -> None:
    hook = _commit_msg_hook()
    match = re.search(r"\(\?!\(([a-z|]+)\)", hook["entry"])
    assert match
    release = json.loads((REPO_ROOT / ".releaserc.json").read_text(encoding="utf-8"))
    analyzer = next(
        plugin[1]
        for plugin in release["plugins"]
        if plugin[0] == "@semantic-release/commit-analyzer"
    )
    release_types = {rule["type"] for rule in analyzer["releaseRules"] if "type" in rule}
    # Renovate commits use `deps` (.github/renovate.json :semanticCommitTypeAll(deps)).
    assert set(match.group(1).split("|")) == release_types | {"deps"}


def test_renovate_validator_runs_in_ci_when_its_inputs_change() -> None:
    hooks = {hook["id"]: hook for repo in _precommit_config()["repos"] for hook in repo["hooks"]}
    assert hooks["renovate-config-validator"]["stages"] == ["manual"]
    assert "check-renovate" in hooks
    workflow = load_workflow("ci-scripts.yml")
    inputs = (".github/renovate.json", ".pre-commit-config.yaml")
    for event in ("pull_request", "push"):
        paths = workflow["on"][event]["paths"]
        assert all(any(fnmatch.fnmatchcase(name, pattern) for pattern in paths) for name in inputs)
    steps = {step["name"]: step for step in workflow["jobs"]["validate-renovate-config"]["steps"]}
    assert " ".join(inputs) in steps["Detect Renovate config changes"]["run"]
    assert steps["Validate Renovate config"]["run"] == (
        "prek run renovate-config-validator --hook-stage manual --files .github/renovate.json"
    )


def test_explicit_hooks_override_docs_hooks(
    fake_precommit: list[list[str]], monkeypatch: pytest.MonkeyPatch
) -> None:
    monkeypatch.setattr(pre_commit, "select_docs_files", lambda refs: ["docs/en/a.md"])

    assert pre_commit.main(["--docs", "--hook", "vale"]) == 0
    assert fake_precommit == [build_precommit_args(["--files", "docs/en/a.md"], "vale")]


def test_docs_without_selected_pages_skips_hooks(
    fake_precommit: list[list[str]], monkeypatch: pytest.MonkeyPatch, tmp_path: Path
) -> None:
    monkeypatch.setattr(pre_commit, "select_docs_files", lambda refs: [])

    assert pre_commit.main(["--docs", "--results-dir", str(tmp_path / "results")]) == 0
    assert fake_precommit == []
    results = json.loads((tmp_path / "results/pre-commit-results.json").read_text())
    assert results["exit_code"] == "0"


def test_results_dir_writes_artifact_relative_to_invocation_directory(
    fake_precommit: list[list[str]], monkeypatch: pytest.MonkeyPatch, tmp_path: Path
) -> None:
    invocation = tmp_path / "invocation"
    invocation.mkdir()
    monkeypatch.chdir(invocation)
    monkeypatch.setenv("GITHUB_SERVER_URL", "https://github.com")
    monkeypatch.setenv("GITHUB_REPOSITORY", "owner/repo")
    monkeypatch.setenv("GITHUB_RUN_ID", "42")

    assert pre_commit.main(["--hook", "typos", "--results-dir", "results"]) == 1

    results = invocation / "results"
    assert (results / "pre-commit-results.json").read_text(encoding="utf-8") == (
        json.dumps(
            {
                "exit_code": "1",
                "passed": "1",
                "failed": "1",
                "skipped": "0",
                "run_url": "https://github.com/owner/repo/actions/runs/42",
            },
            indent=2,
        )
        + "\n"
    )
    assert (results / "pre-commit-output.txt").read_text(encoding="utf-8") == HOOK_OUTPUT


def test_results_dir_records_failure_when_precommit_is_missing(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    monkeypatch.setattr(pre_commit, "repo_root", lambda: tmp_path)
    monkeypatch.setattr(pre_commit, "ensure_precommit_available", lambda: False)

    assert pre_commit.main(["--results-dir", str(tmp_path / "results")]) == 1
    results = json.loads((tmp_path / "results/pre-commit-results.json").read_text())
    assert results["exit_code"] == "1"


def test_ci_failure_writes_summary_outputs_and_annotation(
    fake_precommit: list[list[str]],
    monkeypatch: pytest.MonkeyPatch,
    tmp_path: Path,
    capsys: pytest.CaptureFixture[str],
) -> None:
    summary = tmp_path / "summary.md"
    output = tmp_path / "output.txt"
    monkeypatch.setenv("GITHUB_STEP_SUMMARY", str(summary))
    monkeypatch.setenv("GITHUB_OUTPUT", str(output))

    assert pre_commit.main(["--ci", "--hook", "typos"]) == 1

    assert "**Some checks failed**" in summary.read_text(encoding="utf-8")
    outputs = output.read_text(encoding="utf-8").splitlines()
    assert {"exit_code=1", "passed=1", "failed=1"} <= set(outputs)
    assert "::error::Required pre-commit checks failed" in capsys.readouterr().out


def test_precommit_workflow_relies_on_step_failure_and_uploads_results() -> None:
    steps = get_steps(load_workflow("pre-commit.yml"), "pre-commit")
    by_name = {step.get("name"): step for step in steps}
    run = by_name["Run pre-commit"]
    assert "continue-on-error" not in run
    assert run["run"] == (
        "python3 ./tools/pre_commit.py --ci --changed --results-dir pre-commit-results"
    )
    assert run["env"]["SKIP"] == "trufflehog"
    assert by_name["Upload pre-commit results artifact"]["with"]["path"] == "pre-commit-results"
    after_run = steps[steps.index(run) + 1 :]
    assert after_run
    for step in after_run:
        assert "always()" in step["if"] or "!cancelled()" in step["if"]


def test_docs_lint_workflow_triggers_on_every_full_scan_input() -> None:
    workflow = load_workflow("docs-lint.yml")
    triggers = workflow["on"]["pull_request"]["paths"]
    assert set(DOCS_FULL_SCAN_PATHSPECS) <= set(triggers)
    assert any(fnmatch.fnmatchcase("docs/en/guide/page.md", pattern) for pattern in triggers)
    (lint,) = (
        step
        for step in workflow["jobs"]["lint"]["steps"]
        if step.get("name") == "Lint English documentation"
    )
    assert lint["env"]["PR_BASE_SHA"] == "${{ github.event.pull_request.base.sha }}"
    assert lint["env"]["PR_HEAD_SHA"] == "${{ github.event.pull_request.head.sha }}"
    # Manual dispatch has no base and checks every page.
    assert lint["run"] == "python3 ./tools/pre_commit.py --ci --docs ${PR_BASE_SHA:+--changed}"


def _hooks_by_id() -> dict[str, dict]:
    return {hook["id"]: hook for repo in _precommit_config()["repos"] for hook in repo["hooks"]}


def test_editorconfig_checker_relaxes_only_alignment_checks() -> None:
    hook = _hooks_by_id()["editorconfig-checker"]
    assert "stages" not in hook
    config = json.loads((REPO_ROOT / ".editorconfig-checker.json").read_text(encoding="utf-8"))
    assert config["Disable"] == {"IndentSize": True, "MaxLineLength": True}


@pytest.mark.parametrize(
    ("hook_id", "check_arg"),
    [("gersemi", "--check"), ("yamlfmt", "-lint"), ("qmlformat", None)],
)
def test_trial_formatters_only_check_on_the_manual_stage(hook_id: str, check_arg: str | None):
    hook = _hooks_by_id()[hook_id]
    assert hook["stages"] == ["manual"]
    if check_arg:
        assert check_arg in hook["args"]
    else:
        assert hook["entry"] == "python3 ./tools/analyzers/qmlformat.py"


def test_prek_is_pinned_and_runs_lint() -> None:
    pyproject = (REPO_ROOT / "tools/pyproject.toml").read_text(encoding="utf-8")
    assert re.search(r'^\s*"prek==\d+\.\d+\.\d+",$', pyproject, re.MULTILINE)
    justfile = (REPO_ROOT / "justfile").read_text(encoding="utf-8")
    assert "--group precommit prek run --all-files" in justfile


PREK_CACHE_PATH = "~/.cache/prek\n!~/.cache/prek/cache\n"


@pytest.mark.parametrize(
    ("workflow", "job", "prefix"),
    [
        ("pre-commit.yml", "pre-commit", "prek-v1-"),
        ("copilot-setup-steps.yml", "copilot-setup-steps", "prek-v1-"),
        ("ci-scripts.yml", "validate-renovate-config", "prek-renovate-v1-"),
    ],
)
def test_workflows_cache_prek_environments(workflow: str, job: str, prefix: str) -> None:
    steps = get_steps(load_workflow(workflow), job)
    caches = [step for step in steps if step.get("uses", "").startswith("actions/cache/")]
    assert caches
    # actions/cache keys entries on the path list, so restore and save must match exactly.
    assert {step["with"]["path"] for step in caches} == {PREK_CACHE_PATH}
    (restore,) = (step for step in caches if "restore" in step["uses"])
    assert restore["with"]["key"].startswith(prefix)
    assert restore["with"]["restore-keys"].startswith(prefix)
    for step in steps:
        assert "pre-commit gc" not in step.get("run", "")
        if "gc" in step.get("run", ""):
            assert step["run"] == "prek cache gc"


def test_whitespace_hooks_leave_syrupy_snapshots_alone() -> None:
    snapshot = "tools/tests/ci/__snapshots__/test_generate_build_results_comment.ambr"
    hooks = _hooks_by_id()
    for hook_id in ("trailing-whitespace", "end-of-file-fixer", "mixed-line-ending"):
        assert re.search(hooks[hook_id]["exclude"], snapshot), hook_id
    config = json.loads((REPO_ROOT / ".editorconfig-checker.json").read_text(encoding="utf-8"))
    assert any(re.search(pattern, snapshot) for pattern in config["Exclude"])
