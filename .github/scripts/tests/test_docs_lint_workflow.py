"""Exercise Docs Lint file selection using real Git changes."""

import fnmatch
import os
import subprocess

import pytest
import yaml
from _helpers import REPO_ROOT


@pytest.mark.parametrize(
    ("changed", "expected"),
    [
        (".typos.toml", "all"),
        (".vale.ini", "all"),
        (".markdownlint.yaml", "all"),
        (".markdownlintignore", "all"),
        (".pre-commit-config.yaml", "all"),
        (".github/workflows/docs-lint.yml", "all"),
        ("docs/en/guide/new page.md", "changed"),
        ("docs/en/index.md", "changed"),
        ("docs/en/image.svg", "none"),
        ("docs/fr/index.md", "none"),
        (None, "all"),
    ],
)
def test_docs_lint_selects_files(tmp_path, changed, expected):
    workflow = yaml.safe_load((REPO_ROOT / ".github/workflows/docs-lint.yml").read_text())
    triggers = workflow[True]["pull_request"]["paths"]
    if expected != "none" and changed:
        assert any(fnmatch.fnmatchcase(changed, pattern) for pattern in triggers)
    script = next(
        step["run"]
        for step in workflow["jobs"]["lint"]["steps"]
        if step.get("name") == "Lint English documentation"
    )

    def git(*args):
        return subprocess.run(
            ["git", *args], cwd=tmp_path, check=True, capture_output=True, text=True
        ).stdout.strip()

    def write(path, content):
        target = tmp_path / path
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text(content)

    git("init", "-q")
    git("config", "user.name", "Test")
    git("config", "user.email", "test@example.com")
    english = ["docs/en/index.md", "docs/en/guide/new page.md"]
    for path in [*english, "docs/fr/index.md"]:
        write(path, "# Original\n")
    git("add", ".")
    git("commit", "-qm", "Base")
    base = git("rev-parse", "HEAD")
    if changed:
        write(changed, "# Changed\n")
        git("add", ".")
        git("commit", "-qm", "Change")

    write("bin/pre-commit", '#!/bin/sh\nprintf "%s\\0" "$@" >> "$HOOK_LOG"\n')
    (tmp_path / "bin/pre-commit").chmod(0o755)
    log = tmp_path / "hooks.log"
    result = subprocess.run(
        ["bash", "-e", "-o", "pipefail", "-c", script],
        cwd=tmp_path,
        env={
            **os.environ,
            "PATH": f"{tmp_path / 'bin'}:{os.environ['PATH']}",
            "BASE_SHA": base if changed else "",
            "HOOK_LOG": str(log),
        },
        check=True,
        capture_output=True,
        text=True,
    )
    selected = sorted(english) if expected == "all" else [changed]
    if expected == "none":
        assert not log.exists()
        assert "No changed English Markdown files" in result.stdout
    else:
        calls = log.read_text().split("\0")[:-1]
        assert calls == [
            arg
            for hook in ("markdownlint", "typos", "vale-sync", "vale")
            for arg in ["run", hook, "--files", *selected]
        ]
