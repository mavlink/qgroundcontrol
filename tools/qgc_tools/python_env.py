"""Resolve and synchronize the locked Python environment used by QGC tools."""

from __future__ import annotations

import importlib.metadata
import os
import shutil
import subprocess
import sys
from pathlib import Path

from common.io import read_toml
from common.proc import run_checked_with_retry


def project_path() -> Path:
    override = os.environ.get("QGC_TOOLS_PROJECT")
    return Path(override).resolve() if override else Path(__file__).resolve().parents[1]


def environment_path() -> Path:
    override = os.environ.get("QGC_PYTHON_ENV")
    return Path(override).resolve() if override else project_path() / ".venv"


def executable(name: str, environment: Path | None = None) -> Path:
    environment = environment or environment_path()
    if sys.platform == "win32":
        return environment / "Scripts" / f"{name}.exe"
    return environment / "bin" / name


def require_uv() -> str:
    uv = shutil.which("uv")
    if not uv:
        raise FileNotFoundError(
            "uv is required. Install it from https://docs.astral.sh/uv/getting-started/installation/"
        )
    return uv


def package_groups() -> dict[str, list[str]]:
    definitions = read_toml(project_path() / "pyproject.toml")["dependency-groups"]
    groups: dict[str, list[str]] = {}

    def resolve(name: str, visiting: frozenset[str] = frozenset()) -> list[str]:
        if name in visiting:
            raise ValueError(f"Circular dependency group: {name}")
        if name not in definitions:
            raise ValueError(f"Unknown dependency group: {name}")
        if name not in groups:
            packages: set[str] = set()
            for entry in definitions[name]:
                if isinstance(entry, str):
                    packages.add(entry)
                else:
                    packages.update(resolve(entry["include-group"], visiting | {name}))
            groups[name] = sorted(packages)
        return groups[name]

    for name in definitions:
        resolve(name)
    groups["all"] = sorted({p for packages in groups.values() for p in packages})
    return groups


def requirements_for(group_spec: str) -> list[str]:
    groups = package_groups()
    selected = [name.strip() for name in group_spec.split(",") if name.strip()]
    if not selected or any(name not in groups for name in selected):
        raise ValueError(f"Unknown group: {group_spec!r}. Valid groups: {', '.join(groups)}")
    return sorted({requirement for name in selected for requirement in groups[name]})


def check_requirements(requirements: list[str]) -> int:
    try:
        from packaging.requirements import Requirement
    except ImportError:
        print("Missing packaging; synchronize the scripts group first", file=sys.stderr)
        return 1
    failures = []
    for value in requirements:
        requirement = Requirement(value)
        if requirement.marker and not requirement.marker.evaluate():
            continue
        try:
            installed = importlib.metadata.version(requirement.name)
        except importlib.metadata.PackageNotFoundError:
            failures.append(f"missing {requirement.name}")
        else:
            if installed not in requirement.specifier:
                failures.append(
                    f"{requirement.name}=={installed} does not satisfy {requirement.specifier}"
                )
    if failures:
        print("Invalid Python environment: " + "; ".join(failures), file=sys.stderr)
    return int(bool(failures))


def sync_groups(
    group_spec: str,
    *,
    environment: Path | None = None,
    replace: bool = False,
    dry_run: bool = False,
    python: str | None = None,
) -> Path:
    """Add locked groups; removal requires an explicit profile replacement."""
    target = (environment or environment_path()).resolve()
    interpreter = executable("python", target)
    command = [
        require_uv(),
        "sync",
        "--project",
        str(project_path()),
        "--frozen",
        "--no-default-groups",
        "--no-install-project",
        "--python",
        python or (str(interpreter) if interpreter.exists() else sys.executable),
    ]
    if not replace:
        command.append("--inexact")
    if dry_run:
        command.append("--dry-run")
    names = [name.strip() for name in group_spec.split(",") if name.strip()]
    if not names:
        raise ValueError("At least one dependency group is required")
    if "all" in names:
        command.append("--all-groups")
    else:
        for name in names:
            command.extend(["--group", name])
    env = os.environ.copy()
    env.pop("VIRTUAL_ENV", None)
    env["UV_PROJECT_ENVIRONMENT"] = str(target)
    run_checked_with_retry(command, env=env)
    return target


def tool_command(name: str, group: str, *, source: str = "") -> list[str]:
    """Use the locked tool, or an explicitly isolated caller-validated override."""
    if source:
        return [require_uv(), "tool", "run", "--isolated", "--from", source, name]
    target = sync_groups(group)
    tool = executable(name, target)
    if not tool.is_file():
        raise FileNotFoundError(f"{name} is missing after syncing {group}: {tool}")
    return [str(tool)]


def run_tool(name: str, group: str, args: list[str]) -> int:
    return subprocess.run([*tool_command(name, group), *args], check=False).returncode
