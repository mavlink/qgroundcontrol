"""Tests for generate_docs.py documentation generator."""

from __future__ import annotations

import sys
from pathlib import Path
from unittest.mock import MagicMock, patch

import pytest

sys.path.insert(0, str(Path(__file__).parent.parent))

from generate_docs import (
    DoxygenGenerator,
    DOXYFILE_TEMPLATE,
)


class TestDoxyfileTemplate:
    """Tests for Doxyfile template."""

    def test_template_has_placeholders(self):
        """Test template contains expected placeholders."""
        assert "{output_dir}" in DOXYFILE_TEMPLATE
        assert "{generate_latex}" in DOXYFILE_TEMPLATE
        assert "{have_dot}" in DOXYFILE_TEMPLATE
        assert "{quiet}" in DOXYFILE_TEMPLATE

    def test_template_format(self):
        """Test template can be formatted."""
        content = DOXYFILE_TEMPLATE.format(
            output_dir="docs/output",
            generate_latex="NO",
            have_dot="YES",
            quiet="NO",
        )
        assert "OUTPUT_DIRECTORY" in content
        assert "docs/output" in content


class TestDoxygenGenerator:
    """Tests for DoxygenGenerator class."""

    def test_init(self, tmp_path):
        """Test DoxygenGenerator initialization."""
        generator = DoxygenGenerator(
            repo_root=tmp_path,
            output_dir=tmp_path / "docs",
        )
        assert generator.repo_root == tmp_path
        assert generator.output_dir == tmp_path / "docs"
        assert generator.generate_pdf is False
        assert generator.quiet is False

    def test_init_with_pdf(self, tmp_path):
        """Test DoxygenGenerator with PDF generation."""
        generator = DoxygenGenerator(
            repo_root=tmp_path,
            output_dir=tmp_path / "docs",
            generate_pdf=True,
        )
        assert generator.generate_pdf is True

    def test_check_dependencies_missing_doxygen(self, tmp_path):
        """Test check_dependencies with missing doxygen."""
        generator = DoxygenGenerator(tmp_path, tmp_path / "docs")

        with patch("shutil.which", return_value=None):
            result = generator.check_dependencies()

        assert result is False

    def test_check_dependencies_all_present(self, tmp_path):
        """Test check_dependencies with all tools present."""
        generator = DoxygenGenerator(tmp_path, tmp_path / "docs")

        def which_side_effect(tool):
            return f"/usr/bin/{tool}"

        with patch("shutil.which", side_effect=which_side_effect):
            result = generator.check_dependencies()

        assert result is True

    def test_check_dependencies_missing_graphviz(self, tmp_path):
        """Test check_dependencies with missing graphviz."""
        generator = DoxygenGenerator(tmp_path, tmp_path / "docs")

        def which_side_effect(tool):
            if tool == "dot":
                return None
            return f"/usr/bin/{tool}"

        with patch("shutil.which", side_effect=which_side_effect):
            result = generator.check_dependencies()

        # Should still pass, just warn about missing graphviz
        assert result is True
        assert generator._has_graphviz is False


class TestDoxygenGeneratorDoxyfile:
    """Tests for DoxygenGenerator Doxyfile generation."""

    def test_generate_doxyfile(self, tmp_path):
        """Test generating Doxyfile."""
        generator = DoxygenGenerator(tmp_path, tmp_path / "docs")
        generator._has_graphviz = True

        doxyfile_path = generator.generate_doxyfile()

        assert doxyfile_path.exists()
        content = doxyfile_path.read_text()
        assert "OUTPUT_DIRECTORY" in content

    def test_generate_doxyfile_no_graphviz(self, tmp_path):
        """Test generating Doxyfile without graphviz."""
        generator = DoxygenGenerator(tmp_path, tmp_path / "docs")
        generator._has_graphviz = False

        doxyfile_path = generator.generate_doxyfile()

        content = doxyfile_path.read_text()
        assert "HAVE_DOT               = NO" in content


class TestDoxygenGeneratorRun:
    """Tests for DoxygenGenerator run_doxygen method."""

    def test_run_doxygen_success(self, tmp_path):
        """Test successful doxygen run."""
        generator = DoxygenGenerator(tmp_path, tmp_path / "docs")
        generator._has_graphviz = True

        with patch("subprocess.run") as mock_run:
            mock_run.return_value = MagicMock(returncode=0)
            result = generator.run_doxygen()

        assert result is True

    def test_run_doxygen_failure(self, tmp_path):
        """Test failed doxygen run."""
        generator = DoxygenGenerator(tmp_path, tmp_path / "docs")
        generator._has_graphviz = True

        with patch("subprocess.run") as mock_run:
            from subprocess import CalledProcessError
            mock_run.side_effect = CalledProcessError(1, "doxygen")
            result = generator.run_doxygen()

        assert result is False


class TestDoxygenGeneratorClean:
    """Tests for DoxygenGenerator clean method."""

    def test_clean_removes_output(self, tmp_path):
        """Test clean removes output directory."""
        output_dir = tmp_path / "docs"
        output_dir.mkdir()
        (output_dir / "html").mkdir()
        (output_dir / "html" / "index.html").write_text("<html></html>")

        generator = DoxygenGenerator(tmp_path, output_dir)

        # Create doxyfile
        generator.doxyfile.write_text("# Doxyfile")

        generator.clean()

        assert not output_dir.exists()
        assert not generator.doxyfile.exists()

    def test_clean_nonexistent_dir(self, tmp_path):
        """Test clean with nonexistent directory."""
        generator = DoxygenGenerator(tmp_path, tmp_path / "nonexistent")

        # Should not raise
        generator.clean()


class TestDoxygenGeneratorOpenDocs:
    """Tests for DoxygenGenerator open_docs method."""

    def test_open_docs_missing(self, tmp_path):
        """Test open_docs with missing documentation."""
        generator = DoxygenGenerator(tmp_path, tmp_path / "docs")

        with pytest.raises(SystemExit):
            generator.open_docs()

    def test_open_docs_present(self, tmp_path):
        """Test open_docs with present documentation."""
        output_dir = tmp_path / "docs"
        html_dir = output_dir / "html"
        html_dir.mkdir(parents=True)
        (html_dir / "index.html").write_text("<html></html>")

        generator = DoxygenGenerator(tmp_path, output_dir)

        with patch("webbrowser.open") as mock_open:
            generator.open_docs()
            mock_open.assert_called_once()


if __name__ == "__main__":
    pytest.main([__file__, "-v"])
