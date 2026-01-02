"""
Shared pytest fixtures for QGC tools tests.

Provides common fixtures for:
- Temporary directories and files
- Mock git repositories
- Sample source files
- Configuration files
"""

from __future__ import annotations

import json
import os
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Generator
from unittest.mock import MagicMock, patch

import pytest

# Add tools directory to path
TOOLS_DIR = Path(__file__).parent.parent
sys.path.insert(0, str(TOOLS_DIR))


# =============================================================================
# Directory Fixtures
# =============================================================================


@pytest.fixture
def temp_dir(tmp_path: Path) -> Path:
    """Provide a temporary directory for test files."""
    return tmp_path


@pytest.fixture
def mock_repo(tmp_path: Path) -> Path:
    """Create a mock git repository structure."""
    repo = tmp_path / "repo"
    repo.mkdir()

    # Create .git directory to simulate a repo
    git_dir = repo / ".git"
    git_dir.mkdir()

    # Create basic structure
    (repo / "src").mkdir()
    (repo / "build").mkdir()
    (repo / "tools").mkdir()
    (repo / "CMakeLists.txt").write_text("cmake_minimum_required(VERSION 3.21)\n")

    return repo


@pytest.fixture
def mock_repo_with_git(mock_repo: Path) -> Path:
    """Create a mock git repository with actual git initialization."""
    try:
        subprocess.run(
            ["git", "init"],
            cwd=mock_repo,
            capture_output=True,
            check=True,
        )
        subprocess.run(
            ["git", "config", "user.email", "test@test.com"],
            cwd=mock_repo,
            capture_output=True,
            check=True,
        )
        subprocess.run(
            ["git", "config", "user.name", "Test User"],
            cwd=mock_repo,
            capture_output=True,
            check=True,
        )
    except (subprocess.CalledProcessError, FileNotFoundError):
        pytest.skip("git not available")

    return mock_repo


# =============================================================================
# Sample Source Files
# =============================================================================


@pytest.fixture
def sample_cpp_file(mock_repo: Path) -> Path:
    """Create a sample C++ source file."""
    cpp_file = mock_repo / "src" / "Sample.cc"
    cpp_file.write_text(
        """\
#include "Sample.h"

#include <QDebug>

Sample::Sample(QObject* parent)
    : QObject(parent)
{
    qDebug() << "Sample created";
}

void Sample::doSomething()
{
    // TODO: Implement
}
"""
    )
    return cpp_file


@pytest.fixture
def sample_header_file(mock_repo: Path) -> Path:
    """Create a sample C++ header file."""
    header_file = mock_repo / "src" / "Sample.h"
    header_file.write_text(
        """\
#pragma once

#include <QObject>

class Sample : public QObject
{
    Q_OBJECT

public:
    explicit Sample(QObject* parent = nullptr);

    void doSomething();

signals:
    void somethingHappened();
};
"""
    )
    return header_file


@pytest.fixture
def sample_qml_file(mock_repo: Path) -> Path:
    """Create a sample QML file."""
    qml_dir = mock_repo / "src" / "QmlControls"
    qml_dir.mkdir(parents=True, exist_ok=True)
    qml_file = qml_dir / "Sample.qml"
    qml_file.write_text(
        """\
import QtQuick
import QtQuick.Controls

Item {
    id: root

    property string title: "Sample"

    Text {
        text: root.title
        anchors.centerIn: parent
    }
}
"""
    )
    return qml_file


@pytest.fixture
def sample_factgroup_file(mock_repo: Path) -> Path:
    """Create a sample FactGroup header file."""
    factgroup_file = mock_repo / "src" / "Vehicle" / "VehicleGPSFactGroup.h"
    factgroup_file.parent.mkdir(parents=True, exist_ok=True)
    factgroup_file.write_text(
        """\
#pragma once

#include "FactGroup.h"

class VehicleGPSFactGroup : public FactGroup
{
    Q_OBJECT

public:
    VehicleGPSFactGroup(QObject* parent = nullptr);

    Q_PROPERTY(Fact* lat READ lat CONSTANT)
    Q_PROPERTY(Fact* lon READ lon CONSTANT)

    Fact* lat() { return &_latFact; }
    Fact* lon() { return &_lonFact; }

private:
    Fact _latFact;
    Fact _lonFact;
};
"""
    )
    return factgroup_file


# =============================================================================
# Configuration Fixtures
# =============================================================================


@pytest.fixture
def build_config(mock_repo: Path) -> Path:
    """Create a sample build-config.json file."""
    config_dir = mock_repo / ".github"
    config_dir.mkdir(exist_ok=True)
    config_file = config_dir / "build-config.json"
    config_file.write_text(
        json.dumps(
            {
                "qt_version": "6.8.0",
                "gstreamer_version": "1.24.0",
                "ndk_version": "26.2.11394342",
                "xcode_version": "16.x",
            },
            indent=2,
        )
    )
    return config_file


@pytest.fixture
def cmake_cache(mock_repo: Path) -> Path:
    """Create a sample CMakeCache.txt file."""
    cache_file = mock_repo / "build" / "CMakeCache.txt"
    cache_file.parent.mkdir(exist_ok=True)
    cache_file.write_text(
        """\
CMAKE_BUILD_TYPE:STRING=Debug
QGC_BUILD_TESTING:BOOL=ON
QGC_ENABLE_COVERAGE:BOOL=OFF
"""
    )
    return cache_file


@pytest.fixture
def compile_commands(mock_repo: Path) -> Path:
    """Create a sample compile_commands.json file."""
    commands_file = mock_repo / "build" / "compile_commands.json"
    commands_file.parent.mkdir(exist_ok=True)
    commands_file.write_text(
        json.dumps(
            [
                {
                    "directory": str(mock_repo / "build"),
                    "command": "clang++ -c -o Sample.o ../src/Sample.cc",
                    "file": str(mock_repo / "src" / "Sample.cc"),
                }
            ],
            indent=2,
        )
    )
    return commands_file


# =============================================================================
# Mock Fixtures
# =============================================================================


@pytest.fixture
def mock_subprocess_run():
    """Mock subprocess.run for testing without executing commands."""
    with patch("subprocess.run") as mock:
        mock.return_value = MagicMock(
            returncode=0,
            stdout="",
            stderr="",
        )
        yield mock


@pytest.fixture
def mock_shutil_which():
    """Mock shutil.which to simulate tool availability."""
    with patch("shutil.which") as mock:
        mock.return_value = "/usr/bin/mock-tool"
        yield mock


@pytest.fixture
def mock_no_tools():
    """Mock shutil.which to simulate missing tools."""
    with patch("shutil.which") as mock:
        mock.return_value = None
        yield mock


# =============================================================================
# Environment Fixtures
# =============================================================================


@pytest.fixture
def clean_env():
    """Provide a clean environment without interfering variables."""
    original = os.environ.copy()
    # Remove potentially interfering variables
    for var in ["NO_COLOR", "FORCE_COLOR", "DEBUG", "VERBOSE"]:
        os.environ.pop(var, None)
    yield
    os.environ.clear()
    os.environ.update(original)


@pytest.fixture
def no_color_env():
    """Set NO_COLOR environment variable."""
    original = os.environ.get("NO_COLOR")
    os.environ["NO_COLOR"] = "1"
    yield
    if original is None:
        os.environ.pop("NO_COLOR", None)
    else:
        os.environ["NO_COLOR"] = original


@pytest.fixture
def debug_env():
    """Set DEBUG environment variable."""
    original = os.environ.get("DEBUG")
    os.environ["DEBUG"] = "1"
    yield
    if original is None:
        os.environ.pop("DEBUG", None)
    else:
        os.environ["DEBUG"] = original


# =============================================================================
# Capture Fixtures
# =============================================================================


@pytest.fixture
def capture_logs(capsys):
    """Capture stdout/stderr and return a helper function."""

    def get_output():
        captured = capsys.readouterr()
        return captured.out, captured.err

    return get_output
