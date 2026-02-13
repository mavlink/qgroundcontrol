"""Tests for build_results.py."""

from __future__ import annotations

import json
import sys
import tempfile
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).parent.parent))

from build_results import (
    ArchTestResults,
    ArtifactSize,
    CoverageInfo,
    generate_coverage_section,
    generate_report,
    generate_sizes_section,
    generate_test_section,
    get_artifact_sizes,
    get_coverage_info,
    parse_test_results,
)


class TestArchTestResults:
    """Tests for test result parsing."""

    def test_parse_test_results_empty(self, tmp_path: Path):
        """Test parsing with no test files."""
        results = parse_test_results(tmp_path)
        assert results == []

    def test_parse_test_results_single_arch(self, tmp_path: Path):
        """Test parsing single architecture results."""
        test_dir = tmp_path / "test-results-linux-x64"
        test_dir.mkdir()
        (test_dir / "test-output.txt").write_text(
            "PASS   : TestClass::testFunc()\n"
            "PASS   : TestClass::testFunc2()\n"
            "FAIL   : TestClass::testFail()\n"
            "SKIP   : TestClass::testSkip()\n"
        )

        results = parse_test_results(tmp_path)

        assert len(results) == 1
        assert results[0].arch == "linux-x64"
        assert results[0].passed == 2
        assert results[0].failed == 1
        assert results[0].skipped == 1
        assert len(results[0].failures) == 1

    def test_parse_test_results_multiple_arch(self, tmp_path: Path):
        """Test parsing multiple architecture results."""
        for arch in ["linux-x64", "windows-x64"]:
            test_dir = tmp_path / f"test-results-{arch}"
            test_dir.mkdir()
            (test_dir / "test-output.txt").write_text("PASS   : TestClass::test()\n")

        results = parse_test_results(tmp_path)

        assert len(results) == 2
        arches = [r.arch for r in results]
        assert "linux-x64" in arches
        assert "windows-x64" in arches


class TestCoverageInfo:
    """Tests for coverage parsing."""

    def test_coverage_percentage(self):
        """Test coverage percentage formatting."""
        info = CoverageInfo(line_rate=0.85)
        assert info.percentage == "85.0%"

    def test_coverage_diff_increase(self):
        """Test coverage diff with increase."""
        info = CoverageInfo(line_rate=0.85, baseline_rate=0.80)
        assert info.diff == "+5.0%"

    def test_coverage_diff_decrease(self):
        """Test coverage diff with decrease."""
        info = CoverageInfo(line_rate=0.75, baseline_rate=0.80)
        assert info.diff == "-5.0%"

    def test_coverage_diff_no_baseline(self):
        """Test coverage diff without baseline."""
        info = CoverageInfo(line_rate=0.85)
        assert info.diff is None

    def test_get_coverage_info(self, tmp_path: Path):
        """Test getting coverage info from XML."""
        coverage_dir = tmp_path / "coverage-report"
        coverage_dir.mkdir()
        (coverage_dir / "coverage.xml").write_text(
            '<?xml version="1.0"?>\n'
            '<coverage line-rate="0.75" branch-rate="0.5">\n'
            "</coverage>"
        )

        info = get_coverage_info(tmp_path, None)

        assert info is not None
        assert info.line_rate == 0.75
        assert info.baseline_rate is None


class TestArtifactSize:
    """Tests for artifact size handling."""

    def test_delta_increase(self):
        """Test size delta with increase."""
        size = ArtifactSize(
            name="test.apk",
            size_bytes=110 * 1024 * 1024,
            size_human="110 MB",
            baseline_bytes=100 * 1024 * 1024,
        )
        assert "increase" in size.delta
        assert "+10.00 MB" in size.delta

    def test_delta_decrease(self):
        """Test size delta with decrease."""
        size = ArtifactSize(
            name="test.apk",
            size_bytes=90 * 1024 * 1024,
            size_human="90 MB",
            baseline_bytes=100 * 1024 * 1024,
        )
        assert "decrease" in size.delta
        assert "-10.00 MB" in size.delta

    def test_delta_no_change(self):
        """Test size delta with no change."""
        size = ArtifactSize(
            name="test.apk",
            size_bytes=100 * 1024 * 1024,
            size_human="100 MB",
            baseline_bytes=100 * 1024 * 1024,
        )
        assert size.delta == "No change"

    def test_delta_no_baseline(self):
        """Test size delta without baseline."""
        size = ArtifactSize(
            name="test.apk",
            size_bytes=100 * 1024 * 1024,
            size_human="100 MB",
        )
        assert size.delta is None

    def test_get_artifact_sizes(self, tmp_path: Path):
        """Test loading artifact sizes from JSON."""
        sizes_file = tmp_path / "sizes.json"
        sizes_file.write_text(
            json.dumps(
                {
                    "artifacts": [
                        {"name": "test.apk", "size_bytes": 1000000, "size_human": "1 MB"}
                    ]
                }
            )
        )

        sizes = get_artifact_sizes(sizes_file, None)

        assert len(sizes) == 1
        assert sizes[0].name == "test.apk"
        assert sizes[0].size_bytes == 1000000


class TestMarkdownGeneration:
    """Tests for markdown output generation."""

    def test_generate_test_section_empty(self):
        """Test test section with no results."""
        section = generate_test_section([])
        assert section == ""

    def test_generate_test_section_passing(self):
        """Test test section with passing tests."""
        results = [ArchTestResults(arch="linux", passed=10, failed=0, skipped=2)]
        section = generate_test_section(results)

        assert "### Test Results" in section
        assert "linux" in section
        assert "10 passed" in section
        assert "2 skipped" in section
        assert "failed" not in section.lower() or "0 failed" not in section

    def test_generate_test_section_failures(self):
        """Test test section with failures."""
        results = [
            ArchTestResults(
                arch="linux",
                passed=8,
                failed=2,
                skipped=0,
                failures=["FAIL   : Test1()", "FAIL   : Test2()"],
            )
        ]
        section = generate_test_section(results)

        assert "2 failed" in section
        assert "<details>" in section
        assert "FAIL" in section

    def test_generate_coverage_section_no_baseline(self):
        """Test coverage section without baseline."""
        info = CoverageInfo(line_rate=0.85)
        section = generate_coverage_section(info)

        assert "### Code Coverage" in section
        assert "85.0%" in section
        assert "No baseline" in section

    def test_generate_coverage_section_with_baseline(self):
        """Test coverage section with baseline."""
        info = CoverageInfo(line_rate=0.85, baseline_rate=0.80)
        section = generate_coverage_section(info)

        assert "| Coverage | Baseline | Change |" in section
        assert "85.0%" in section
        assert "80.0%" in section
        assert "+5.0%" in section

    def test_generate_sizes_section_empty(self):
        """Test sizes section with no sizes."""
        section = generate_sizes_section([])
        assert section == ""

    def test_generate_sizes_section_no_baseline(self):
        """Test sizes section without baseline."""
        sizes = [
            ArtifactSize(name="test.apk", size_bytes=1000000, size_human="1 MB")
        ]
        section = generate_sizes_section(sizes)

        assert "### Artifact Sizes" in section
        assert "test.apk" in section
        assert "1 MB" in section
        assert "No baseline" in section


class TestFullReport:
    """Tests for full report generation."""

    def test_generate_report_empty(self, tmp_path: Path):
        """Test report with no data."""
        report = generate_report(
            artifacts_dir=tmp_path,
            pr_sizes_file=None,
            baseline_coverage=None,
            baseline_sizes=None,
        )

        assert "## Build Results" in report

    def test_generate_report_with_builds_table(self, tmp_path: Path):
        """Test report with builds table."""
        report = generate_report(
            artifacts_dir=tmp_path,
            pr_sizes_file=None,
            baseline_coverage=None,
            baseline_sizes=None,
            builds_table="| Linux | Passed |",
            builds_summary="All builds passed.",
        )

        assert "### Platform Status" in report
        assert "| Linux | Passed |" in report
        assert "All builds passed" in report
