"""Local composite actions and their callers agree on inputs, and actions on their own step ids."""

from __future__ import annotations

import json
import re

from ..._helpers import action_names, get_steps, load_action
from .._workflows import local_action, step_groups


def test_local_action_callers_pass_only_declared_and_all_required_inputs() -> None:
    # actionlint checks the workflows' calls; only this covers one composite action calling another.
    problems = []
    for label, _, steps in step_groups():
        for step in steps:
            name = local_action(step)
            if name is None:
                continue
            options = step.get("with") or {}
            declared = load_action(name).get("inputs") or {}
            required = {
                key
                for key, spec in declared.items()
                if spec.get("required") is True and "default" not in spec
            }
            if unknown := sorted(set(options) - set(declared)):
                problems.append(f"{label} passes undeclared {name} inputs {unknown}")
            if missing := sorted(required - set(options)):
                problems.append(f"{label} omits required {name} inputs {missing}")
    assert not problems


def test_composite_actions_read_exactly_their_declared_inputs_and_their_own_steps() -> None:
    # actionlint does not read action.yml: an undeclared input or step id expands to an empty
    # string, and a declared input that no step reads is silently ignored.
    problems = []
    for name in action_names():
        action = load_action(name)
        if action["runs"]["using"] != "composite":
            continue
        expressions = json.dumps([action["runs"], action.get("outputs")])
        declared = set(action.get("inputs") or {})
        read = set(re.findall(r"\binputs\.([\w-]+)", expressions))
        if declared != read:
            problems.append(
                f"{name}: unread {sorted(declared - read)}, undeclared {sorted(read - declared)}"
            )
        step_ids = {step["id"] for step in get_steps(action) if "id" in step}
        if unknown := sorted(set(re.findall(r"\bsteps\.([\w-]+)\.", expressions)) - step_ids):
            problems.append(f"{name}: no step has the id {unknown}")
    assert not problems
