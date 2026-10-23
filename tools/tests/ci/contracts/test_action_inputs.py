"""Local composite action callers pass only declared inputs, and leave presets and build-config
to supply the build type, testing switch, and Qt version."""

from __future__ import annotations

from typing import TYPE_CHECKING, Any

from ..._helpers import REPO_ROOT, action_names, get_steps, load_action, load_yaml, workflow_names

if TYPE_CHECKING:
    from collections.abc import Iterator

LOCAL_ACTION = "./.github/actions/"


def _local_action_steps() -> Iterator[tuple[str, str, dict[str, Any]]]:
    """Yield (caller, action name, inputs) for every step that uses a local composite action."""
    workflows = [REPO_ROOT / ".github/workflows" / name for name in workflow_names()]
    actions = [REPO_ROOT / ".github/actions" / name / "action.yml" for name in action_names()]
    for path in [*workflows, *actions]:
        document = load_yaml(path)
        if "jobs" in document:
            steps = [step for job in document["jobs"].values() for step in job.get("steps") or []]
        else:
            steps = get_steps(document) if document["runs"]["using"] == "composite" else []
        for step in steps:
            uses = step.get("uses", "")
            if uses.startswith(LOCAL_ACTION):
                caller = f"{path.relative_to(REPO_ROOT)}:{step.get('name') or step.get('id', uses)}"
                yield caller, uses.removeprefix(LOCAL_ACTION), step.get("with") or {}


def test_local_action_callers_pass_only_declared_and_all_required_inputs() -> None:
    problems = []
    for caller, name, options in _local_action_steps():
        declared = load_action(name).get("inputs") or {}
        required = {
            key
            for key, spec in declared.items()
            if spec.get("required") is True and "default" not in spec
        }
        if unknown := sorted(set(options) - set(declared)):
            problems.append(f"{caller} passes undeclared {name} inputs {unknown}")
        if missing := sorted(required - set(options)):
            problems.append(f"{caller} omits required {name} inputs {missing}")
    assert not problems


def test_presets_select_the_build_type_and_testing() -> None:
    # Every CI configure passes a preset, which sets CMAKE_BUILD_TYPE and QGC_BUILD_TESTING;
    # cmake_helper.py reads a multi-config tree's build type from CMakeCache.txt.
    configure = load_action("cmake-configure")["inputs"]
    assert configure["preset"]["required"] is True
    assert not {"build-type", "testing"} & set(configure)
    for name in ("cmake-build", "run-unit-tests"):
        assert "build-type" not in load_action(name)["inputs"]


def test_qt_install_takes_the_build_config_version() -> None:
    # install_qt.py plan defaults --version to build-config.json's qt.version.
    action = load_action("qt-install")
    assert "version" not in action["inputs"]
    plan = next(step for step in get_steps(action) if step.get("id") == "plan")
    assert "--version" not in plan["run"]
    assert [name for _, name, _ in _local_action_steps()].count("qt-install") >= 3
