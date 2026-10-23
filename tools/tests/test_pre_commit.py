"""Tests for tools/pre_commit.py."""

from __future__ import annotations

import fnmatch
import json
import re
from typing import TYPE_CHECKING

import pre_commit
import pytest
from common.git import ZERO_SHA
from common.io import read_toml
from pre_commit import (
    DOCS_FULL_SCAN_PATHSPECS,
    DOCS_HOOKS,
    QT_PATHSPECS,
    build_precommit_args,
    changed_refs,
    describe_selection,
    extract_hook_lines,
    failed_hooks,
    file_selection_args,
    needs_qt,
    parse_args,
    select_docs_files,
    summarize_output,
)

from ._helpers import (
    REPO_ROOT,
    commit_files,
    completed,
    get_step,
    get_steps,
    git,
    load_workflow,
    load_yaml,
    pr_merge_checkout,
)

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


def test_failed_hooks_lists_names_in_run_order() -> None:
    output = (
        "clang-format.......................\x1b[41mFailed\x1b[0m\n"
        "- hook id: clang-format\n"
        "ruff check (python)................Passed\n"
        "qmllint.........(no files to check)Skipped\n"
        "check yaml.........................Failed\n"
        "Failed to load a file\n"
    )
    assert failed_hooks(output) == ["clang-format", "check yaml"]


def test_describe_selection() -> None:
    assert describe_selection(["--all-files"]) == "all files"
    assert (
        describe_selection(["--from-ref", "a" * 40, "--to-ref", "b"])
        == "changed since aaaaaaaaaaaa"
    )
    assert describe_selection(["--files", "a.md", "b.md"]) == "2 selected files"


def test_build_precommit_args_all_files() -> None:
    assert file_selection_args(parse_args([])) == ["--all-files"]
    assert build_precommit_args(["--all-files"]) == [
        "prek",
        "run",
        "--show-diff-on-failure",
        "--color=always",
        "--all-files",
    ]


def test_build_precommit_args_places_hooks_before_options() -> None:
    assert build_precommit_args(["--files", "a.md"], ["typos", "vale"]) == [
        "prek",
        "run",
        "typos",
        "vale",
        "--show-diff-on-failure",
        "--color=always",
        "--files",
        "a.md",
    ]


@pytest.mark.parametrize(
    ("branch", "selection"),
    [
        ("master", ["--from-ref", "master", "--to-ref", "HEAD"]),
        ("main", ["--from-ref", "main", "--to-ref", "HEAD"]),
        ("trunk", ["--all-files"]),
    ],
)
def test_changed_mode_diffs_against_the_default_branch(
    git_repo: Path, monkeypatch: pytest.MonkeyPatch, branch: str, selection: list[str]
) -> None:
    commit_files(git_repo, {"a.txt": "a\n"}, "Base")
    git(git_repo, "branch", "-m", branch)
    git(git_repo, "checkout", "-qb", "topic")
    monkeypatch.chdir(git_repo)
    monkeypatch.delenv("PR_BASE_SHA", raising=False)
    monkeypatch.delenv("PR_HEAD_SHA", raising=False)
    assert file_selection_args(parse_args(["--changed"])) == selection


def test_new_branch_push_checks_all_files(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setenv("PR_BASE_SHA", ZERO_SHA)
    assert file_selection_args(parse_args(["--changed"])) == ["--all-files"]


def test_summary_counts_colored_hook_results_and_ignores_diagnostics():
    output = (
        "format...........\x1b[41mFailed\x1b[0m\n"
        "python...........\x1b[42mPassed\x1b[0m\n"
        "qml..............(no files)\x1b[46mSkipped\x1b[0m\n"
        "Failed to load a file\n"
    )
    assert summarize_output(output) == (1, 1, 1)


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
    base = commit_files(
        git_repo, dict.fromkeys([*ENGLISH_PAGES, "docs/fr/index.md"], "# Original\n"), "Base"
    )
    if changed:
        commit_files(git_repo, {changed: "# Changed\n"}, "Change")

    monkeypatch.chdir(git_repo)
    assert select_docs_files((base, "HEAD") if changed else None) == expected


def test_docs_selection_ignores_deleted_pages(
    git_repo: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    base = commit_files(git_repo, dict.fromkeys(ENGLISH_PAGES, "# Original\n"), "Base")
    git(git_repo, "rm", "-q", "docs/en/index.md")
    git(git_repo, "commit", "-qm", "Delete")

    monkeypatch.chdir(git_repo)
    assert select_docs_files((base, "HEAD")) == []


@pytest.mark.parametrize("clone", ["full", "shallow"])
def test_changed_refs_diff_a_pr_merge_checkout_against_its_base_tip(
    tmp_path: Path, isolated_git: None, monkeypatch: pytest.MonkeyPatch, clone: str
) -> None:
    pr = pr_merge_checkout(
        tmp_path,
        base={"docs/en/index.md": "# Original\n"},
        newer_base={"docs/en/base.md": "# Base\n", "src/Base.qml": "Item {}\n"},
        pr={"docs/en/pr.md": "# PR\n"},
    )
    monkeypatch.chdir(getattr(pr, clone))
    monkeypatch.setenv("PR_BASE_SHA", pr.fork_point)
    monkeypatch.setenv("PR_HEAD_SHA", pr.pr_head)

    refs = changed_refs(parse_args(["--changed"]))
    assert refs == (pr.base_tip, "HEAD")
    assert file_selection_args(parse_args(["--changed"])) == [
        *("--from-ref", pr.base_tip, "--to-ref", "HEAD")
    ]
    assert select_docs_files(refs) == ["docs/en/pr.md"]
    assert not needs_qt(refs)


@pytest.mark.parametrize(
    ("pr_files", "expected"),
    [({"src/Pr.qml": "Item {}\n"}, "true"), ({"src/pr.cc": "int x;\n"}, "false")],
)
def test_needs_qt_reports_whether_qml_is_selected(
    tmp_path: Path,
    isolated_git: None,
    monkeypatch: pytest.MonkeyPatch,
    gh_output: Path,
    pr_files: dict[str, str],
    expected: str,
) -> None:
    pr = pr_merge_checkout(
        tmp_path / "repo",
        base={"src/Old.qml": "Item {}\n"},
        newer_base={"src/Old.qml": "Rectangle {}\n"},
        pr=pr_files,
    )
    monkeypatch.setattr(pre_commit, "repo_root", lambda: pr.shallow)
    monkeypatch.setenv("PR_BASE_SHA", pr.fork_point)
    monkeypatch.setenv("PR_HEAD_SHA", pr.pr_head)

    assert pre_commit.main(["--changed", "--needs-qt"]) == 0
    assert gh_output.read_text(encoding="utf-8") == f"needs-qt={expected}\n"


def test_full_runs_need_qt() -> None:
    assert needs_qt(None)


@pytest.fixture
def fake_precommit(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> list[list[str]]:
    """Run main() against tmp_path, recording prek invocations; selecting typos fails."""
    commands: list[list[str]] = []

    def fake_tee(command: list[str], output_file: Path, **_kwargs: object) -> int:
        commands.append(command)
        failed = "typos" in command
        output_file.write_text(HOOK_OUTPUT if failed else "", encoding="utf-8")
        return int(failed)

    def fake_run(command: list[str], **_kwargs: object) -> subprocess.CompletedProcess:
        commands.append(command)
        return completed()

    monkeypatch.setattr(pre_commit, "repo_root", lambda: tmp_path)
    monkeypatch.setattr(pre_commit, "ensure_precommit_available", lambda: True)
    monkeypatch.setattr(pre_commit, "run_tee", fake_tee)
    monkeypatch.setattr(pre_commit, "run_captured", fake_run)
    return commands


def test_docs_runs_the_docs_hooks_on_selected_pages_in_one_call(
    fake_precommit: list[list[str]], monkeypatch: pytest.MonkeyPatch
) -> None:
    monkeypatch.setattr(pre_commit, "select_docs_files", lambda refs: ["docs/en/a.md"])

    assert pre_commit.main(["--docs"]) == 1
    assert fake_precommit == [build_precommit_args(["--files", "docs/en/a.md"], DOCS_HOOKS)]


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


def test_conventional_commit_types_match_release_notes_sections() -> None:
    hook = _commit_msg_hook()
    match = re.search(r"\(\?!\(([a-z|]+)\)", hook["entry"])
    assert match
    parsers = read_toml(REPO_ROOT / "tools/configs/cliff.toml")["git"]["commit_parsers"]
    # Each type either opens a release-notes section or is skipped; Renovate commits use `deps`
    # (.github/renovate.json :semanticCommitTypeAll(deps)).
    notes_types = {
        name
        for parser in parsers
        if (types := re.match(r"\^\(?([a-z|]+)", parser.get("message", "")))
        for name in types.group(1).split("|")
    }
    assert set(match.group(1).split("|")) == notes_types


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
    assert fake_precommit == [build_precommit_args(["--files", "docs/en/a.md"], ["vale"])]


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
                "failed_hooks": ["lint"],
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


def test_ci_failure_writes_summary_and_annotation(
    fake_precommit: list[list[str]],
    gh_output: Path,
    gh_summary: Path,
    capsys: pytest.CaptureFixture[str],
) -> None:
    assert pre_commit.main(["--ci", "--hook", "typos"]) == 1

    # Without --results-dir the streamed output is read back from a temporary copy.
    summary = gh_summary.read_text(encoding="utf-8")
    assert summary.startswith("### Pre-commit\n\n| Item | Value |")
    assert "| Result | failed |" in summary
    assert "| Hooks | 1 passed, 1 failed, 0 skipped |" in summary
    assert "Failed hooks: `lint`." in summary
    assert "<details><summary>Hook results</summary>" in summary
    assert "lint........................Failed" in summary
    # No workflow step reads outputs; the summary and results artifact carry the counts.
    assert gh_output.read_text(encoding="utf-8") == ""
    assert "::error::Required pre-commit checks failed" in capsys.readouterr().out


def test_precommit_workflow_relies_on_step_failure_and_uploads_results() -> None:
    job = load_workflow("pre-commit.yml")["jobs"]["pre-commit"]
    assert job["env"] == {
        "PR_BASE_SHA": "${{ github.event.pull_request.base.sha || github.event.before }}",
        "PR_HEAD_SHA": "${{ github.event.pull_request.head.sha || github.sha }}",
    }
    steps = job["steps"]
    by_name = {step.get("name"): step for step in steps}
    checkout = by_name["Checkout"]["with"]
    # A merge checkout and its parents name a PR's diff; pushes diff against `before`.
    assert checkout["fetch-depth"] == "${{ github.event_name == 'pull_request' && 2 || 0 }}"
    run = by_name["Run pre-commit"]
    assert "continue-on-error" not in run
    assert run["run"] == (
        "python3 ./tools/pre_commit.py --ci --changed --results-dir pre-commit-results"
    )
    # Docs Lint owns the prose linter; `vale` also selects the `vale-sync` alias.
    assert run["env"] == {"SKIP": "trufflehog,vale"}
    assert "vale" in DOCS_HOOKS
    assert by_name["Upload pre-commit results artifact"]["with"]["path"] == "pre-commit-results"
    after_run = steps[steps.index(run) + 1 :]
    assert after_run
    for step in after_run:
        assert "always()" in step["if"] or "!cancelled()" in step["if"]


def test_precommit_installs_qt_only_when_qml_is_checked() -> None:
    steps = get_steps(load_workflow("pre-commit.yml"), "pre-commit")
    query = get_step(steps, "qml")
    assert query["run"] == "python3 ./tools/pre_commit.py --changed --needs-qt"
    install = get_step(steps, "Install Qt (for qmllint)")
    assert install["if"] == "steps.qml.outputs.needs-qt == 'true'"
    assert steps.index(get_step(steps, "Setup Python")) < steps.index(query) < steps.index(install)
    # qmllint is the only automatic hook that runs a Qt tool, and only on QML_PATHSPECS files.
    qt_hooks = {
        hook_id
        for hook_id, hook in _hooks_by_id().items()
        if "qml" in hook.get("entry", "") and hook.get("stages") != ["manual"]
    }
    assert qt_hooks == {"qmllint"}
    assert _hooks_by_id()["qmllint"]["files"] == r"\.qml$"
    assert QT_PATHSPECS == ("*.qml",)


def test_docs_lint_workflow_triggers_on_every_full_scan_input() -> None:
    workflow = load_workflow("docs-lint.yml")
    triggers = workflow["on"]["pull_request"]["paths"]
    assert set(DOCS_FULL_SCAN_PATHSPECS) <= set(triggers)
    assert any(fnmatch.fnmatchcase("docs/en/guide/page.md", pattern) for pattern in triggers)
    (checkout,) = (
        step
        for step in workflow["jobs"]["lint"]["steps"]
        if step.get("uses", "").startswith("actions/checkout@")
    )
    # Pull request merge checkouts diff against their first parent; manual runs check every page.
    assert checkout["with"]["fetch-depth"] == 2
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


EDITORCONFIG_CHECKER_CONFIG = REPO_ROOT / "tools/configs/editorconfig-checker.json"


def test_editorconfig_checker_relaxes_only_alignment_checks() -> None:
    hook = _hooks_by_id()["editorconfig-checker"]
    assert "stages" not in hook
    config = json.loads(EDITORCONFIG_CHECKER_CONFIG.read_text(encoding="utf-8"))
    assert config["Disable"] == {"IndentSize": True, "MaxLineLength": True}


@pytest.mark.parametrize(
    ("hook_id", "flag", "config"),
    [
        ("editorconfig-checker", "-config", "tools/configs/editorconfig-checker.json"),
        ("gersemi", "--config", "tools/configs/gersemirc.yaml"),
        ("yamlfmt", "-conf", "tools/configs/yamlfmt.yaml"),
    ],
)
def test_hooks_pass_configs_their_tools_do_not_discover(hook_id: str, flag: str, config: str):
    # Each tool only looks for its configuration in the working directory or the file's parents.
    args = _hooks_by_id()[hook_id]["args"]
    assert args[args.index(flag) + 1] == config
    assert (REPO_ROOT / config).is_file()


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
        assert hook["entry"].endswith(" python ./tools/analyzers/qmlformat.py")


def test_local_python_hooks_run_in_the_locked_environment() -> None:
    # A git hook's bare python3 may be a system interpreter older than the tools' 3.10 floor.
    scripts = {
        hook_id: hook["entry"]
        for hook_id, hook in _hooks_by_id().items()
        if hook.get("language") == "system" and ".py" in hook["entry"]
    }
    hooks = {
        "qmllint",
        "qmlformat",
        "clang-tidy",
        "clazy",
        "vehicle-null-check",
        "qt-translate-noop-check",
    }
    assert hooks <= set(scripts)
    for hook_id, entry in scripts.items():
        assert entry.startswith("uv run --frozen "), hook_id


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
        ("docs-lint.yml", "lint", "prek-v1-"),
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


@pytest.mark.parametrize(
    ("workflow", "job"),
    [("docs-lint.yml", "lint"), ("copilot-setup-steps.yml", "copilot-setup-steps")],
)
def test_other_workflows_restore_prek_environments_read_only(workflow: str, job: str) -> None:
    steps = get_steps(load_workflow(workflow), job)
    (cache,) = (step for step in steps if step.get("uses", "").startswith("actions/cache"))
    assert cache["uses"].startswith("actions/cache/restore@")
    pre_commit_restore = get_step(
        get_steps(load_workflow("pre-commit.yml"), "pre-commit"), "pre-commit-cache"
    )
    assert cache["with"] == pre_commit_restore["with"]


def test_pushes_prepare_every_hook_environment_before_saving() -> None:
    # prek installs environments only for hooks that run, which would freeze a partial cache.
    steps = get_steps(load_workflow("pre-commit.yml"), "pre-commit")
    run = get_step(steps, "Run pre-commit")
    prepare = get_step(steps, "prepare-hooks")
    prune = get_step(steps, "Prune unused prek environments")
    save = get_step(steps, "Save prek environments")
    assert steps.index(run) < steps.index(prepare) < steps.index(prune) < steps.index(save)
    assert prepare["if"] == prune["if"]
    assert "github.event_name == 'push'" in prepare["if"]
    assert prepare["continue-on-error"] is True
    assert save["if"].startswith(prepare["if"])
    assert "steps.prepare-hooks.outcome == 'success'" in save["if"]
    command = prepare["run"].split()
    assert command[:2] == ["prek", "prepare-hooks"]
    skipped = set(command[3::2])
    assert command[2::2] == ["--skip"] * len(skipped)
    # Docs Lint restores this cache for vale; TruffleHog and the Renovate validator never use it.
    assert skipped == {"trufflehog", "renovate-config-validator"}
    assert skipped <= set(_hooks_by_id())


def test_whitespace_hooks_leave_syrupy_snapshots_alone() -> None:
    snapshot = "tools/tests/ci/__snapshots__/test_generate_build_results_comment.ambr"
    hooks = _hooks_by_id()
    for hook_id in ("trailing-whitespace", "end-of-file-fixer", "mixed-line-ending"):
        assert re.search(hooks[hook_id]["exclude"], snapshot), hook_id
    config = json.loads(EDITORCONFIG_CHECKER_CONFIG.read_text(encoding="utf-8"))
    assert any(re.search(pattern, snapshot) for pattern in config["Exclude"])
