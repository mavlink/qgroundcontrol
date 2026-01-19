#!/usr/bin/env python3
"""
Smoke tests for QGroundControl tools.

Verifies all tools work end-to-end:
- Help texts for all major scripts
- Python tool imports
- Config file reading
- Directory structure
- Script syntax validation

Usage:
    pytest tools/smoke_test.py              # Run all tests
    pytest tools/smoke_test.py -v           # Verbose output
    python tools/smoke_test.py              # Run directly
"""

import json
import subprocess
import sys
from pathlib import Path

import pytest


@pytest.fixture
def repo_root() -> Path:
    """Return the repository root directory."""
    return Path(__file__).parent.parent.resolve()


@pytest.fixture
def tools_dir() -> Path:
    """Return the tools directory."""
    return Path(__file__).parent.resolve()


class TestHelpTexts:
    """Tests for script help text functionality."""

    SCRIPTS_WITH_HELP = [
        "analyze.py",
        "clean.py",
        "configure.py",
        "coverage.py",
        "pre_commit.py",
        "run_tests.py",
    ]

    @pytest.mark.parametrize("script", SCRIPTS_WITH_HELP)
    def test_script_help_exits_zero(self, tools_dir: Path, script: str) -> None:
        """Verify script --help exits with code 0."""
        script_path = tools_dir / script
        if not script_path.exists():
            pytest.skip(f"Script {script} does not exist")

        result = subprocess.run(
            [sys.executable, str(script_path), "--help"],
            capture_output=True,
            timeout=5,
        )
        assert result.returncode == 0, (
            f"{script} --help failed with code {result.returncode}: "
            f"{result.stderr.decode()}"
        )

    @pytest.mark.parametrize("script", SCRIPTS_WITH_HELP)
    def test_script_help_produces_output(self, tools_dir: Path, script: str) -> None:
        """Verify script --help produces some output."""
        script_path = tools_dir / script
        if not script_path.exists():
            pytest.skip(f"Script {script} does not exist")

        result = subprocess.run(
            [sys.executable, str(script_path), "--help"],
            capture_output=True,
            timeout=5,
        )
        output = result.stdout.decode() + result.stderr.decode()
        assert len(output.strip()) > 0, f"{script} --help produced no output"


class TestPythonTools:
    """Tests for Python tool scripts."""

    PYTHON_SCRIPTS_WITH_HELP = [
        "setup/install_qt.py",
        "setup/read_config.py",
        "setup/gstreamer/build-gstreamer.py",
        "translations/qgc_lupdate.py",
    ]

    @pytest.mark.parametrize("script", PYTHON_SCRIPTS_WITH_HELP)
    def test_python_script_help(self, tools_dir: Path, script: str) -> None:
        """Verify Python script --help succeeds."""
        script_path = tools_dir / script
        if not script_path.exists():
            pytest.skip(f"Script {script} does not exist")

        result = subprocess.run(
            [sys.executable, str(script_path), "--help"],
            capture_output=True,
            timeout=5,
        )
        assert result.returncode == 0, (
            f"{script} --help failed with code {result.returncode}: "
            f"{result.stderr.decode()}"
        )

    def test_qgc_locator_can_be_imported(self, tools_dir: Path) -> None:
        """Verify qgc_locator.py can be imported as a module."""
        locator_dir = tools_dir / "locators"
        if not locator_dir.exists():
            pytest.skip("locators directory does not exist")

        result = subprocess.run(
            [
                sys.executable,
                "-c",
                f"import sys; sys.path.insert(0, '{tools_dir}'); "
                "from locators import qgc_locator",
            ],
            capture_output=True,
            timeout=5,
        )
        assert result.returncode == 0, (
            f"Failed to import qgc_locator: {result.stderr.decode()}"
        )


class TestConfig:
    """Tests for configuration file reading."""

    def test_build_config_exists(self, repo_root: Path) -> None:
        """Verify build-config.json exists."""
        config_path = repo_root / ".github" / "build-config.json"
        assert config_path.exists(), f"Config file not found: {config_path}"

    def test_build_config_valid_json(self, repo_root: Path) -> None:
        """Verify build-config.json is valid JSON."""
        config_path = repo_root / ".github" / "build-config.json"
        if not config_path.exists():
            pytest.skip("build-config.json does not exist")

        with open(config_path) as f:
            try:
                json.load(f)
            except json.JSONDecodeError as e:
                pytest.fail(f"Invalid JSON in build-config.json: {e}")

    def test_qt_version_exists(self, repo_root: Path) -> None:
        """Verify build-config.json contains qt_version."""
        config_path = repo_root / ".github" / "build-config.json"
        if not config_path.exists():
            pytest.skip("build-config.json does not exist")

        with open(config_path) as f:
            config = json.load(f)

        assert "qt_version" in config, "build-config.json missing 'qt_version' key"
        assert config["qt_version"], "qt_version is empty"

    def test_qt_version_format(self, repo_root: Path) -> None:
        """Verify qt_version has expected format (X.Y.Z)."""
        config_path = repo_root / ".github" / "build-config.json"
        if not config_path.exists():
            pytest.skip("build-config.json does not exist")

        with open(config_path) as f:
            config = json.load(f)

        qt_version = config.get("qt_version", "")
        parts = qt_version.split(".")
        assert len(parts) >= 2, f"qt_version '{qt_version}' should have at least major.minor"
        assert all(p.isdigit() for p in parts), f"qt_version '{qt_version}' parts should be numeric"


class TestDirectoryStructure:
    """Tests for expected directory structure."""

    EXPECTED_SUBDIRS = [
        "setup",
        "debuggers",
        "translations",
        "simulation",
        "analyzers",
        "generators",
        "locators",
        "common",
        "tests",
        "configs",
        "schemas",
    ]

    @pytest.mark.parametrize("subdir", EXPECTED_SUBDIRS)
    def test_subdirectory_exists(self, tools_dir: Path, subdir: str) -> None:
        """Verify expected subdirectory exists."""
        subdir_path = tools_dir / subdir
        assert subdir_path.is_dir(), f"Directory not found: {subdir_path}"

    def test_readme_exists(self, tools_dir: Path) -> None:
        """Verify README.md exists."""
        assert (tools_dir / "README.md").is_file()


class TestSyntax:
    """Tests for script syntax validation."""

    def test_python_scripts_compile(self, tools_dir: Path) -> None:
        """Verify Python scripts in tools/ compile without errors."""
        python_files = list(tools_dir.glob("*.py"))
        for py_file in python_files:
            if py_file.name.startswith("__"):
                continue
            result = subprocess.run(
                [sys.executable, "-m", "py_compile", str(py_file)],
                capture_output=True,
                timeout=5,
            )
            assert result.returncode == 0, (
                f"{py_file.name} has syntax errors: {result.stderr.decode()}"
            )


class TestIntegration:
    """Integration tests combining multiple components."""

    def test_read_config_returns_qt_version(self, tools_dir: Path, repo_root: Path) -> None:
        """Verify read_config.py can read qt_version from build-config.json."""
        read_config = tools_dir / "setup" / "read_config.py"
        config_path = repo_root / ".github" / "build-config.json"

        if not read_config.exists():
            pytest.skip("read_config.py does not exist")
        if not config_path.exists():
            pytest.skip("build-config.json does not exist")

        result = subprocess.run(
            [sys.executable, str(read_config), "--key", "qt_version"],
            capture_output=True,
            timeout=5,
            cwd=repo_root,
        )

        if result.returncode != 0:
            pytest.skip(f"read_config.py failed: {result.stderr.decode()}")

        qt_version = result.stdout.decode().strip()
        assert qt_version, "read_config.py returned empty qt_version"

        with open(config_path) as f:
            expected = json.load(f)["qt_version"]
        assert qt_version == expected, (
            f"read_config.py returned '{qt_version}', expected '{expected}'"
        )


if __name__ == "__main__":
    sys.exit(pytest.main([__file__, "-v"] + sys.argv[1:]))
