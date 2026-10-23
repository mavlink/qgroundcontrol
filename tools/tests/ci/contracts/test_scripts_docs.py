"""Keep the CI script index and production test coverage complete."""

from __future__ import annotations

import re
from pathlib import Path

from ..._helpers import TOOLS_DIR

CI_DIR = TOOLS_DIR / "ci"
TESTS_DIR = TOOLS_DIR / "tests" / "ci"
# Tests of workflows, actions and other configuration belong in contracts/, not beside script tests.
# This workflow drift guard stays with the tools/_bootstrap.py tests it extends.
UNMAPPED_TESTS = {"test_bootstrap_sparse_checkout.py"}


def _production_scripts() -> set[str]:
    return {
        path.relative_to(CI_DIR).as_posix()
        for path in CI_DIR.rglob("*.py")
        if path.name != "__init__.py"
    }


def test_readme_indexes_every_production_script() -> None:
    readme = (CI_DIR / "README.md").read_text(encoding="utf-8")
    documented = set(re.findall(r"^\| `([^`]+\.py)` \|", readme, flags=re.MULTILINE))
    assert documented == _production_scripts()
    packages = {path.parent.name for path in CI_DIR.glob("*/__init__.py")}
    assert set(re.findall(r"^\| `([a-z_]+)/` \|", readme, flags=re.MULTILINE)) == packages


def test_every_production_script_has_a_matching_test() -> None:
    tests = {path.name for path in TESTS_DIR.glob("test_*.py")}
    missing = {
        script for script in _production_scripts() if f"test_{Path(script).name}" not in tests
    }
    assert not missing


def test_every_script_test_matches_a_production_script() -> None:
    scripts = {Path(script).name for script in _production_scripts()}
    tests = {path.name for path in TESTS_DIR.glob("test_*.py")} - UNMAPPED_TESTS
    assert not {test for test in tests if test.removeprefix("test_") not in scripts}
