"""Tests for install_dependencies_helper.py."""

from __future__ import annotations

from unittest.mock import MagicMock, patch

import install_dependencies_helper


class TestNormalizeAptSources:
    """Tests for Ubuntu apt mirror normalization."""

    def test_normalizes_legacy_ec2_ports_mirror(self):
        """Remaps the ARM64 EC2 mirror to the canonical HTTPS endpoint."""
        source = "deb http://us-west-2.ec2.ports.ubuntu.com/ubuntu-ports noble main universe\n"

        assert install_dependencies_helper._normalize_ubuntu_apt_urls(source) == (
            "deb https://ports.ubuntu.com/ubuntu-ports noble main universe\n"
        )

    def test_normalizes_deb822_official_mirrors(self):
        """Normalizes official archive and security URLs in deb822 sources."""
        source = (
            "Types: deb\n"
            "URIs: http://archive.ubuntu.com/ubuntu http://security.ubuntu.com/ubuntu\n"
            "Suites: noble noble-updates noble-security\n"
        )

        assert install_dependencies_helper._normalize_ubuntu_apt_urls(source) == (
            "Types: deb\n"
            "URIs: https://archive.ubuntu.com/ubuntu https://security.ubuntu.com/ubuntu\n"
            "Suites: noble noble-updates noble-security\n"
        )

    def test_preserves_third_party_and_unknown_ubuntu_mirrors(self):
        """Does not rewrite repositories whose HTTPS support is unknown."""
        source = (
            "deb http://packages.example.com/ubuntu noble main\n"
            "deb http://azure.archive.ubuntu.com/ubuntu noble main\n"
        )

        assert install_dependencies_helper._normalize_ubuntu_apt_urls(source) == source

    def test_updates_legacy_and_deb822_files(self, tmp_path, capsys):
        """Discovers and updates both supported apt source file formats."""
        sources_dir = tmp_path / "sources.list.d"
        sources_dir.mkdir()
        legacy_source = tmp_path / "sources.list"
        deb822_source = sources_dir / "ubuntu.sources"
        ignored_source = sources_dir / "ignored.conf"
        legacy_source.write_text("deb http://ports.ubuntu.com/ubuntu-ports noble main\n")
        deb822_source.write_text("URIs: http://eu-west-1.ec2.archive.ubuntu.com/ubuntu\n")
        ignored_source.write_text("deb http://archive.ubuntu.com/ubuntu noble main\n")

        install_dependencies_helper.normalize_apt_sources(tmp_path)

        assert legacy_source.read_text() == (
            "deb https://ports.ubuntu.com/ubuntu-ports noble main\n"
        )
        assert deb822_source.read_text() == "URIs: https://archive.ubuntu.com/ubuntu\n"
        assert ignored_source.read_text() == "deb http://archive.ubuntu.com/ubuntu noble main\n"
        assert capsys.readouterr().out == "Normalized 2 apt source file(s)\n"


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
