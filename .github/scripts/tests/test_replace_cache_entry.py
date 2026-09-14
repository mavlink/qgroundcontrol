"""Run the composite action's deletion step against a mock GitHub CLI."""

from __future__ import annotations

import os
import subprocess

import pytest
import yaml
from _helpers import REPO_ROOT


@pytest.mark.parametrize("cli_status", [0, 1])
def test_replace_cache_uses_builtin_command_and_scopes_ref(cli_status: int) -> None:
    action = yaml.safe_load(
        (REPO_ROOT / ".github/actions/replace-cache-entry/action.yml").read_text()
    )
    script = action["runs"]["steps"][0]["run"]
    result = subprocess.run(
        ["bash", "-c", f'gh() {{ printf "<%s>\\n" "$@"; return {cli_status}; }};\n' + script],
        env={
            **os.environ,
            "CACHE_KEY": "baseline key",
            "GH_REPO": "owner/repo",
            "CACHE_REF": "refs/heads/master",
        },
        capture_output=True,
        text=True,
        check=False,
    )
    assert result.returncode == 0  # Deleting a missing entry must not prevent the subsequent save.
    assert result.stdout.splitlines()[:7] == [
        "<cache>",
        "<delete>",
        "<baseline key>",
        "<--repo>",
        "<owner/repo>",
        "<--ref>",
        "<refs/heads/master>",
    ]
    assert ("::warning::" in result.stdout) is bool(cli_status)
