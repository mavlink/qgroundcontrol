"""Common utilities for QGC developer tools — lazy facade.

Submodules are imported on first attribute access, not at package-import
time. This keeps `from common import log_info` cheap and isolates
optional third-party deps (httpx, defusedxml, etc.) from setup scripts
that run before `pip install` completes.

Submodules:
    patterns:        QGC-specific regex patterns
    file_traversal:  C++ source-tree walkers
    gh_actions:      GitHub Actions API helpers
    build_config:    build-config.json loader
    github_runs:     workflow run helpers
    logging:         colored terminal output
    errors:          ToolNotFoundError
    deps:            external-tool checks
"""

from __future__ import annotations

import importlib
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from .build_config import (
        derive_ios_qt_modules as derive_ios_qt_modules,
    )
    from .build_config import (
        export_build_config_values as export_build_config_values,
    )
    from .build_config import (
        find_build_config as find_build_config,
    )
    from .build_config import (
        get_build_config_value as get_build_config_value,
    )
    from .build_config import (
        load_build_config as load_build_config,
    )
    from .deps import (
        check_and_report as check_and_report,
    )
    from .deps import (
        check_dependencies as check_dependencies,
    )
    from .deps import (
        require_tool as require_tool,
    )
    from .env import (
        is_debug as is_debug,
    )
    from .env import (
        is_verbose as is_verbose,
    )
    from .errors import (
        ToolNotFoundError as ToolNotFoundError,
    )
    from .file_traversal import (
        DEFAULT_SKIP_DIRS as DEFAULT_SKIP_DIRS,
    )
    from .file_traversal import (
        find_cpp_files as find_cpp_files,
    )
    from .file_traversal import (
        find_header_files as find_header_files,
    )
    from .file_traversal import (
        find_json_files as find_json_files,
    )
    from .file_traversal import (
        find_repo_root as find_repo_root,
    )
    from .file_traversal import (
        find_source_files as find_source_files,
    )
    from .gh_actions import (
        append_github_env as append_github_env,
    )
    from .gh_actions import (
        gh as gh,
    )
    from .gh_actions import (
        gh_error as gh_error,
    )
    from .gh_actions import (
        gh_notice as gh_notice,
    )
    from .gh_actions import (
        gh_warning as gh_warning,
    )
    from .gh_actions import (
        list_run_artifacts as list_run_artifacts,
    )
    from .gh_actions import (
        list_workflow_runs_for_sha as list_workflow_runs_for_sha,
    )
    from .gh_actions import (
        write_github_output as write_github_output,
    )
    from .gh_actions import (
        write_step_summary as write_step_summary,
    )
    from .github_runs import (
        group_runs_by_name as group_runs_by_name,
    )
    from .github_runs import (
        is_newer_run as is_newer_run,
    )
    from .github_runs import (
        parse_created_at as parse_created_at,
    )
    from .github_runs import (
        select_latest_runs_by_name as select_latest_runs_by_name,
    )
    from .logging import (
        Color as Color,
    )
    from .logging import (
        Logger as Logger,
    )
    from .logging import (
        colorize as colorize,
    )
    from .logging import (
        log_command as log_command,
    )
    from .logging import (
        log_debug as log_debug,
    )
    from .logging import (
        log_error as log_error,
    )
    from .logging import (
        log_info as log_info,
    )
    from .logging import (
        log_ok as log_ok,
    )
    from .logging import (
        log_step as log_step,
    )
    from .logging import (
        log_verbose as log_verbose,
    )
    from .logging import (
        log_warn as log_warn,
    )
    from .logging import (
        use_color as use_color,
    )
    from .patterns import (
        ACTIVE_VEHICLE_ASSIGN_PATTERN as ACTIVE_VEHICLE_ASSIGN_PATTERN,
    )
    from .patterns import (
        ACTIVE_VEHICLE_DIRECT_PATTERN as ACTIVE_VEHICLE_DIRECT_PATTERN,
    )
    from .patterns import (
        FACT_MEMBER_PATTERN as FACT_MEMBER_PATTERN,
    )
    from .patterns import (
        FACTGROUP_CLASS_PATTERN as FACTGROUP_CLASS_PATTERN,
    )
    from .patterns import (
        GET_PARAMETER_DIRECT_PATTERN as GET_PARAMETER_DIRECT_PATTERN,
    )
    from .patterns import (
        MAVLINK_MSG_ID_PATTERN as MAVLINK_MSG_ID_PATTERN,
    )
    from .patterns import (
        NULL_CHECK_PATTERNS as NULL_CHECK_PATTERNS,
    )
    from .patterns import (
        PARAM_NAME_PATTERN as PARAM_NAME_PATTERN,
    )


_LAZY_SYMBOLS: dict[str, str] = {
    "FACT_MEMBER_PATTERN": "patterns",
    "FACTGROUP_CLASS_PATTERN": "patterns",
    "MAVLINK_MSG_ID_PATTERN": "patterns",
    "PARAM_NAME_PATTERN": "patterns",
    "ACTIVE_VEHICLE_DIRECT_PATTERN": "patterns",
    "ACTIVE_VEHICLE_ASSIGN_PATTERN": "patterns",
    "GET_PARAMETER_DIRECT_PATTERN": "patterns",
    "NULL_CHECK_PATTERNS": "patterns",
    "find_repo_root": "file_traversal",
    "find_cpp_files": "file_traversal",
    "find_header_files": "file_traversal",
    "find_source_files": "file_traversal",
    "find_json_files": "file_traversal",
    "DEFAULT_SKIP_DIRS": "file_traversal",
    "gh": "gh_actions",
    "gh_error": "gh_actions",
    "gh_warning": "gh_actions",
    "gh_notice": "gh_actions",
    "list_workflow_runs_for_sha": "gh_actions",
    "list_run_artifacts": "gh_actions",
    "write_github_output": "gh_actions",
    "write_step_summary": "gh_actions",
    "append_github_env": "gh_actions",
    "find_build_config": "build_config",
    "load_build_config": "build_config",
    "get_build_config_value": "build_config",
    "export_build_config_values": "build_config",
    "derive_ios_qt_modules": "build_config",
    "parse_created_at": "github_runs",
    "is_newer_run": "github_runs",
    "select_latest_runs_by_name": "github_runs",
    "group_runs_by_name": "github_runs",
    "Color": "logging",
    "Logger": "logging",
    "use_color": "logging",
    "colorize": "logging",
    "log_info": "logging",
    "log_ok": "logging",
    "log_warn": "logging",
    "log_error": "logging",
    "log_debug": "logging",
    "log_verbose": "logging",
    "log_step": "logging",
    "log_command": "logging",
    "ToolNotFoundError": "errors",
    "check_dependencies": "deps",
    "require_tool": "deps",
    "check_and_report": "deps",
    "get_default_branch_ref": "git",
    "run_captured": "proc",
    "run_text": "proc",
    "atomic_write": "io",
    "read_json": "io",
    "read_toml": "io",
    "write_json": "io",
    "is_ci": "env",
    "is_debug": "env",
    "is_github_actions": "env",
    "is_pr_event": "env",
    "is_verbose": "env",
    "runner_arch": "env",
    "probe_version": "tool_version",
    "add_build_dir": "cli",
    "add_ci_flag": "cli",
    "add_dry_run": "cli",
    "add_jobs": "cli",
    "add_json_output": "cli",
    "resolve_jobs": "cli",
    "current_platform": "platform",
    "is_linux": "platform",
    "is_macos": "platform",
    "is_windows": "platform",
    "open_in_default_app": "opener",
    "format_bytes": "format",
    "format_delta_bytes": "format",
    "md_table": "markdown",
}

__all__ = [*_LAZY_SYMBOLS.keys(), "pip_install"]


def __getattr__(name: str):
    submodule = _LAZY_SYMBOLS.get(name)
    if submodule is None:
        raise AttributeError(f"module {__name__!r} has no attribute {name!r}")
    mod = importlib.import_module(f".{submodule}", __package__)
    value = getattr(mod, name)
    globals()[name] = value
    return value


def pip_install(packages: list[str], quiet: bool = True) -> None:
    """Install packages using uv (preferred) or pip.

    Targets the project .venv (on PATH in CI) rather than --system: the system
    interpreter may be externally managed (PEP 668) when no writable runner
    Python is provisioned. Falls back to --system when no .venv exists.
    """
    import shutil
    import subprocess
    import sys

    if shutil.which("uv"):
        from .file_traversal import find_repo_root

        rel = "Scripts/python.exe" if sys.platform == "win32" else "bin/python"
        venv_python = find_repo_root() / ".venv" / rel
        if venv_python.exists():
            cmd = ["uv", "pip", "install", "--python", str(venv_python), *packages]
        else:
            cmd = ["uv", "pip", "install", "--system", *packages]
    else:
        cmd = [sys.executable, "-m", "pip", "install", *packages]
        if quiet:
            cmd.append("--quiet")
    subprocess.run(cmd, check=True)
