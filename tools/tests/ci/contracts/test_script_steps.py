"""Steps set the INPUT_* variables their Python scripts read, and stop at a failed CTest run."""

from __future__ import annotations

import ast
import functools
import re
from typing import TYPE_CHECKING, Any

from ..._helpers import REPO_ROOT, get_steps, load_action
from .._workflows import local_action, step_groups
from ..test_bootstrap_sparse_checkout import NAMED_FILE

if TYPE_CHECKING:
    from collections.abc import Iterable

INPUT_NAME = re.compile(r"INPUT_[A-Z0-9_]+")
CMAKE_HELPER = "tools/ci/compile/cmake_helper.py"
# What may continue after a failed CTest run, as (label, step name), or (label, None) for a job
# that continues on error as a whole: none.
MAY_IGNORE_CTEST_FAILURE: frozenset[tuple[str, str | None]] = frozenset()


def _script_calls(step: dict[str, Any]) -> list[tuple[str, list[str]]]:
    """(script, the arguments after it on its line) for each Python script *step* names."""
    texts = [step.get("run"), *(step.get("with") or {}).values()]
    return [
        (match[1], text[match.end() :].partition("\n")[0].lstrip('"').split())
        for text in texts
        if isinstance(text, str)
        for match in NAMED_FILE.finditer(text)
        if match[1].endswith(".py")
    ]


def _input_names(node: ast.AST) -> dict[ast.Constant, str]:
    """The INPUT_* name of each string literal under *node* that is exactly one."""
    return {
        child: child.value
        for child in ast.walk(node)
        if isinstance(child, ast.Constant)
        and isinstance(child.value, str)
        and INPUT_NAME.fullmatch(child.value)
    }


def _name(node: ast.AST) -> str:
    """The identifier *node* is, or "" for any other expression."""
    return node.id if isinstance(node, ast.Name) else ""


@functools.cache
def _inputs_read(script: str) -> dict[str | None, set[str]]:
    """INPUT_* names *script* reads, by argparse sub-command; None has those every command reads.

    A name is a whole string literal, as passed to `env_default()`, `env_flag()` or an environment
    mapping. One inside `<parser>.add_argument()` belongs to the sub-commands of <parser>: a
    variable assigned `add_parser("<command>")`, or looping over a tuple of such variables. Any
    other name counts for every command, which errs towards failing for a script that builds its
    sub-parsers another way.
    """
    tree = ast.parse((REPO_ROOT / script).read_text(encoding="utf-8"))
    read: dict[str | None, set[str]] = {None: set()}
    parsers: dict[str, set[str]] = {}
    scoped: set[ast.Constant] = set()
    nodes = [node for node in ast.walk(tree) if isinstance(node, ast.Assign | ast.For | ast.Call)]
    for node in sorted(nodes, key=lambda node: (node.lineno, node.col_offset)):
        match node:
            case ast.Call(
                func=ast.Attribute(attr="add_parser"), args=[ast.Constant(value=str(command)), *_]
            ):
                read.setdefault(command, set())
            case ast.Assign(
                targets=[ast.Name(id=variable)],
                value=ast.Call(
                    func=ast.Attribute(attr="add_parser"),
                    args=[ast.Constant(value=str(command)), *_],
                ),
            ):
                parsers[variable] = {command}
            case ast.For(target=ast.Name(id=variable), iter=ast.Tuple(elts=elements)) if all(
                _name(element) in parsers for element in elements
            ):
                parsers[variable] = set().union(*(parsers[_name(element)] for element in elements))
            case (
                ast.Assign(targets=[ast.Name(id=variable)]) | ast.For(target=ast.Name(id=variable))
            ):
                parsers.pop(variable, None)
            case ast.Call(func=ast.Attribute(value=ast.Name(id=variable), attr="add_argument")) if (
                variable in parsers
            ):
                names = _input_names(node)
                scoped.update(names)
                for command in parsers[variable]:
                    read.setdefault(command, set()).update(names.values())
    read[None] = {name for literal, name in _input_names(tree).items() if literal not in scoped}
    return read


def test_steps_set_exactly_the_input_variables_their_scripts_read() -> None:
    # A script reads its default for a variable the step misspells or omits, and nothing fails:
    # with INPUT_EXCLUDE_LABELS empty, every lane would run the Flaky and Network tests.
    problems = []
    setting = set()
    for label, _, steps in step_groups():
        for step in steps:
            calls = _script_calls(step)
            if not calls:
                continue
            allowed: set[str] = set()
            required: set[str] = set()
            for script, arguments in calls:
                by_command = _inputs_read(script)
                command = arguments[0] if arguments and arguments[0] in by_command else None
                read = by_command[None] | by_command[command]
                allowed |= read
                # Arguments after the sub-command may give the values instead.
                if not (arguments[1:] if command else arguments):
                    required |= read
            passed = {name for name in step.get("env") or {} if INPUT_NAME.fullmatch(name)}
            if passed:
                setting.add(label)
            where = f"{label} ({step.get('name')})"
            if unread := sorted(passed - allowed):
                problems.append(f"{where} sets {unread}, which its script does not read")
            if unset := sorted(required - passed):
                problems.append(f"{where} does not set {unset}, which its script reads")
    assert {"actions/cache", "actions/run-unit-tests"} <= setting
    assert not problems


def _runs_ctest(step: dict[str, Any]) -> bool:
    """Whether *step* runs `cmake_helper.py ctest`, itself or through the local action it uses."""
    if any(
        script == CMAKE_HELPER and arguments[:1] == ["ctest"]
        for script, arguments in _script_calls(step)
    ):
        return True
    name = local_action(step)
    return name is not None and any(map(_runs_ctest, get_steps(load_action(name))))


def _ignored_ctest_failures(
    groups: Iterable[tuple[str, dict[str, Any], list[dict[str, Any]]]],
) -> set[tuple[str, str | None]]:
    """Where a failed CTest run would not fail its job, among `step_groups()` entries."""
    ignored: set[tuple[str, str | None]] = set()
    for label, owner, steps in groups:
        ctest_steps = [step for step in steps if _runs_ctest(step)]
        ignored.update(
            (label, step.get("name")) for step in ctest_steps if "continue-on-error" in step
        )
        # Only a job can have the key; an action's document cannot.
        if ctest_steps and "continue-on-error" in owner:
            ignored.add((label, None))
    return ignored


def test_no_step_or_job_continues_after_a_failed_ctest_run() -> None:
    # CTest's exit code is all that fails a job whose tests fail: `continue-on-error` on its
    # step, on a step that uses its action, or on the job would report every test failure as
    # success.
    groups = list(step_groups())
    assert any(
        label == "actions/run-unit-tests" and any(map(_runs_ctest, steps))
        for label, _, steps in groups
    )
    assert _ignored_ctest_failures(groups) == MAY_IGNORE_CTEST_FAILURE


def test_the_ctest_sweep_sees_a_step_and_a_job_that_continue_on_error() -> None:
    ctest = {"name": "Test", "run": f'python3 "${{GITHUB_WORKSPACE}}/{CMAKE_HELPER}" ctest'}
    build = {"name": "Build", "run": f"python3 {CMAKE_HELPER} build", "continue-on-error": True}
    through_action = {"name": "Tests", "uses": "./.github/actions/run-unit-tests"}
    groups = [
        ("clean.yml:test", {}, [build, ctest]),
        ("step.yml:test", {}, [{**ctest, "continue-on-error": True}]),
        ("job.yml:test", {"continue-on-error": True}, [build, ctest]),
        ("job.yml:build", {"continue-on-error": True}, [build]),
        ("action.yml:test", {}, [{**through_action, "continue-on-error": True}]),
    ]
    assert _ignored_ctest_failures(groups) == {
        ("step.yml:test", "Test"),
        ("job.yml:test", None),
        ("action.yml:test", "Tests"),
    }
