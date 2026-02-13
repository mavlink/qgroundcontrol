#!/usr/bin/env python3
"""Tests for install-python.py."""

import subprocess
import sys
from pathlib import Path
from unittest.mock import MagicMock, patch

import pytest

# Add parent directory to path for imports
sys.path.insert(0, str(Path(__file__).parent.parent / "setup"))


class TestPackageGroups:
    """Tests for package group definitions."""

    def test_groups_defined(self) -> None:
        """Verify all expected groups are defined."""
        from install_python import PACKAGE_GROUPS

        expected = {"ci", "qt", "coverage", "dev", "lsp", "all"}
        assert expected.issubset(set(PACKAGE_GROUPS.keys()))

    def test_ci_group_has_packages(self) -> None:
        """Verify CI group contains essential packages."""
        from install_python import PACKAGE_GROUPS

        ci_packages = PACKAGE_GROUPS["ci"]
        assert len(ci_packages) > 0

    def test_all_group_includes_others(self) -> None:
        """Verify 'all' group aggregates other groups."""
        from install_python import PACKAGE_GROUPS

        all_packages = set(PACKAGE_GROUPS["all"])
        ci_packages = set(PACKAGE_GROUPS["ci"])
        assert ci_packages.issubset(all_packages)


class TestToolDetection:
    """Tests for package manager detection."""

    def test_has_uv_when_present(self) -> None:
        """Verify uv detection when available."""
        from install_python import has_uv

        with patch("shutil.which", return_value="/usr/bin/uv"):
            assert has_uv() is True

    def test_has_uv_when_absent(self) -> None:
        """Verify uv detection when not available."""
        from install_python import has_uv

        with patch("shutil.which", return_value=None):
            assert has_uv() is False


class TestVenvCreation:
    """Tests for virtual environment creation."""

    def test_get_venv_path(self) -> None:
        """Verify venv path is in repo root."""
        from install_python import get_venv_path

        venv_path = get_venv_path()
        assert venv_path.name == ".venv"

    def test_get_activate_script_unix(self) -> None:
        """Verify Unix activate script path."""
        from install_python import get_activate_script

        with patch("sys.platform", "linux"):
            venv = Path("/repo/.venv")
            script = get_activate_script(venv)
            assert script == venv / "bin" / "activate"

    def test_get_activate_script_windows(self) -> None:
        """Verify Windows activate script path."""
        from install_python import get_activate_script

        with patch("sys.platform", "win32"):
            venv = Path("C:/repo/.venv")
            script = get_activate_script(venv)
            assert script == venv / "Scripts" / "activate.bat"

    def test_create_venv_with_uv(self) -> None:
        """Verify venv creation uses uv when available."""
        from install_python import create_venv

        with patch("install_python.has_uv", return_value=True):
            with patch("subprocess.run") as mock_run:
                mock_run.return_value = MagicMock(returncode=0)
                venv_path = Path("/tmp/test-venv")
                create_venv(venv_path)
                mock_run.assert_called_once()
                assert "uv" in mock_run.call_args[0][0]

    def test_create_venv_with_pip(self) -> None:
        """Verify venv creation uses python when uv unavailable."""
        from install_python import create_venv

        with patch("install_python.has_uv", return_value=False):
            with patch("subprocess.run") as mock_run:
                mock_run.return_value = MagicMock(returncode=0)
                venv_path = Path("/tmp/test-venv")
                create_venv(venv_path)
                mock_run.assert_called_once()
                assert sys.executable in mock_run.call_args[0][0]


class TestPackageInstallation:
    """Tests for package installation."""

    def test_install_packages_with_uv(self) -> None:
        """Verify package installation uses uv pip."""
        from install_python import install_packages

        with patch("install_python.has_uv", return_value=True):
            with patch("subprocess.run") as mock_run:
                mock_run.return_value = MagicMock(returncode=0)
                install_packages(Path("/tmp/.venv"), ["package1", "package2"])
                mock_run.assert_called()
                call_args = mock_run.call_args[0][0]
                assert "uv" in call_args
                assert "pip" in call_args
                assert "install" in call_args

    def test_install_packages_with_pip(self) -> None:
        """Verify package installation uses pip when uv unavailable."""
        from install_python import install_packages

        with patch("install_python.has_uv", return_value=False):
            with patch("subprocess.run") as mock_run:
                mock_run.return_value = MagicMock(returncode=0)
                install_packages(Path("/tmp/.venv"), ["package1"])
                mock_run.assert_called()


class TestArgumentParsing:
    """Tests for CLI argument parsing."""

    def test_default_group(self) -> None:
        """Verify default group is 'ci'."""
        from install_python import parse_args

        args = parse_args([])
        assert args.group == "ci"

    def test_custom_group(self) -> None:
        """Verify custom group parsing."""
        from install_python import parse_args

        args = parse_args(["dev"])
        assert args.group == "dev"

    def test_multiple_groups(self) -> None:
        """Verify comma-separated groups."""
        from install_python import parse_args

        args = parse_args(["ci,coverage"])
        assert args.group == "ci,coverage"

    def test_list_groups_flag(self) -> None:
        """Verify --list-groups flag."""
        from install_python import parse_args

        args = parse_args(["--list-groups"])
        assert args.list_groups is True


class TestGroupParsing:
    """Tests for parsing group specifications."""

    def test_single_group(self) -> None:
        """Verify single group parsing."""
        from install_python import get_packages_for_groups

        packages = get_packages_for_groups("ci")
        assert len(packages) > 0

    def test_multiple_groups(self) -> None:
        """Verify comma-separated group parsing."""
        from install_python import get_packages_for_groups

        ci_packages = get_packages_for_groups("ci")
        combined = get_packages_for_groups("ci,coverage")
        assert len(combined) >= len(ci_packages)

    def test_invalid_group_raises(self) -> None:
        """Verify invalid group raises error."""
        from install_python import get_packages_for_groups

        with pytest.raises(ValueError, match="Unknown group"):
            get_packages_for_groups("nonexistent")
