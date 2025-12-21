"""
Common utilities for QGC developer tools.

Modules:
    patterns: QGC-specific regex patterns for code analysis
    file_traversal: Utilities for walking C++ source trees
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
]
