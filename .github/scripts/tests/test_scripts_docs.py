"""Keep the CI script index and production test coverage complete."""

from __future__ import annotations

import re

from _helpers import REPO_ROOT

SCRIPTS_DIR = REPO_ROOT / ".github" / "scripts"


def _production_scripts() -> set[str]:
    return {path.name for path in SCRIPTS_DIR.glob("*.py")}


def test_readme_indexes_every_production_script() -> None:
    readme = (SCRIPTS_DIR / "README.md").read_text(encoding="utf-8")
    documented = set(re.findall(r"^\| `([^`]+\.py)` \|", readme, flags=re.MULTILINE))
    assert documented == _production_scripts()


def test_every_production_script_has_a_matching_test() -> None:
    tests = {path.name for path in (SCRIPTS_DIR / "tests").glob("test_*.py")}
    missing = {script for script in _production_scripts() if f"test_{script}" not in tests}
    assert not missing
