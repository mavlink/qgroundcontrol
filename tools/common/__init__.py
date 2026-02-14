"""
Common utilities for QGC developer tools.

Modules:
    patterns: QGC-specific regex patterns for code analysis
    file_traversal: Utilities for walking C++ source trees
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
]
