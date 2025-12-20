"""
File traversal utilities for QGC developer tools.

Provides consistent file discovery across tools with proper
filtering of build directories and generated files.
"""

from pathlib import Path
from typing import Generator, Iterable

# Directories to skip when traversing
DEFAULT_SKIP_DIRS = frozenset({
    'build',
    'libs',
    'test',
    '.cache',
    'cpm_modules',
    '_deps',
    'node_modules',
    '.git',
    '.rcc',
})

# C++ file extensions
CPP_EXTENSIONS = frozenset({'.cc', '.cpp', '.cxx'})
HEADER_EXTENSIONS = frozenset({'.h', '.hpp', '.hxx'})
ALL_CPP_EXTENSIONS = CPP_EXTENSIONS | HEADER_EXTENSIONS


def find_repo_root(start_path: Path = None) -> Path:
    """
    Find the repository root by looking for .git directory.

    Args:
        start_path: Starting point for search. Defaults to current file.

    Returns:
        Path to repository root, or start_path if not found.
    """
    if start_path is None:
        start_path = Path(__file__).resolve()

    current = start_path if start_path.is_dir() else start_path.parent

    for parent in [current] + list(current.parents):
        if (parent / '.git').exists():
            return parent

    return start_path


def should_skip_path(path: Path, skip_dirs: Iterable[str] = None) -> bool:
    """
    Check if a path should be skipped based on directory name.

    Args:
        path: Path to check
        skip_dirs: Directory names to skip. Defaults to DEFAULT_SKIP_DIRS.

    Returns:
        True if path should be skipped.
    """
    if skip_dirs is None:
        skip_dirs = DEFAULT_SKIP_DIRS

    parts = path.parts
    return any(skip_dir in parts for skip_dir in skip_dirs)


def find_cpp_files(
    paths: Iterable[Path],
    skip_dirs: Iterable[str] = None,
) -> Generator[Path, None, None]:
    """
    Find all C++ files (.cc, .cpp, .h, .hpp, etc.) in given paths.

    Args:
        paths: Files or directories to search
        skip_dirs: Directory names to skip. Defaults to DEFAULT_SKIP_DIRS.

    Yields:
        Paths to C++ files.
    """
    for path in paths:
        if path.is_file():
            if path.suffix in ALL_CPP_EXTENSIONS:
                yield path
        elif path.is_dir():
            for file_path in path.rglob('*'):
                if file_path.is_file() and file_path.suffix in ALL_CPP_EXTENSIONS:
                    if not should_skip_path(file_path, skip_dirs):
                        yield file_path


def find_header_files(
    root: Path,
    skip_dirs: Iterable[str] = None,
) -> Generator[Path, None, None]:
    """
    Find all header files in a directory tree.

    Args:
        root: Root directory to search
        skip_dirs: Directory names to skip. Defaults to DEFAULT_SKIP_DIRS.

    Yields:
        Paths to header files.
    """
    for ext in HEADER_EXTENSIONS:
        for file_path in root.rglob(f'*{ext}'):
            if not should_skip_path(file_path, skip_dirs):
                yield file_path


def find_source_files(
    root: Path,
    skip_dirs: Iterable[str] = None,
) -> Generator[Path, None, None]:
    """
    Find all source files (.cc, .cpp, .cxx) in a directory tree.

    Args:
        root: Root directory to search
        skip_dirs: Directory names to skip. Defaults to DEFAULT_SKIP_DIRS.

    Yields:
        Paths to source files.
    """
    for ext in CPP_EXTENSIONS:
        for file_path in root.rglob(f'*{ext}'):
            if not should_skip_path(file_path, skip_dirs):
                yield file_path


def find_json_files(
    root: Path,
    pattern: str = '*Fact.json',
    skip_dirs: Iterable[str] = None,
) -> Generator[Path, None, None]:
    """
    Find JSON files matching a pattern in a directory tree.

    Args:
        root: Root directory to search
        pattern: Glob pattern for JSON files. Defaults to '*Fact.json'.
        skip_dirs: Directory names to skip. Defaults to DEFAULT_SKIP_DIRS.

    Yields:
        Paths to matching JSON files.
    """
    for file_path in root.rglob(pattern):
        if not should_skip_path(file_path, skip_dirs):
            yield file_path
