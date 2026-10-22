"""Tests for install_dependencies_helper.py."""

from __future__ import annotations

from unittest.mock import MagicMock, patch

import pytest
from ci.runner import install_dependencies_helper


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


class TestFixAptAlternatives:
    """Tests for fix_apt_alternatives function."""

    def test_skips_blas_repair_when_blas_is_not_installed(self):
        """A category install without BLAS (e.g. Android's core) must not gain BLAS packages."""
        with (
            patch.object(
                install_dependencies_helper, "_sudo", return_value=MagicMock(returncode=0)
            ) as mock_sudo,
            patch.object(install_dependencies_helper, "_package_installed", return_value=False),
            patch.object(install_dependencies_helper, "_ldconfig_has_blas") as mock_has_blas,
        ):
            install_dependencies_helper.fix_apt_alternatives()

        mock_sudo.assert_called_once_with(["dpkg", "--configure", "-a"], check=False)
        mock_has_blas.assert_not_called()

    @pytest.mark.parametrize(
        ("returncode", "stdout", "expected"),
        [(0, "installed", True), (0, "config-files", False), (1, "", False)],
    )
    def test_package_installed_reads_dpkg_status(self, returncode, stdout, expected):
        """Only a fully installed package counts."""
        result = MagicMock(returncode=returncode, stdout=stdout)
        with patch.object(install_dependencies_helper, "run_captured", return_value=result):
            assert install_dependencies_helper._package_installed("libblas3") is expected


class TestPrintPackages:
    """Tests for print_packages function."""

    @pytest.mark.parametrize(("category", "selected"), [("core", "core"), ("", None)])
    def test_forwards_the_category(self, category, selected):
        """A blank category selects every category; otherwise only that one."""
        with (
            patch.object(
                install_dependencies_helper, "get_debian_packages", return_value=["cmake", "git"]
            ) as mock_packages,
            patch.object(install_dependencies_helper, "write_github_output") as mock_output,
        ):
            install_dependencies_helper.print_packages(category)

        mock_packages.assert_called_once_with(selected)
        mock_output.assert_called_once_with({"packages": "cmake git"})

    def test_core_category_resolves_real_package_list(self):
        with patch.object(install_dependencies_helper, "write_github_output") as mock_output:
            install_dependencies_helper.print_packages("core")

        packages = mock_output.call_args.args[0]["packages"].split()
        assert {"cmake", "ninja-build"} <= set(packages)

    def test_unknown_category_fails_instead_of_installing_nothing(self):
        """install_dependencies has an empty list for an unknown category."""
        with (
            patch.object(install_dependencies_helper, "write_github_output") as mock_output,
            pytest.raises(SystemExit),
        ):
            install_dependencies_helper.print_packages("bogus")

        mock_output.assert_not_called()
