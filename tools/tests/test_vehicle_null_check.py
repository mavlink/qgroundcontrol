#!/usr/bin/env python3
"""Tests for vehicle_null_check.py analyzer."""

import json
import subprocess
import sys
from pathlib import Path

import pytest

TOOLS_DIR = Path(__file__).parent.parent
FIXTURES_DIR = Path(__file__).parent / "fixtures"
ANALYZER = TOOLS_DIR / "analyzers" / "vehicle_null_check.py"


def run_analyzer(*args, json_output=False):
    """Run the analyzer and return output."""
    cmd = [sys.executable, str(ANALYZER)]
    if json_output:
        cmd.append("--json")
    cmd.extend(str(a) for a in args)
    result = subprocess.run(cmd, capture_output=True, text=True)
    return result


class TestNullCheckAnalyzer:
    """Test null-check pattern detection."""

    def test_detects_unsafe_direct_access(self):
        """Should detect activeVehicle()->method() pattern."""
        result = run_analyzer(FIXTURES_DIR / "null_check_samples.cpp", json_output=True)
        violations = json.loads(result.stdout)

        # Find the unsafe_active_vehicle_direct violation
        direct_violations = [v for v in violations if v["pattern"] == "unsafe_active_vehicle_direct"]
        assert len(direct_violations) >= 1, "Should detect direct activeVehicle() access"

    def test_detects_unsafe_variable_use(self):
        """Should detect variable used after activeVehicle() without check."""
        result = run_analyzer(FIXTURES_DIR / "null_check_samples.cpp", json_output=True)
        violations = json.loads(result.stdout)

        use_violations = [v for v in violations if v["pattern"] == "unsafe_active_vehicle_use"]
        assert len(use_violations) >= 1, "Should detect unsafe variable use"

    def test_detects_unsafe_get_parameter(self):
        """Should detect getParameter()->method() pattern."""
        result = run_analyzer(FIXTURES_DIR / "null_check_samples.cpp", json_output=True)
        violations = json.loads(result.stdout)

        param_violations = [v for v in violations if v["pattern"] == "unsafe_get_parameter"]
        assert len(param_violations) >= 1, "Should detect unsafe getParameter access"

    def test_ignores_safe_patterns(self):
        """Should not flag code with proper null checks."""
        result = run_analyzer(FIXTURES_DIR / "null_check_samples.cpp", json_output=True)
        violations = json.loads(result.stdout)

        # Check that safe functions are not flagged
        for v in violations:
            # Safe patterns are in functions named "safe*"
            assert "safeWith" not in v["code"], f"Should not flag safe pattern: {v['code']}"

    def test_ignores_comments(self):
        """Should not detect patterns inside comments."""
        result = run_analyzer(FIXTURES_DIR / "null_check_samples.cpp", json_output=True)
        violations = json.loads(result.stdout)

        for v in violations:
            assert "comment" not in v["code"].lower(), "Should not flag commented code"

    def test_json_output_format(self):
        """JSON output should have required fields."""
        result = run_analyzer(FIXTURES_DIR / "null_check_samples.cpp", json_output=True)
        violations = json.loads(result.stdout)

        assert isinstance(violations, list)
        if violations:
            v = violations[0]
            assert "file" in v
            assert "line" in v
            assert "column" in v
            assert "pattern" in v
            assert "code" in v
            assert "suggestion" in v

    def test_exit_code_on_violations(self):
        """Should return exit code 1 when violations found."""
        result = run_analyzer(FIXTURES_DIR / "null_check_samples.cpp")
        assert result.returncode == 1

    def test_exit_code_no_violations(self, tmp_path):
        """Should return exit code 0 when no violations."""
        # Create a safe file
        safe_file = tmp_path / "safe.cpp"
        safe_file.write_text("""
void safeFunction() {
    Vehicle *v = getVehicle();
    if (!v) return;
    v->doSomething();
}
""")
        result = run_analyzer(safe_file)
        assert result.returncode == 0

    def test_help_flag(self):
        """--help should show usage and exit 0."""
        result = run_analyzer("--help")
        assert result.returncode == 0
        assert "Usage" in result.stdout or "usage" in result.stdout.lower()


class TestAnalyzerIntegration:
    """Integration tests for analyzer."""

    def test_analyze_directory(self):
        """Should analyze all C++ files in directory."""
        result = run_analyzer(FIXTURES_DIR, json_output=True)
        violations = json.loads(result.stdout)
        assert isinstance(violations, list)

    def test_multiple_files(self, tmp_path):
        """Should handle multiple file arguments."""
        f1 = tmp_path / "a.cpp"
        f2 = tmp_path / "b.cpp"
        f1.write_text("void f() { activeVehicle()->test(); }")
        f2.write_text("void g() { activeVehicle()->test(); }")

        result = run_analyzer(f1, f2, json_output=True)
        violations = json.loads(result.stdout)
        files = {v["file"] for v in violations}
        assert len(files) == 2


if __name__ == "__main__":
    pytest.main([__file__, "-v"])
