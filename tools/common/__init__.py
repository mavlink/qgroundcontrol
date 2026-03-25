"""
Common utilities for QGC developer tools.

Modules:
    patterns: QGC-specific regex patterns for code analysis
    file_traversal: Utilities for walking C++ source trees
    gh_actions: Shared GitHub Actions API helpers
    logging: Colored terminal output and logging utilities
    errors: Custom exception hierarchy
"""

from .patterns import (
    FACT_MEMBER_PATTERN,
    FACTGROUP_CLASS_PATTERN,
    MAVLINK_MSG_ID_PATTERN,
    PARAM_NAME_PATTERN,
    ACTIVE_VEHICLE_DIRECT_PATTERN,
    ACTIVE_VEHICLE_ASSIGN_PATTERN,
    GET_PARAMETER_DIRECT_PATTERN,
    NULL_CHECK_PATTERNS,
)

from .file_traversal import (
    find_repo_root,
    find_cpp_files,
    find_header_files,
    find_source_files,
    find_json_files,
    DEFAULT_SKIP_DIRS,
)

from .gh_actions import (
    gh,
    parse_json_documents,
    list_workflow_runs_for_sha,
    list_run_artifacts,
    write_github_output,
)

from .build_config import (
    find_build_config,
    load_build_config,
    get_build_config_value,
    export_build_config_values,
    derive_ios_qt_modules,
)

from .github_runs import (
    parse_created_at,
    is_newer_run,
    select_latest_runs_by_name,
    group_runs_by_name,
)

from .logging import (
    Color,
    Colors,
    Logger,
    use_color,
    colorize,
    is_debug,
    is_verbose,
    log_info,
    log_ok,
    log_warn,
    log_error,
    log_debug,
    log_verbose,
    log_step,
    log_command,
)

from .errors import (
    QGCToolError,
    ConfigError,
    BuildError,
    ToolNotFoundError,
    ValidationError,
    GitError,
    SubprocessError,
    FileError,
    TestError,
    Result,
)

from .deps import (
    check_dependencies,
    require_tool,
    check_and_report,
)

__all__ = [
    # Patterns
    'FACT_MEMBER_PATTERN',
    'FACTGROUP_CLASS_PATTERN',
    'MAVLINK_MSG_ID_PATTERN',
    'PARAM_NAME_PATTERN',
    'ACTIVE_VEHICLE_DIRECT_PATTERN',
    'ACTIVE_VEHICLE_ASSIGN_PATTERN',
    'GET_PARAMETER_DIRECT_PATTERN',
    'NULL_CHECK_PATTERNS',
    # File traversal
    'find_repo_root',
    'find_cpp_files',
    'find_header_files',
    'find_source_files',
    'find_json_files',
    'DEFAULT_SKIP_DIRS',
    # GitHub Actions
    'gh',
    'parse_json_documents',
    'list_workflow_runs_for_sha',
    'list_run_artifacts',
    'write_github_output',
    'find_build_config',
    'load_build_config',
    'get_build_config_value',
    'export_build_config_values',
    'derive_ios_qt_modules',
    'parse_created_at',
    'is_newer_run',
    'select_latest_runs_by_name',
    'group_runs_by_name',
    # Logging
    'Color',
    'Colors',
    'Logger',
    'use_color',
    'colorize',
    'is_debug',
    'is_verbose',
    'log_info',
    'log_ok',
    'log_warn',
    'log_error',
    'log_debug',
    'log_verbose',
    'log_step',
    'log_command',
    # Errors
    'QGCToolError',
    'ConfigError',
    'BuildError',
    'ToolNotFoundError',
    'ValidationError',
    'GitError',
    'SubprocessError',
    'FileError',
    'TestError',
    'Result',
    # Dependencies
    'check_dependencies',
    'require_tool',
    'check_and_report',
    # Utilities
    'pip_install',
]


def pip_install(packages: list[str], quiet: bool = True) -> None:
    """Install packages using uv (preferred) or pip."""
    import shutil
    import subprocess
    import sys

    if shutil.which("uv"):
        cmd = ["uv", "pip", "install", "--system"] + packages
    else:
        cmd = [sys.executable, "-m", "pip", "install"] + packages
        if quiet:
            cmd.append("--quiet")
    subprocess.run(cmd, check=True)
