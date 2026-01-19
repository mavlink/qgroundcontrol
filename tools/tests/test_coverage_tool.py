"""Tests for coverage.py code coverage tool."""

from __future__ import annotations

import sys
from pathlib import Path
from unittest.mock import MagicMock, patch

import pytest

sys.path.insert(0, str(Path(__file__).parent.parent))

from coverage import (
    CoverageResult,
    CoverageRunner,
)
from common import Logger


class TestCoverageResult:
    """Tests for CoverageResult dataclass."""

    def test_default_values(self):
        """Test default CoverageResult values."""
        result = CoverageResult()
        assert result.line_coverage == 0.0
        assert result.branch_coverage == 0.0
        assert result.html_path is None
        assert result.xml_path is None
        assert result.success is True
        assert result.error_message == ""

    def test_with_coverage_values(self):
        """Test CoverageResult with coverage values."""
        result = CoverageResult(
            line_coverage=85.5,
            branch_coverage=72.3,
            html_path=Path("/tmp/coverage/html"),
            xml_path=Path("/tmp/coverage/coverage.xml"),
        )
        assert result.line_coverage == 85.5
        assert result.branch_coverage == 72.3
        assert result.html_path is not None

    def test_failed_result(self):
        """Test failed CoverageResult."""
        result = CoverageResult(
            success=False,
            error_message="gcovr not found",
        )
        assert result.success is False
        assert "gcovr" in result.error_message


class TestCoverageRunner:
    """Tests for CoverageRunner class."""

    def test_init(self, tmp_path):
        """Test CoverageRunner initialization."""
        build_dir = tmp_path / "build"
        build_dir.mkdir()
        output_dir = tmp_path / "coverage"

        runner = CoverageRunner(
            build_dir=build_dir,
            output_dir=output_dir,
        )

        assert runner.build_dir == build_dir.resolve()
        assert runner.output_dir == output_dir.resolve()

    def test_init_with_logger(self, tmp_path):
        """Test CoverageRunner with custom logger."""
        build_dir = tmp_path / "build"
        build_dir.mkdir()
        output_dir = tmp_path / "coverage"
        logger = Logger(color=False)

        runner = CoverageRunner(
            build_dir=build_dir,
            output_dir=output_dir,
            logger=logger,
        )

        assert runner.logger == logger

    def test_check_dependencies_missing_gcovr(self, tmp_path):
        """Test check_dependencies when gcovr missing."""
        build_dir = tmp_path / "build"
        build_dir.mkdir()
        output_dir = tmp_path / "coverage"

        runner = CoverageRunner(
            build_dir=build_dir,
            output_dir=output_dir,
            logger=Logger(color=False),
        )

        with patch("shutil.which", return_value=None):
            result = runner.check_dependencies()

        assert result is False

    def test_check_dependencies_all_present(self, tmp_path):
        """Test check_dependencies when all tools present."""
        build_dir = tmp_path / "build"
        build_dir.mkdir()
        output_dir = tmp_path / "coverage"

        runner = CoverageRunner(
            build_dir=build_dir,
            output_dir=output_dir,
            logger=Logger(color=False),
        )

        with patch("shutil.which", return_value="/usr/bin/tool"):
            result = runner.check_dependencies()

        assert result is True


class TestCoverageRunnerConfigureBuild:
    """Tests for CoverageRunner.configure_build method."""

    def test_configure_build_existing_config(self, tmp_path):
        """Test configure_build with existing coverage config."""
        build_dir = tmp_path / "build"
        build_dir.mkdir()
        output_dir = tmp_path / "coverage"

        # Create CMakeCache with coverage enabled
        cmake_cache = build_dir / "CMakeCache.txt"
        cmake_cache.write_text("QGC_ENABLE_COVERAGE:BOOL=ON\n")

        runner = CoverageRunner(
            build_dir=build_dir,
            output_dir=output_dir,
            logger=Logger(color=False),
        )

        result = runner.configure_build()
        assert result is True


class TestCoverageRunnerGenerateReport:
    """Tests for CoverageRunner.generate_report method."""

    def test_generate_report_no_build_dir(self, tmp_path):
        """Test generate_report with missing build directory."""
        output_dir = tmp_path / "coverage"

        runner = CoverageRunner(
            build_dir=tmp_path / "nonexistent",
            output_dir=output_dir,
            logger=Logger(color=False),
        )

        result = runner.generate_report()
        assert result.success is False

    def test_generate_report_options(self, tmp_path):
        """Test generate_report with different options."""
        build_dir = tmp_path / "build"
        build_dir.mkdir()
        output_dir = tmp_path / "coverage"

        runner = CoverageRunner(
            build_dir=build_dir,
            output_dir=output_dir,
            logger=Logger(color=False),
        )

        # Can pass html and xml options
        with patch("subprocess.run") as mock_run:
            mock_run.return_value = MagicMock(returncode=1, stderr="No data")
            result = runner.generate_report(html=True, xml=False)

        # Will fail due to no coverage data, but tests the interface
        assert isinstance(result, CoverageResult)


class TestCoverageRunnerCheckThreshold:
    """Tests for CoverageRunner.check_threshold method."""

    def test_check_threshold_pass(self, tmp_path):
        """Test check_threshold when coverage meets threshold."""
        build_dir = tmp_path / "build"
        build_dir.mkdir()
        output_dir = tmp_path / "coverage"

        runner = CoverageRunner(
            build_dir=build_dir,
            output_dir=output_dir,
            logger=Logger(color=False),
        )

        result = CoverageResult(line_coverage=85.0)
        assert runner.check_threshold(result, 80.0) is True

    def test_check_threshold_fail(self, tmp_path):
        """Test check_threshold when coverage below threshold."""
        build_dir = tmp_path / "build"
        build_dir.mkdir()
        output_dir = tmp_path / "coverage"

        runner = CoverageRunner(
            build_dir=build_dir,
            output_dir=output_dir,
            logger=Logger(color=False),
        )

        result = CoverageResult(line_coverage=75.0)
        assert runner.check_threshold(result, 80.0) is False


class TestCoverageRunnerClean:
    """Tests for CoverageRunner.clean method."""

    def test_clean_removes_build_dir(self, tmp_path):
        """Test clean removes build directory."""
        build_dir = tmp_path / "build"
        build_dir.mkdir()
        output_dir = tmp_path / "coverage"

        # Create some files in build dir
        (build_dir / "CMakeCache.txt").write_text("cache")
        (build_dir / "coverage.gcda").write_text("data")

        runner = CoverageRunner(
            build_dir=build_dir,
            output_dir=output_dir,
            logger=Logger(color=False),
        )

        runner.clean()

        # Build directory should be removed
        assert not build_dir.exists()


if __name__ == "__main__":
    pytest.main([__file__, "-v"])
