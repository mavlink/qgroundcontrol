"""Walk the steps of every workflow job and composite action, for repository-wide contract sweeps."""

from __future__ import annotations

import fnmatch
import re
from typing import TYPE_CHECKING, Any

from .._helpers import (
    REPO_ROOT,
    action_names,
    get_steps,
    load_action,
    load_workflow,
    load_yaml,
    workflow_names,
)

if TYPE_CHECKING:
    from collections.abc import Iterable, Iterator

LOCAL_ACTION = "./.github/actions/"
_USES_LINE = re.compile(
    r"^[ \t]*(?:-[ \t]+)?uses:[ \t]+(?P<uses>[^\s#]+)(?:[ \t]+#[ \t]*(?P<comment>\S.*?))?[ \t]*$",
    re.MULTILINE,
)


def step_groups(
    *, workflows: bool = True, actions: bool = True
) -> Iterator[tuple[str, dict[str, Any], list[dict[str, Any]]]]:
    """Yield (label, owner, steps) for each workflow job and each local action.

    The label is `<workflow file>:<job id>` or `actions/<name>`; the owner is the job mapping or the
    action document.
    """
    if workflows:
        for name in workflow_names():
            for job_id, job in load_workflow(name)["jobs"].items():
                yield f"{name}:{job_id}", job, job.get("steps") or []
    if actions:
        for name in action_names():
            action = load_action(name)
            yield f"actions/{name}", action, get_steps(action)


def uses_action(step: dict[str, Any], name: str) -> bool:
    """Whether *step* runs the action *name* (`owner/repo[/path]` or a `./` path) at any version."""
    return str(step.get("uses", "")).partition("@")[0] == name


def uses_lines(text: str) -> list[tuple[str, str]]:
    """Each `uses:` value in a workflow's or action's text, with its trailing comment ("" if none).

    The comment holds the version of an action pinned to a commit SHA, and YAML parsing drops it.
    """
    return [(match["uses"], match["comment"] or "") for match in _USES_LINE.finditer(text)]


def uses_references() -> Iterator[tuple[str, dict[str, Any], list[tuple[str, str]]]]:
    """Yield (label, document, `uses_lines()` of its text) for each workflow and local action.

    The label is `<workflow file>` or `actions/<name>`.
    """
    files = [(name, f".github/workflows/{name}") for name in workflow_names()]
    files += [(f"actions/{name}", f".github/actions/{name}/action.yml") for name in action_names()]
    for label, path in files:
        yield label, load_yaml(path), uses_lines((REPO_ROOT / path).read_text(encoding="utf-8"))


def local_action(step: dict[str, Any]) -> str | None:
    """Name of the local composite action *step* runs, if any."""
    uses = str(step.get("uses", ""))
    return uses.removeprefix(LOCAL_ACTION).strip("/") if uses.startswith(LOCAL_ACTION) else None


def path_filter_covers(path: str, patterns: Iterable[str]) -> bool:
    """Whether a trigger's `paths` patterns match the file *path*, or everything in that directory."""
    return any(
        pattern in (path, f"{path}/**") or fnmatch.fnmatchcase(path, pattern)
        for pattern in patterns
    )
