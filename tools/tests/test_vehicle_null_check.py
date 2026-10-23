"""Tests for vehicle_null_check.py analyzer."""

from __future__ import annotations

import contextlib
import io
import json
import subprocess
import sys
from pathlib import Path

import pytest
from analyzers.vehicle_null_check import analyze_file, main

from ._helpers import TOOLS_DIR

FIXTURES_DIR = Path(__file__).parent / "fixtures"
FIXTURE = FIXTURES_DIR / "null_check_samples.cpp"
ANALYZER = TOOLS_DIR / "analyzers" / "vehicle_null_check.py"


def run_analyzer(*args: Path | str) -> tuple[int, str]:
    stdout = io.StringIO()
    with contextlib.redirect_stdout(stdout), contextlib.redirect_stderr(io.StringIO()):
        returncode = main([str(arg) for arg in args])
    return returncode, stdout.getvalue()


def json_violations(*args: Path | str) -> list[dict]:
    return json.loads(run_analyzer("--json", *args)[1])


@pytest.fixture(scope="module")
def sample_violations() -> list[dict]:
    return json_violations(FIXTURE)


def test_detects_unsafe_direct_access(sample_violations: list[dict]) -> None:
    assert [v for v in sample_violations if v["pattern"] == "unsafe_active_vehicle_direct"]


def test_detects_unsafe_variable_use(sample_violations: list[dict]) -> None:
    assert [v for v in sample_violations if v["pattern"] == "unsafe_active_vehicle_use"]


def test_fixture_reports_exactly_the_unsafe_samples(sample_violations: list[dict]) -> None:
    assert [(v["line"], v["pattern"]) for v in sample_violations] == [
        (11, "unsafe_active_vehicle_direct"),
        (18, "unsafe_active_vehicle_use"),
        (29, "unsafe_active_vehicle_use"),
        (96, "unsafe_active_vehicle_direct"),  # string literals are not parsed
    ]


def test_ignores_safe_patterns(sample_violations: list[dict]) -> None:
    assert all("safeWith" not in v["code"] for v in sample_violations)


def test_ignores_comments(sample_violations: list[dict]) -> None:
    assert all("comment" not in v["code"].lower() for v in sample_violations)


def test_json_output_format(sample_violations: list[dict]) -> None:
    assert sample_violations, "analyzer should report violations for the fixture"
    for key in ("file", "line", "column", "pattern", "code", "suggestion"):
        assert key in sample_violations[0]


def test_hook_script_reports_violations() -> None:
    result = subprocess.run(
        [sys.executable, str(ANALYZER), str(FIXTURE)], capture_output=True, text=True, check=False
    )
    assert result.returncode == 1
    assert f"{FIXTURE}:11:5: warning: unsafe_active_vehicle_direct" in result.stdout
    assert "4 vehicle-null-check finding(s)" in result.stderr


def test_exit_code_no_violations(tmp_path: Path) -> None:
    safe_file = tmp_path / "safe.cpp"
    safe_file.write_text(
        "void safeFunction() {\n"
        "    Vehicle *v = getVehicle();\n"
        "    if (!v) return;\n"
        "    v->doSomething();\n"
        "}\n"
    )
    assert run_analyzer(safe_file) == (0, "")


def test_help_flag(capsys: pytest.CaptureFixture[str]) -> None:
    with pytest.raises(SystemExit) as exc:
        main(["--help"])
    assert exc.value.code == 0
    assert "usage" in capsys.readouterr().out.lower()


def test_analyze_directory_finds_fixture_violations() -> None:
    violations = json_violations(FIXTURES_DIR)
    assert violations, "directory scan should surface the known fixture violations"
    assert {Path(v["file"]).name for v in violations} == {"null_check_samples.cpp"}


def test_multiple_files(tmp_path: Path) -> None:
    f1 = tmp_path / "a.cpp"
    f2 = tmp_path / "b.cpp"
    f1.write_text("void f() { activeVehicle()->test(); }")
    f2.write_text("void g() { activeVehicle()->test(); }")

    violations = json_violations(f1, f2)
    assert len({v["file"] for v in violations}) == 2


def _use_violations(tmp_path: Path, body: str) -> list[tuple[int, str]]:
    source = tmp_path / "sample.cc"
    source.write_text(body)
    return [(v.line, v.pattern) for v in analyze_file(source)]


def test_unrelated_null_check_does_not_hide_unchecked_use(tmp_path: Path) -> None:
    body = (
        "void f(QObject *other) {\n"
        "    if (!other) {\n"
        "        return;\n"
        "    }\n"
        "    Vehicle *vehicle = MultiVehicleManager::instance()->activeVehicle();\n"
        "    vehicle->arm();\n"
        "}\n"
    )
    assert _use_violations(tmp_path, body) == [(6, "unsafe_active_vehicle_use")]


def test_check_of_longer_name_is_not_a_guard(tmp_path: Path) -> None:
    body = (
        "void f() {\n"
        "    if (vehicleCount) {\n"
        "    }\n"
        "    Vehicle *v = MultiVehicleManager::instance()->activeVehicle();\n"
        "    v->arm();\n"
        "}\n"
    )
    assert _use_violations(tmp_path, body) == [(5, "unsafe_active_vehicle_use")]


@pytest.mark.parametrize(
    "guard_and_use",
    [
        "    QString name = vehicle ? vehicle->name() : QString();\n",
        "    const bool armed = vehicle && vehicle->armed();\n",
        "    if (!vehicle || vehicle->armed()) {\n        return;\n    }\n",
        "    QVERIFY(vehicle);\n    vehicle->arm();\n",
        '    QVERIFY2(vehicle, "no vehicle");\n    vehicle->arm();\n',
    ],
)
def test_guards_of_the_assigned_variable_are_accepted(tmp_path: Path, guard_and_use: str) -> None:
    body = (
        "void f() {\n"
        "    Vehicle *vehicle = MultiVehicleManager::instance()->activeVehicle();\n"
        f"{guard_and_use}"
        "}\n"
    )
    assert _use_violations(tmp_path, body) == []


def test_direct_get_parameter_use_is_not_reported(tmp_path: Path) -> None:
    body = (
        "void f(Vehicle *vehicle) {\n"
        '    vehicle->parameterManager()->getParameter(-1, "P")->setRawValue(1);\n'
        "}\n"
    )
    assert _use_violations(tmp_path, body) == []
