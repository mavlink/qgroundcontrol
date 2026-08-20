"""Tests for install_dependencies_helper.py."""

from __future__ import annotations

from unittest.mock import MagicMock, patch

import install_dependencies_helper


class TestDetectPythonVersion:
    """Tests for detect_python_version function."""

    def test_detect_python_version(self, tmp_path):
        """Writes correct python minor version string."""
        captured = {}

        def fake_write_output(d: dict) -> None:
            captured.update(d)

        with patch.object(
            install_dependencies_helper, "write_github_output", side_effect=fake_write_output
        ):
            install_dependencies_helper.detect_python_version()

        import sys

        expected = f"{sys.version_info.major}.{sys.version_info.minor}"
        assert captured["minor"] == expected


class TestLdconfigHasBlas:
    """Tests for _ldconfig_has_blas function."""

    def test_ldconfig_has_blas_positive(self):
        """Returns True when libblas.so.3 is in ldconfig output."""
        fake_result = MagicMock()
        fake_result.stdout = (
            "\tlibblas.so.3 (libc6,x86-64) => /usr/lib/x86_64-linux-gnu/libblas.so.3\n"
            "\tlibm.so.6 (libc6,x86-64) => /lib/x86_64-linux-gnu/libm.so.6\n"
        )
        fake_result.returncode = 0
        with patch.object(install_dependencies_helper, "run_captured", return_value=fake_result):
            assert install_dependencies_helper._ldconfig_has_blas() is True

    def test_ldconfig_has_blas_negative(self):
        """Returns False when no libblas.so.3 in output."""
        fake_result = MagicMock()
        fake_result.stdout = "\tlibm.so.6 (libc6,x86-64) => /lib/x86_64-linux-gnu/libm.so.6\n"
        fake_result.returncode = 0
        with patch.object(install_dependencies_helper, "run_captured", return_value=fake_result):
            assert install_dependencies_helper._ldconfig_has_blas() is False


class TestEnableUniverse:
    """Tests for enable_universe function."""

    def test_existing_repository_skips_apt(self):
        """Does not refresh package lists when universe is already enabled."""
        with (
            patch.object(install_dependencies_helper.Path, "exists", return_value=True),
            patch.object(install_dependencies_helper.Path, "is_file", return_value=True),
            patch.object(
                install_dependencies_helper.Path,
                "read_text",
                return_value="Components: main universe restricted\n",
            ),
            patch.object(install_dependencies_helper, "_sudo") as mock_sudo,
        ):
            install_dependencies_helper.enable_universe()

        mock_sudo.assert_not_called()
