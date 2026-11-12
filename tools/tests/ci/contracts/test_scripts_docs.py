"""Keep the CI script index, script test coverage, and the tests the CI documents name current."""

from __future__ import annotations

import re
from pathlib import Path

import ci_script_index

from ..._helpers import REPO_ROOT, TOOLS_DIR

CI_DIR = TOOLS_DIR / "ci"
TESTS_DIR = TOOLS_DIR / "tests" / "ci"
# Tests of workflows, actions and other configuration belong in contracts/, not beside script tests.
# This workflow drift guard stays with the tools/_bootstrap.py tests it extends.
UNMAPPED_TESTS = {"test_bootstrap_sparse_checkout.py"}

# A test the CI documents name as `<file>` or `<file>::<function>`, by its repository path or, for a
# contract test, by its file name alone.
TEST_REFERENCE = re.compile(r"`([\w./-]*test_\w+\.py)(?:::(\w+))?`")
TEST_REFERENCE_ROOTS = (REPO_ROOT, TESTS_DIR / "contracts")


def _production_scripts() -> set[str]:
    return {
        path.relative_to(CI_DIR).as_posix()
        for path in CI_DIR.rglob("*.py")
        if path.name != "__init__.py"
    }


def test_committed_script_index_is_up_to_date() -> None:
    assert ci_script_index.main(["--check"]) == 0, (
        "run `uv run --frozen --project tools python tools/ci_script_index.py`"
    )


def test_every_script_and_package_has_a_module_docstring() -> None:
    # The index takes each purpose and scope from these docstrings.
    modules = sorted(CI_DIR.rglob("*.py"))
    packages = {path.parent for path in modules if path.name != "__init__.py"}
    assert all((package / "__init__.py").is_file() for package in packages)
    assert not [
        path.relative_to(CI_DIR).as_posix()
        for path in modules
        if path.parent != CI_DIR and not ci_script_index.summary(path)
    ]


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


def test_ci_documents_name_tests_that_exist() -> None:
    documents = [
        REPO_ROOT / ".github/ci-overview.md",
        *sorted((REPO_ROOT / ".github/ci-notes").glob("*.md")),
    ]
    references = 0
    missing = []
    for document in documents:
        for path, function in TEST_REFERENCE.findall(document.read_text(encoding="utf-8")):
            references += 1
            candidates = [root / path for root in TEST_REFERENCE_ROOTS if (root / path).is_file()]
            source = candidates[0].read_text(encoding="utf-8") if candidates else None
            if source is None or (
                function and not re.search(rf"^\s*def {function}\(", source, re.MULTILINE)
            ):
                missing.append(f"{document.name}: {path}{'::' + function if function else ''}")
    assert references
    assert not missing
