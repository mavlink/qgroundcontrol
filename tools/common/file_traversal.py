"""
File traversal utilities for QGC developer tools.

Provides consistent file discovery across tools with proper
filtering of build directories and generated files.
"""

from collections.abc import Generator, Iterable
from pathlib import Path

# Directories to skip when traversing
DEFAULT_SKIP_DIRS = frozenset(
    {
        "build",
        "libs",
        "test",
        ".cache",
        ".ccache",
        "cpm_modules",
        "_deps",
        "node_modules",
        ".git",
        ".rcc",
    }
)

# C++ file extensions
CPP_EXTENSIONS = frozenset({".cc", ".cpp", ".cxx"})
HEADER_EXTENSIONS = frozenset({".h", ".hpp", ".hxx"})
ALL_CPP_EXTENSIONS = CPP_EXTENSIONS | HEADER_EXTENSIONS


def find_repo_root(start_path: Path | None = None) -> Path:
    """
    Find the repository root by looking for .git directory.

    Args:
        start_path: Starting point for search. Defaults to current file.

    Returns:
        Path to the repository root.

    Raises:
        RuntimeError: If no ``.git`` marker exists at or above the start path.
    """
    if start_path is None:
        start_path = Path(__file__).resolve()

    current = start_path if start_path.is_dir() else start_path.parent

    for parent in [current, *current.parents]:
        if (parent / ".git").exists():
            return parent

    raise RuntimeError(f"Could not find repository root from {start_path}")


def should_skip_path(path: Path, skip_dirs: Iterable[str] | None = None) -> bool:
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
    skip_dirs: Iterable[str] | None = None,
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
            for file_path in path.rglob("*"):
                if (
                    file_path.is_file()
                    and file_path.suffix in ALL_CPP_EXTENSIONS
                    and not should_skip_path(file_path, skip_dirs)
                ):
                    yield file_path
