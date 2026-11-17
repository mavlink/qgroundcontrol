"""Tests for tools/ci_script_index.py."""

from __future__ import annotations

from typing import TYPE_CHECKING

import ci_script_index
import pytest
from ci_script_index import END, NO_DOCSTRING, START, render_index, replace_index

from ._helpers import write_files

if TYPE_CHECKING:
    from pathlib import Path

README = "tools/ci/README.md"


@pytest.fixture
def repo(tmp_path: Path) -> Path:
    write_files(
        tmp_path,
        {
            ".github/workflows/build.yml": (
                "run: python3 tools/ci/pack/plan.py --all\n# tools/ci/pack/helper.pyc\n"
            ),
            ".github/actions/setup/action.yml": (
                'run: python3 "${GITHUB_WORKSPACE}/tools/ci/pack/report.py"\n'
            ),
            ".pre-commit-config.yaml": "entry: python tools/ci/pack/report.py\n",
            "justfile": "",
            "tools/ci/pack/__init__.py": '"""Planning and\nreports."""\n',
            "tools/ci/pack/plan.py": (
                '"""Plan the ``Stable*`` build | legs,\nfor every platform.\n\nUsage: plan.py\n"""\n'
                "from ci.pack import helper\n"
            ),
            "tools/ci/pack/helper.py": '"""Library: shared planning."""\n',
            "tools/ci/pack/report.py": "import sys\n",
            "tools/ci/bare/tool.py": '"""Standalone."""\n',
            README: f"# CI Scripts\n\n{START}\nstale\n{END}\n\n## Tests\n",
        },
    )
    return tmp_path


def test_index_takes_purposes_from_docstrings_and_names_what_uses_each_script(repo: Path) -> None:
    index = render_index(repo)

    assert index.startswith(START)
    assert index.endswith(END)
    # A package without __init__.py and a script without a docstring fall back instead of failing.
    assert f"### `bare/`\n\n{NO_DOCSTRING}\n" in index
    assert "| `bare/tool.py` | Standalone. | — |" in index
    assert "### `pack/`\n\nPlanning and reports.\n" in index
    assert (
        "| `pack/plan.py` | Plan the `Stable*` build \\| legs, for every platform. | `build.yml` |"
    ) in index
    assert "| `pack/helper.py` | Library: shared planning. | `pack/plan.py` |" in index
    assert (
        f"| `pack/report.py` | {NO_DOCSTRING} | `actions/setup`, `.pre-commit-config.yaml` |"
    ) in index


def test_check_reports_a_stale_index_that_a_plain_run_rewrites(
    repo: Path, monkeypatch: pytest.MonkeyPatch, capsys: pytest.CaptureFixture[str]
) -> None:
    monkeypatch.setattr(ci_script_index, "REPO_ROOT", repo)
    readme = repo / README
    stale = readme.read_text(encoding="utf-8")

    assert ci_script_index.main(["--check"]) == 1
    assert readme.read_text(encoding="utf-8") == stale
    # The failure names the command that regenerates the index.
    assert (
        "`uv run --frozen --project tools python tools/ci_script_index.py`"
        in capsys.readouterr().err
    )

    assert ci_script_index.main([]) == 0
    updated = readme.read_text(encoding="utf-8")
    assert updated == f"# CI Scripts\n\n{render_index(repo)}\n\n## Tests\n"
    assert ci_script_index.main(["--check"]) == 0


def test_readme_without_both_markers_is_an_error(
    repo: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    with pytest.raises(ValueError, match="markers"):
        replace_index(f"{END}\n{START}\n", "index")

    monkeypatch.setattr(ci_script_index, "REPO_ROOT", repo)
    (repo / README).write_text("# CI Scripts\n", encoding="utf-8")
    assert ci_script_index.main([]) == 1
    assert (repo / README).read_text(encoding="utf-8") == "# CI Scripts\n"
