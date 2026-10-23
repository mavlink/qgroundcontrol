"""Tests for generate_build_results_comment.py.

The scenario builders below also render the example comments for review; refresh the snapshots
with --snapshot-update after an intended layout change.
"""

from __future__ import annotations

import json
import re
from datetime import datetime, timezone
from typing import TYPE_CHECKING

import pytest
from ci.reporting import generate_build_results_comment as mod
from ci.reporting.generate_build_results_comment import generate_comment
from common.analysis_diagnostics import load_unique_warnings

from .._helpers import write_files
from ..test_junit import ASAN_FAILURE, QT_FAILURE, TIMEOUT_OUTPUT, junit_xml

if TYPE_CHECKING:
    from pathlib import Path

    from syrupy.assertion import SnapshotAssertion

NOW = datetime(2026, 2, 17, 12, 0, 0, tzinfo=timezone.utc)
HEAD = "0123456789abcdef0123456789abcdef01234567"
BASE = "a" * 40
# GitHub's merge of HEAD into the base branch, which Code Analysis checks out.
MERGE = "fedcba9876543210fedcba9876543210fedcba98"
RUNS = "https://github.com/mavlink/qgroundcontrol/actions/runs"
BLOB = "https://github.com/mavlink/qgroundcontrol/blob"
MIB = 1024 * 1024
EXPIRES = "2026-02-24T11:00:00Z"


def _passing(
    prefix: str, count: int, seconds: float = 0.5
) -> list[tuple[str, str, float, str, str]]:
    return [(f"{prefix}{index:03d}Test", "passed", seconds, "", "") for index in range(count)]


SLOW = [
    ("MissionControllerTest", "passed", 95.2, "", ""),
    ("VehicleLinkManagerTest", "passed", 61.0, "", ""),
    ("ParameterManagerTest", "passed", 48.7, "", ""),
    ("FTPManagerTest", "passed", 33.1, "", ""),
]


def _job(
    name: str,
    run_id: int,
    job_id: int,
    *,
    conclusion: str = "success",
    seconds: float = 600.0,
    failed_steps: tuple[tuple[str, int], ...] = (),
    runner_lost: dict[str, str] | None = None,
) -> dict[str, object]:
    url = f"{RUNS}/{run_id}/job/{job_id}"
    return {
        "name": name,
        "status": "completed",
        "conclusion": conclusion,
        "url": url,
        "duration_seconds": seconds,
        "failed_steps": [
            {"name": step, "number": number, "url": f"{url}#step:{number}:1"}
            for step, number in failed_steps
        ],
        "runner_lost": runner_lost,
    }


def _workflow(
    name: str,
    state: str,
    run_id: int | None,
    *,
    seconds: float | None = None,
    attempt: int = 1,
    elapsed: float | None = None,
    jobs: tuple[dict[str, object], ...] = (),
) -> dict[str, object]:
    return {
        "name": name,
        "state": state,
        "run_id": str(run_id or ""),
        "url": f"{RUNS}/{run_id}" if run_id else "",
        "run_number": run_id - 9000 if run_id else None,
        "attempt": attempt if run_id else None,
        "duration_seconds": seconds,
        "elapsed_seconds": seconds if elapsed is None else elapsed,
        "jobs": list(jobs),
    }


def _coverage(src: tuple[int, int], test: tuple[int, int] = (9_000, 9_100)) -> str:
    """A gcovr summary of one src/ file and one test/ file, with lines and branches."""

    def entry(filename: str, lines: tuple[int, int]) -> dict[str, object]:
        return {
            "filename": filename,
            "line_covered": lines[0],
            "line_total": lines[1],
            "function_covered": lines[0] // 10,
            "function_total": lines[1] // 10,
            "branch_covered": lines[0] // 2,
            "branch_total": lines[1],
        }

    files = [entry("src/Vehicle/Vehicle.cc", src), entry("test/Vehicle/VehicleTest.cc", test)]
    totals = {
        key: sum(int(file[key]) for file in files)  # type: ignore[call-overload]
        for key in files[0]
        if key != "filename"
    }
    percents = {
        f"{metric}_percent": round(
            100.0 * totals[f"{metric}_covered"] / totals[f"{metric}_total"], 1
        )
        for metric in ("line", "function", "branch")
    }
    return json.dumps({**totals, **percents, "files": files})


RUN_IDS = {"Linux": 9901, "Windows": 9902, "MacOS": 9903, "Android": 9904, "iOS": 9907}


def _sizes(head: str, sizes: list[tuple[str, str, int]], runs: list[dict[str, object]]) -> str:
    return json.dumps(
        {
            "head_sha": head,
            "runs": runs,
            "artifacts": [
                {
                    "name": name,
                    "workflow": workflow,
                    "size_bytes": size,
                    "run_id": RUN_IDS.get(workflow, 0),
                    "artifact_id": 5000 + index,
                    "expires_at": EXPIRES,
                }
                for index, (name, workflow, size) in enumerate(sizes, 1)
            ],
        }
    )


def _base_run(workflow: str, run_id: int, conclusion: str = "success", status: str = "completed"):
    return {
        "workflow": workflow,
        "run_id": run_id,
        "run_number": run_id - 8000,
        "status": status,
        "conclusion": conclusion,
        "url": f"{RUNS}/{run_id}",
    }


def _timings(tool: str, files: int, findings: list[str], errors: list[str] = ()) -> str:  # type: ignore[assignment]
    rows = [{"file": f"src/File{index:03d}.cc", "status": "passed"} for index in range(files)]
    rows += [{"file": name, "status": "findings"} for name in findings]
    rows += [{"file": name, "status": "error"} for name in errors]
    return json.dumps({"tool": tool, "selected_files": len(rows), "files": rows})


def _diagnostics(
    tool: str,
    by_file: dict[str, list[list[object]]],
    changed: dict[str, list[list[object]]] | None = None,
) -> str:
    report: dict[str, object] = {"tool": tool, "shard": 1, "commit": MERGE, "diagnostics": by_file}
    if changed is not None:
        report["changed"] = changed
    return json.dumps(report)


# A header warning repeats across shards; error diagnostics and malformed rows are not warnings.
# The pull request changed Vehicle.h line 10 and MissionController.cc lines 7 and 9.
DIAGNOSTICS = {
    "analysis-artifacts/clang-tidy-clazy-output-1/clang-tidy-diagnostics.json": _diagnostics(
        "clang-tidy",
        {
            "src/Vehicle/Vehicle.h": [
                [10, 5, "warning", "modernize-use-nodiscard"],
                [12, 1, "warning", "readability-identifier-naming"],
            ],
            "src/Vehicle/Vehicle.cc": [[40, 3, "warning", "readability-implicit-bool-conversion"]],
        },
        {"src/Vehicle/Vehicle.h": [[10, 5, "warning", "modernize-use-nodiscard"]]},
    ),
    "analysis-artifacts/clang-tidy-clazy-output-2/clang-tidy-diagnostics.json": _diagnostics(
        "clang-tidy",
        {
            "src/Vehicle/Vehicle.h": [[10, 5, "warning", "modernize-use-nodiscard"]],
            "src/MissionManager/MissionController.cc": [
                [7, 1, "warning", "modernize-use-nodiscard"],
                [9, 2, "error", "clang-diagnostic-error"],
                ["malformed"],
            ],
        },
        {
            "src/Vehicle/Vehicle.h": [[10, 5, "warning", "modernize-use-nodiscard"]],
            "src/MissionManager/MissionController.cc": [
                [7, 1, "warning", "modernize-use-nodiscard"],
                [9, 2, "error", "clang-diagnostic-error"],
            ],
        },
    ),
    "analysis-artifacts/clang-tidy-clazy-output-1/clazy-diagnostics.json": _diagnostics(
        "clazy",
        {"src/Vehicle/Vehicle.cc": [[5, 1, "warning", "-Wclazy-non-pod-global-static"]]},
        {},
    ),
}


PR_SIZES = [
    ("QGroundControl-aarch64.AppImage", "Linux", 192_280_813),
    ("QGroundControl-x86_64.AppImage", "Linux", 203_088_195),
    ("QGroundControl-installer-AMD64.exe", "Windows", 102_760_448),
    ("QGroundControl.dmg", "MacOS", 351_272_960),
    ("QGroundControl-linux-arm64-v8a.apk", "Android", 89_128_960),
    ("QGroundControl.ipa", "iOS", 81_146_176),
]
BASE_SIZES = [
    ("QGroundControl-aarch64.AppImage", "Linux", 192_276_717),
    ("QGroundControl-x86_64.AppImage", "Linux", 203_088_195),
    ("QGroundControl-installer-AMD64.exe", "Windows", 102_700_000),
    ("QGroundControl.dmg", "MacOS", 351_400_000),
    ("QGroundControl-linux-arm64-v8a-armeabi-v7a.apk", "Android", 170_000_000),
    ("QGroundControl.ipa", "iOS", 81_183_320),
]
BASE_RUNS = [
    _base_run("Linux", 8801),
    _base_run("Windows", 8802),
    _base_run("MacOS", 8803, conclusion="failure"),
    _base_run("Android", 8804),
    _base_run("iOS", 8807),
]
MANIFEST = [
    ("test-results-linux-coverage-unit", "Linux"),
    ("test-results-linux-coverage-integration", "Linux"),
    ("test-results-linux-sanitizers-unit", "Linux"),
    ("test-results-linux-sanitizers-integration", "Linux"),
    ("test-results-macos-unit", "MacOS"),
    ("test-results-macos-integration", "MacOS"),
    ("test-results-portable-windows", "Windows"),
    ("coverage-report", "Linux"),
]
OTHERS = [
    _workflow("CI Scripts", "success", 9911, seconds=95),
    _workflow("CodeQL", "success", 9912, seconds=150),
    _workflow("Docker", "success", 9913, seconds=610),
]


def _manifest(entries: list[tuple[str, str]], **extra: object) -> str:
    return json.dumps(
        [{"name": name, "workflow": workflow, "run_id": 1, **extra} for name, workflow in entries]
    )


def _linux_jobs(run_id: int, *, failed: bool = False) -> tuple[dict[str, object], ...]:
    coverage = _job(
        "Test + Coverage linux_gcc_64 Debug",
        run_id,
        2,
        seconds=1420,
        conclusion="failure" if failed else "success",
        failed_steps=(("Run and report tests", 14),) if failed else (),
    )
    gate_failed = (("Require every leg to pass", 3),) if failed else ()
    return (
        _job("changes / Detect Changes", run_id, 1, seconds=12),
        _job("Release linux_gcc_64", run_id, 3, seconds=1210),
        coverage,
        _job("Sanitizers linux_gcc_64 Debug (ASan+UBSan)", run_id, 4, seconds=1305),
        _job(
            "Linux",
            run_id,
            5,
            seconds=4,
            conclusion="failure" if failed else "success",
            failed_steps=gate_failed,
        ),
    )


def write_scenario(base: Path, scenario: str) -> dict[str, str]:
    """Write a scenario's inputs below *base* and return the comment step's environment."""
    linux_unit = [*SLOW, *_passing("Unit", 374)]
    # The base ran one test this pull request removed and lacked the one it added.
    base_unit = [*SLOW[:-1], ("RetiredLinkTest", "passed", 1.0, "", ""), *_passing("Unit", 374)]
    files: dict[str, str] = {
        "artifacts/download-manifest.json": _manifest(MANIFEST, downloaded=True),
        "artifacts/test-results-linux-coverage-unit/junit-results-linux-coverage-unit.xml": junit_xml(
            linux_unit, 83
        ),
        "artifacts/test-results-linux-coverage-integration/junit.xml": junit_xml(
            _passing("Integration", 61, 4.0), 412
        ),
        "artifacts/test-results-linux-sanitizers-unit/junit.xml": junit_xml(linux_unit, 141),
        "artifacts/test-results-linux-sanitizers-integration/junit.xml": junit_xml(
            _passing("Integration", 61, 6.0), 380
        ),
        "artifacts/test-results-macos-unit/junit.xml": junit_xml(
            [*_passing("Unit", 378), ("SerialLinkTest", "skipped", 0, "", "")], 96
        ),
        "artifacts/test-results-macos-integration/junit.xml": junit_xml(
            _passing("Integration", 61, 3.0), 205
        ),
        "artifacts/test-results-portable-windows/junit.xml": junit_xml(_passing("Portable", 10), 4),
        "artifacts/coverage-report/coverage-summary.json": _coverage((66_120, 99_400)),
        "artifacts/pre-commit-results/pre-commit-results.json": json.dumps(
            {
                "exit_code": "0",
                "passed": "41",
                "failed": "0",
                "skipped": "17",
                "failed_hooks": [],
                "run_url": f"{RUNS}/9905",
            }
        ),
        "analysis-artifacts/clang-tidy-clazy-output-1/clang-tidy-timings.json": _timings(
            "clang-tidy", 75, []
        ),
        "analysis-artifacts/clang-tidy-clazy-output-1/clazy-timings.json": _timings(
            "clazy", 74, ["src/Vehicle/Vehicle.cc"]
        ),
        "analysis-artifacts/clang-tidy-clazy-output-2/clang-tidy-timings.json": _timings(
            "clang-tidy", 75, []
        ),
        "analysis-artifacts/clang-tidy-clazy-output-2/clazy-timings.json": _timings(
            "clazy", 75, []
        ),
        **DIAGNOSTICS,
        "pr-sizes.json": _sizes(HEAD, PR_SIZES, []),
        "base-sizes.json": _sizes(BASE, BASE_SIZES, BASE_RUNS),
        "base-artifacts/download-manifest.json": _manifest(
            [
                ("test-results-linux-coverage-unit", "Linux"),
                ("test-results-macos-unit", "MacOS"),
                ("coverage-report", "Linux"),
            ],
            downloaded=True,
        ),
        "base-artifacts/test-results-linux-coverage-unit/junit.xml": junit_xml(base_unit, 80),
        "base-artifacts/test-results-macos-unit/junit.xml": junit_xml(
            [*_passing("Unit", 378), ("SerialLinkTest", "skipped", 0, "", "")], 95
        ),
        "base-artifacts/coverage-report/coverage-summary.json": _coverage((66_050, 99_380)),
    }
    platforms = [
        _workflow("Linux", "success", 9901, seconds=1610, jobs=_linux_jobs(9901)),
        _workflow("Windows", "success", 9902, seconds=1202, attempt=2, elapsed=2404),
        _workflow("MacOS", "success", 9903, seconds=1688),
        _workflow("Android", "success", 9904, seconds=1324),
        _workflow("iOS", "success", 9907, seconds=2314),
    ]
    precommit = _workflow("pre-commit", "success", 9905, seconds=110)
    analysis = _workflow("Code Analysis", "success", 9906, seconds=372)
    others = list(OTHERS)
    env = {
        "BUILD_STATUS_JSON": "build-status.json",
        "REPO": "mavlink/qgroundcontrol",
        "PR_NUMBER": "15370",
        "COMMIT_SHA": HEAD,
        "BASE_SHA": BASE,
        "TRIGGERED_BY": "Android",
    }

    if scenario == "failures":
        failing_unit = [
            ("MissionControllerTest", "failed", 95.2, "Failed", QT_FAILURE),
            ("FTPManagerTest", "failed", 120.0, "Timeout", TIMEOUT_OUTPUT),
            *SLOW[1:3],
            *_passing("Unit", 373),
        ]
        files[
            "artifacts/test-results-linux-coverage-unit/junit-results-linux-coverage-unit.xml"
        ] = junit_xml(failing_unit, 131)
        files["artifacts/test-results-linux-sanitizers-unit/junit.xml"] = junit_xml(
            [
                ("VehicleLinkManagerTest", "failed", 7.5, "Failed", ASAN_FAILURE),
                *_passing("Unit", 378),
            ],
            150,
        )
        files["artifacts/pre-commit-results/pre-commit-results.json"] = json.dumps(
            {
                "exit_code": "1",
                "passed": "39",
                "failed": "2",
                "skipped": "17",
                "failed_hooks": ["clang-format", "ruff check"],
                "run_url": f"{RUNS}/9905",
            }
        )
        del files["artifacts/test-results-portable-windows/junit.xml"]
        files["artifacts/download-manifest.json"] = json.dumps(
            [
                *json.loads(files["artifacts/download-manifest.json"]),
                {"name": "test-results-portable-windows-arm64", "workflow": "Windows"}
                | {"run_id": 1, "downloaded": False},
            ]
        )
        files["pr-sizes.json"] = _sizes(
            HEAD,
            [
                ("QGroundControl-aarch64.AppImage", "Linux", 192_280_813),
                ("QGroundControl-x86_64.AppImage", "Linux", 207_502_131),
                ("QGroundControl.dmg", "MacOS", 351_272_960),
                ("QGroundControl-linux-arm64-v8a.apk", "Android", 89_128_960),
            ],
            [],
        )
        platforms[0] = _workflow(
            "Linux", "failure", 9901, seconds=1655, jobs=_linux_jobs(9901, failed=True)
        )
        platforms[1] = _workflow(
            "Windows",
            "failure",
            9902,
            seconds=941,
            jobs=(
                _job("changes / Detect Changes", 9902, 1, seconds=10),
                _job(
                    "Build x64 Release",
                    9902,
                    2,
                    conclusion="failure",
                    seconds=905,
                    failed_steps=(("Build", 9),),
                ),
                _job(
                    "Build arm64 Release",
                    9902,
                    4,
                    conclusion="failure",
                    seconds=155,
                    runner_lost={
                        "step": "Build Setup",
                        "started_at": "2026-02-17T11:02:56Z",
                        "completed_at": "2026-02-17T11:05:31Z",
                    },
                ),
                _job("Windows", 9902, 3, conclusion="failure", seconds=3),
            ),
        )
        precommit = _workflow(
            "pre-commit",
            "failure",
            9905,
            seconds=131,
            jobs=(
                _job(
                    "pre-commit",
                    9905,
                    1,
                    conclusion="failure",
                    seconds=125,
                    failed_steps=(("Run pre-commit", 9),),
                ),
            ),
        )
        others[0] = _workflow(
            "CI Scripts",
            "failure",
            9911,
            seconds=95,
            jobs=(
                _job(
                    "Test CI Scripts (py3.12)",
                    9911,
                    1,
                    conclusion="failure",
                    failed_steps=(("Run CI script tests", 8),),
                ),
            ),
        )
        env["TRIGGERED_BY"] = "Linux"
    elif scenario == "pending":
        for name in [
            *(path for path in files if "linux" in path or "windows" in path),
            "artifacts/coverage-report/coverage-summary.json",
        ]:
            files.pop(name, None)
        files = {name: text for name, text in files.items() if not name.startswith("analysis-")}
        files["pr-sizes.json"] = _sizes(HEAD, [("QGroundControl.dmg", "MacOS", 351_272_960)], [])
        files["base-sizes.json"] = _sizes(
            BASE,
            [("QGroundControl.dmg", "MacOS", 351_400_000)],
            [*BASE_RUNS[:2], _base_run("MacOS", 8803, status="in_progress", conclusion="")],
        )
        platforms = [
            _workflow("Linux", "running", 9901),
            _workflow("Windows", "pending", None),
            platforms[2],
            _workflow(
                "Android",
                "skipped",
                9904,
                seconds=21,
                jobs=(
                    _job("changes / Detect Changes", 9904, 1, seconds=11),
                    _job("Android (${{ matrix.host }})", 9904, 2, conclusion="skipped", seconds=0),
                    _job("Android", 9904, 3, seconds=3),
                ),
            ),
            _workflow("iOS", "running", 9907),
        ]
        analysis = _workflow("Code Analysis", "running", 9906)
        others[2] = _workflow("Docker", "running", 9913)
        env["TRIGGERED_BY"] = "MacOS"
    elif scenario == "no-base":
        # The PR base's own builds have not run (or their artifacts expired).
        files = {name: text for name, text in files.items() if not name.startswith("base-")}
        files["base-sizes.json"] = _sizes(BASE, [], [])
    elif scenario == "truncated":
        many_failures = [
            (f"Flaky{index:02d}Test", "failed", 2.0, "Failed", QT_FAILURE) for index in range(25)
        ]
        files[
            "artifacts/test-results-linux-coverage-unit/junit-results-linux-coverage-unit.xml"
        ] = junit_xml([*many_failures, *_passing("Unit", 300)], 140)
        for index in range(30):
            files[f"artifacts/test-results-extra{index:02d}-unit/junit.xml"] = junit_xml(
                _passing("Extra", 3), 2
            )
        files["pr-sizes.json"] = _sizes(
            HEAD,
            [
                (f"QGroundControl-{index:02d}.zip", "Linux", (40 + index) * MIB)
                for index in range(40)
            ],
            [],
        )
        files["base-sizes.json"] = _sizes(
            BASE,
            [(f"QGroundControl-{index:02d}.zip", "Linux", 40 * MIB) for index in range(40)],
            BASE_RUNS,
        )
        platforms[0] = _workflow(
            "Linux", "failure", 9901, seconds=1655, jobs=_linux_jobs(9901, failed=True)
        )
    elif scenario != "all-passed":
        raise ValueError(scenario)

    status = {
        "repo": "mavlink/qgroundcontrol",
        "head_sha": HEAD,
        "platforms": platforms,
        "precommit": precommit,
        "analysis": analysis,
        "others": others,
    }
    files["build-status.json"] = json.dumps(status)
    write_files(base, files)
    return env


SCENARIOS = ["all-passed", "failures", "pending", "no-base", "truncated"]


@pytest.mark.parametrize("scenario", SCENARIOS)
def test_rendered_comment_matches_snapshot(
    tmp_path: Path, scenario: str, snapshot: SnapshotAssertion
) -> None:
    """Review layout changes in the .ambr diff; refresh with --snapshot-update."""
    env = write_scenario(tmp_path, scenario)
    comment = generate_comment(env, tmp_path, now_utc=NOW)
    assert len(comment) < mod.MAX_COMMENT_CHARS
    assert comment == snapshot


def _verdict(comment: str) -> str:
    return comment.splitlines()[2]


def _status(tmp_path: Path) -> dict[str, object]:
    return json.loads((tmp_path / "build-status.json").read_text())


def _write_status(tmp_path: Path, status: dict[str, object]) -> None:
    (tmp_path / "build-status.json").write_text(json.dumps(status))


def test_verdicts(tmp_path: Path) -> None:
    passed = generate_comment(write_scenario(tmp_path / "a", "all-passed"), tmp_path / "a", NOW)
    assert _verdict(passed).startswith("✅ **All builds passed** · [`0123456`](")
    assert _verdict(passed).endswith("· 5 of 5 platform workflows done")
    failed = generate_comment(write_scenario(tmp_path / "f", "failures"), tmp_path / "f", NOW)
    assert _verdict(failed).startswith(
        "❌ **4 workflows failed** (Linux, Windows, pre-commit, CI Scripts)"
    )
    pending = generate_comment(write_scenario(tmp_path / "p", "pending"), tmp_path / "p", NOW)
    assert _verdict(pending).startswith("⏳ **Builds in progress**")
    assert _verdict(pending).endswith(
        "2 of 5 platform workflows done (1 skipped by change detection, 3 pending)"
        " · 1 other check still running"
    )


@pytest.mark.parametrize(
    ("states", "verdict"),
    [
        (["skipped", "skipped"], "⏭️ **No platform builds needed**"),
        (["success", "cancelled"], "🚫 **Builds cancelled**"),
        (["success", "action_required"], "⏳ **Builds in progress**"),
        (["success", "skipped"], "✅ **All builds passed**"),
    ],
)
def test_verdict_for_skipped_cancelled_and_unapproved_runs(
    tmp_path: Path, states: list[str], verdict: str
) -> None:
    status = {
        "platforms": [
            _workflow(f"P{index}", state, 9000 + index) for index, state in enumerate(states)
        ],
        "precommit": _workflow("pre-commit", "success", 9905),
    }
    write_files(tmp_path, {"build-status.json": json.dumps(status)})
    comment = generate_comment({"COMMIT_SHA": HEAD}, tmp_path, NOW)
    assert _verdict(comment).startswith(verdict)


def test_a_failed_other_check_fails_the_verdict(tmp_path: Path) -> None:
    env = write_scenario(tmp_path, "all-passed")
    status = _status(tmp_path)
    status["others"] = [
        _workflow("Docker", "running", 9913),
        _workflow("CI Scripts", "failure", 9911),
        *OTHERS[1:2],
        _workflow("ClusterFuzzLite", "skipped", 9914),
    ]
    _write_status(tmp_path, status)

    comment = generate_comment(env, tmp_path, NOW)

    assert _verdict(comment).startswith("❌ **1 workflow failed** (CI Scripts)")
    assert "All builds passed" not in comment
    assert (
        f"**Other checks:** ❌ [CI Scripts]({RUNS}/9911) · 🔄 [Docker]({RUNS}/9913)"
        " · ✅ CodeQL · ⏭️ ClusterFuzzLite"
    ) in comment
    # Without its jobs, the failed run is still linked from the failures.
    assert f"- **CI Scripts**: failed · [run]({RUNS}/9911)" in comment


def test_other_checks_still_running_do_not_hide_a_pass(tmp_path: Path) -> None:
    env = write_scenario(tmp_path, "all-passed")
    status = _status(tmp_path)
    status["others"] = [_workflow("Docker", "running", 9913), *OTHERS[:2]]
    _write_status(tmp_path, status)

    verdict = _verdict(generate_comment(env, tmp_path, NOW))

    assert verdict.startswith("✅ **All builds passed**")
    assert verdict.endswith("· 1 other check still running")


def test_code_analysis_is_a_gating_trigger(tmp_path: Path) -> None:
    env = write_scenario(tmp_path, "all-passed")
    status = _status(tmp_path)
    status["analysis"] = _workflow("Code Analysis", "running", 9906)
    _write_status(tmp_path, status)
    assert _verdict(generate_comment(env, tmp_path, NOW)).startswith("⏳ **Builds in progress**")


def test_failures_list_failed_steps_lost_runners_tests_and_hooks_first(tmp_path: Path) -> None:
    comment = generate_comment(write_scenario(tmp_path, "failures"), tmp_path, NOW)
    failures = comment.index("### ❌ Failures")
    assert failures < comment.index("<details><summary>Tests:")
    assert (
        "- **Linux** · Test + Coverage linux\\_gcc\\_64 Debug: failed at "
        f"[`Run and report tests`]({RUNS}/9901/job/2#step:14:1) · [log]({RUNS}/9901/job/2)"
    ) in comment
    # The gate job fails with every leg, so only the legs are listed.
    assert "Require every leg to pass" not in comment
    assert (
        f"- **Windows** · Build x64 Release: failed at [`Build`]({RUNS}/9902/job/2#step:9:1)"
        in comment
    )
    assert (
        "- **Windows** · Build arm64 Release: runner lost during `Build Setup` "
        f"(started 11:02 UTC, ended 11:05 UTC), no logs; re-run · [log]({RUNS}/9902/job/4)"
    ) in comment
    assert "- **pre-commit** · failed hooks: `clang-format`, `ruff check`" in comment
    assert (
        "- **CI Scripts** · Test CI Scripts \\(py3.12\\): failed at "
        f"[`Run CI script tests`]({RUNS}/9911/job/1#step:8:1)"
    ) in comment
    assert "**Failed tests** (3 of " in comment
    assert "FAIL!  : MissionControllerTest::_testGimbalRecalc()" in comment
    assert "   Loc: [src/MissionManager/MissionControllerTest.cc(412)]" in comment
    assert "==4242==ERROR: AddressSanitizer: heap-use-after-free" in comment
    # A failure without QtTest or sanitizer reports shows its message and output tail.
    assert "Timeout\nPASS   : FTPManagerTest::initTestCase()" in comment


def test_failed_downloads_are_named(tmp_path: Path) -> None:
    comment = generate_comment(write_scenario(tmp_path, "failures"), tmp_path, NOW)
    assert (
        "⚠️ Could not download `test-results-portable-windows-arm64` (Windows); "
        "the results below leave them out."
    ) in comment


def test_expired_base_artifacts_are_not_failed_downloads(tmp_path: Path) -> None:
    env = write_scenario(tmp_path, "all-passed")
    manifest = tmp_path / "base-artifacts" / "download-manifest.json"
    entries = json.loads(manifest.read_text(encoding="utf-8"))
    entries.append(
        {"name": "test-results-windows-unit", "workflow": "Windows", "run_id": 802}
        | {"downloaded": False, "expired": True}
    )
    manifest.write_text(json.dumps(entries), encoding="utf-8")

    comment = generate_comment(env, tmp_path, NOW)
    assert "Could not download" not in comment
    assert (
        "⌛ Past artifact retention: `test-results-windows-unit` (Windows, base); "
        "comparisons with them are left out."
    ) in comment


def test_duration_shows_the_attempt_and_total_wall_time(tmp_path: Path) -> None:
    comment = generate_comment(write_scenario(tmp_path, "all-passed"), tmp_path, NOW)
    assert "| Windows | ✅ Passed | 20m 02s (attempt 2, 40m 04s total) |" in comment
    assert "| Linux | ✅ Passed | 26m 50s | " in comment


def test_precommit_details_appear_only_for_failures(tmp_path: Path) -> None:
    passed = generate_comment(write_scenario(tmp_path / "a", "all-passed"), tmp_path / "a", NOW)
    assert "| pre-commit | ✅ Passed | 1m 50s | hooks: 41 passed, 17 skipped |" in passed
    assert "<summary>Pre-commit" not in passed
    failed = generate_comment(write_scenario(tmp_path / "f", "failures"), tmp_path / "f", NOW)
    assert "<details><summary>Pre-commit: 39 passed, 2 failed, 17 skipped</summary>" in failed


def test_untrusted_names_cannot_inject_markup_or_mentions(tmp_path: Path) -> None:
    name = "Evil<img src=x>@octocat[x](https://bad.test)`Test"
    write_files(
        tmp_path,
        {
            "artifacts/test-results-evil/junit.xml": junit_xml(
                [(name, "failed", 1.0, "", "FAIL!  : ```\n   @octocat <script>")], 1
            ),
            "artifacts/pre-commit-results/pre-commit-results.json": json.dumps(
                {"failed": "1", "failed_hooks": ["@octocat"], "run_url": "javascript:alert(1)"}
            ),
            "build-status.json": json.dumps(
                {
                    "platforms": [],
                    "precommit": _workflow("pre-commit", "failure", 9905),
                    "others": [{"name": "@octocat<b>", "state": "failure", "url": "javascript:1"}],
                }
            ),
        },
    )
    comment = generate_comment({}, tmp_path, NOW)
    # Outside code spans and blocks, markup is escaped and mentions are broken. Escaped
    # backticks are prose, so they must not pair with a code span's backtick.
    prose = re.sub(
        r"(`{2,})(?:(?!\1).)+\1|`[^`]+`",
        "",
        re.sub(r"(?s)````text.*?````", "", comment.replace("\\`", "")),
    )
    assert "<img" not in prose
    assert "<script>" not in prose
    assert "<b>" not in prose
    assert "@octocat" not in prose
    assert "Evil&lt;img src=x&gt;@\u200boctocat\\[x\\]\\(https://bad.test\\)\\`Test" in comment
    assert "](https://bad.test)" not in prose
    assert "javascript:" not in comment
    # Code keeps the text verbatim behind a fence longer than any backtick run in it.
    assert "````text\nFAIL!  : ```\n   @octocat <script>\n````" in comment
    assert "Failed: `@octocat`" in comment


def test_comment_shrinks_lists_to_fit_the_size_limit(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    env = write_scenario(tmp_path, "truncated")
    full = generate_comment(env, tmp_path, NOW)
    monkeypatch.setattr(mod, "MAX_COMMENT_CHARS", len(full) - 1)
    smaller = generate_comment(env, tmp_path, NOW)
    assert len(smaller) < len(full)
    assert "…and 20 more" in smaller  # 25 failed tests, 5 shown
    monkeypatch.setattr(mod, "MAX_COMMENT_CHARS", 2_000)
    clipped = generate_comment(env, tmp_path, NOW)
    assert len(clipped) <= 2_000
    assert clipped.endswith(
        "…comment truncated to fit GitHub's size limit; see the workflow runs.\n"
    )


def test_unreadable_junit_is_skipped(tmp_path: Path, caplog: pytest.LogCaptureFixture) -> None:
    write_files(tmp_path, {"artifacts/test-results-bad/junit.xml": "<testsuite"})
    comment = generate_comment({}, tmp_path, NOW)
    assert "Failed to parse JUnit results" in caplog.text
    assert "<details><summary>Tests:" not in comment


def test_tests_added_and_removed_versus_the_base_build(tmp_path: Path) -> None:
    comment = generate_comment(write_scenario(tmp_path, "all-passed"), tmp_path, NOW)
    assert (
        "<details><summary>Tests: 1,327 passed, 0 failed, 1 skipped in 7 suites; "
        "1 added, 1 removed vs base</summary>"
    ) in comment
    assert "| `linux-coverage-unit` | Linux | 378 | 0 | 0 | 1m 23s | +1 -1 |" in comment
    assert "| `macos-unit` | MacOS | 378 | 0 | 1 | 1m 36s | same |" in comment
    # The base build has no sanitizers results, so that suite is not compared.
    assert "| `linux-sanitizers-unit` | Linux | 378 | 0 | 0 | 2m 21s | — |" in comment
    assert "Added: `FTPManagerTest` (linux-coverage-unit)" in comment
    assert "Removed: `RetiredLinkTest` (linux-coverage-unit)" in comment
    assert mod.compare_tests([], []) is None


def test_coverage_counts_src_only_against_the_base_build(tmp_path: Path) -> None:
    comment = generate_comment(write_scenario(tmp_path, "all-passed"), tmp_path, NOW)
    # src/ covers 66,120 of 99,400 lines; with test/ the total would read 69.9%.
    assert "<details><summary>Code coverage (src/): 66.5% of lines (+0.06 pp vs base)" in comment
    assert "| Lines | 66,120 of 99,400 | 66.5% | 66.5% | +0.06 pp |" in comment
    assert "Base: `aaaaaaa` (Linux build)." in comment
    assert (
        "Coverage of the changed lines: "
        "[Codecov](https://app.codecov.io/gh/mavlink/qgroundcontrol/pull/15370)."
    ) in comment
    no_base = generate_comment(write_scenario(tmp_path / "n", "no-base"), tmp_path / "n", NOW)
    assert "<summary>Code coverage (src/): 66.5% of lines</summary>" in no_base
    assert "No coverage report for base `aaaaaaa`, so changes are not shown." in no_base


def test_base_line_names_each_platforms_base_run(tmp_path: Path) -> None:
    comment = generate_comment(write_scenario(tmp_path, "all-passed"), tmp_path, NOW)
    assert (
        "**Base:** [`aaaaaaa`](https://github.com/mavlink/qgroundcontrol/commit/" + BASE + ") · "
        f"Linux [#801]({RUNS}/8801) · Windows [#802]({RUNS}/8802) · "
        f"MacOS [#803]({RUNS}/8803) (failure) · Android [#804]({RUNS}/8804) · iOS [#807]({RUNS}/8807)"
    ) in comment
    pending = generate_comment(write_scenario(tmp_path / "p", "pending"), tmp_path / "p", NOW)
    assert (
        f"MacOS [#803]({RUNS}/8803) (still running) · Android: no build · iOS: no build" in pending
    )
    no_base = generate_comment(write_scenario(tmp_path / "n", "no-base"), tmp_path / "n", NOW)
    assert "has no builds of its own yet, so changes are not shown." in no_base
    (tmp_path / "n" / "base-sizes.json").unlink()
    unread = generate_comment(write_scenario(tmp_path / "n", "no-base"), tmp_path / "n", NOW)
    assert "its builds could not be read" not in unread
    (tmp_path / "n" / "base-sizes.json").unlink()
    unread = generate_comment({"BASE_SHA": BASE, "REPO": "o/r"}, tmp_path / "n", NOW)
    assert "**Base:** [`aaaaaaa`](https://github.com/o/r/commit/" in unread
    assert "its builds could not be read, so changes are not shown." in unread
    assert "**Base:**" not in generate_comment({}, tmp_path / "n", NOW)


@pytest.mark.parametrize(
    ("delta", "text"),
    [
        (0, "unchanged"),
        (4, "+4 B"),
        (-4_096, "-4.0 KB"),
        (19_865, "+19.4 KB"),
        (4_413_936, "+4.21 MB"),
        (-3 * MIB, "-3.00 MB"),
    ],
)
def test_size_changes_use_human_units(delta: int, text: str) -> None:
    assert mod._delta_size(delta) == text


def test_sizes_link_downloads_and_show_only_changes_in_the_table(tmp_path: Path) -> None:
    comment = generate_comment(write_scenario(tmp_path, "all-passed"), tmp_path, NOW)
    # Linux: +4,096 B and unchanged; Android's single-ABI APK has no same-named base.
    assert "| Linux | ✅ Passed | 26m 50s | 878 passed | +4.0 KB |" in comment
    assert "| Android | ✅ Passed | 22m 04s | — | — |" in comment
    assert (
        f"| [`QGroundControl-installer-AMD64.exe`]({RUNS}/9902/artifacts/5003) | Windows "
        "| 98.00 MB | +59.0 KB | +0.06% | 2026-02-24 |"
    ) in comment
    assert "<details><summary>Artifacts: 6 downloads, first expires 2026-02-24</summary>" in comment
    assert "Downloading needs a GitHub sign-in." in comment


def test_android_abi_variants_are_not_reported_as_size_reductions(tmp_path: Path) -> None:
    write_files(
        tmp_path,
        {
            "pr-sizes.json": _sizes(
                HEAD,
                [
                    ("QGroundControl-linux-arm64-v8a", "Android", 85 * MIB),
                    ("QGroundControl-mac-arm64-v8a", "Android", 85 * MIB),
                ],
                [],
            ),
            "base-sizes.json": _sizes(
                BASE,
                [
                    ("QGroundControl-linux-arm64-v8a-armeabi-v7a", "Android", 162 * MIB),
                    ("QGroundControl-mac-arm64-v8a", "Android", 87 * MIB),
                ],
                [_base_run("Android", 8804)],
            ),
        },
    )
    out = generate_comment({"BASE_SHA": BASE}, tmp_path, NOW)
    assert "| `QGroundControl-linux-arm64-v8a` | Android | 85.00 MB | — | — | 2026-02-24 |" in out
    assert (
        "| `QGroundControl-mac-arm64-v8a` | Android | 85.00 MB | -2.00 MB | -2.30% | 2026-02-24 |"
        in out
    )
    assert "Changes are against base `aaaaaaa`" in out


@pytest.mark.parametrize("base_sha", ["older-master", None])
def test_size_delta_rejects_another_commits_sizes(
    tmp_path: Path, base_sha: str | None, caplog: pytest.LogCaptureFixture
) -> None:
    baseline: dict[str, object] = {
        "artifacts": [{"name": "QGroundControl", "size_bytes": 78 * MIB}]
    }
    if base_sha is not None:
        baseline["head_sha"] = base_sha
    write_files(
        tmp_path,
        {
            "pr-sizes.json": json.dumps(
                {"artifacts": [{"name": "QGroundControl", "size_bytes": MIB}]}
            ),
            "base-sizes.json": json.dumps(baseline),
        },
    )
    out = generate_comment({"BASE_SHA": BASE}, tmp_path, NOW)
    assert "Ignoring base sizes" in caplog.text
    assert "| `QGroundControl` | — | 1.00 MB | — | — | — |" in out
    assert "No artifacts of base `aaaaaaa` to compare with." in out


def test_flagged_growth_is_called_out(tmp_path: Path) -> None:
    comment = generate_comment(write_scenario(tmp_path, "failures"), tmp_path, NOW)
    # x86_64 grew by 4.2 MB (2.2%).
    assert "| Linux | ❌ Failed | 27m 35s | 875 passed, **3 failed** | ⚠️ +4.21 MB |" in comment
    assert (
        "<summary>⚠️ Artifacts: 4 downloads, first expires 2026-02-24; 1 grew</summary>" in comment
    )


def test_analysis_reports_warnings_on_changed_lines_once_across_shards(tmp_path: Path) -> None:
    write_files(tmp_path, DIAGNOSTICS)
    warnings, changed = load_unique_warnings(tmp_path)
    assert {tool: len(found) for tool, found in warnings.items()} == {"clang-tidy": 4, "clazy": 1}
    assert sorted((w.file, w.line, w.check) for w in changed["clang-tidy"]) == [
        ("src/MissionManager/MissionController.cc", 7, "modernize-use-nodiscard"),
        ("src/Vehicle/Vehicle.h", 10, "modernize-use-nodiscard"),
    ]
    assert changed["clazy"] == set()

    comment = generate_comment(write_scenario(tmp_path, "all-passed"), tmp_path, now_utc=NOW)
    assert "| **2 warnings on changed lines** |" in comment
    assert (
        "<details><summary>Code Analysis: ✅ Passed, 2 warnings on changed lines</summary>"
        in comment
    )
    assert (
        f"- [`src/Vehicle/Vehicle.h:10`]({BLOB}/{MERGE}/src/Vehicle/Vehicle.h#L10) "
        "`modernize-use-nodiscard` (clang-tidy)"
    ) in comment
    # Every warning counts only in the collapsed table, never in the main table.
    assert "| `clang-tidy` | 75 | 2 | 4 | 0 |" in comment
    assert "unique warnings in" not in comment
    assert "Most frequent checks" not in comment


@pytest.mark.parametrize(
    "commits",
    [
        [None, None, None],
        [MERGE, MERGE, BASE],
        ["main", None, None],
        [MERGE.upper(), None, None],
        [f"{MERGE}/../../x", None, None],
    ],
)
def test_warning_links_fall_back_to_the_head_without_one_analyzed_commit(
    tmp_path: Path, commits: list[str | None]
) -> None:
    env = write_scenario(tmp_path, "all-passed")
    paths = sorted(tmp_path.glob("analysis-artifacts/**/*-diagnostics.json"))
    for path, commit in zip(paths, commits, strict=True):
        report = json.loads(path.read_text())
        report["commit"] = commit
        if commit is None:
            del report["commit"]
        path.write_text(json.dumps(report))
    comment = generate_comment(env, tmp_path, now_utc=NOW)
    assert f"({BLOB}/{HEAD}/src/Vehicle/Vehicle.h#L10)" in comment
    assert MERGE not in comment


def test_warnings_without_a_known_commit_are_not_linked(tmp_path: Path) -> None:
    env = write_scenario(tmp_path, "all-passed")
    status = json.loads((tmp_path / "build-status.json").read_text())
    del status["head_sha"]
    (tmp_path / "build-status.json").write_text(json.dumps(status))
    for path in tmp_path.glob("analysis-artifacts/**/*-diagnostics.json"):
        report = json.loads(path.read_text())
        del report["commit"]
        path.write_text(json.dumps(report))
    comment = generate_comment({**env, "COMMIT_SHA": ""}, tmp_path, now_utc=NOW)
    assert "- `src/Vehicle/Vehicle.h:10` `modernize-use-nodiscard` (clang-tidy)" in comment
    assert "/blob/" not in comment


def test_analysis_without_changed_lines_says_so(tmp_path: Path) -> None:
    env = write_scenario(tmp_path, "all-passed")
    for path in tmp_path.glob("analysis-artifacts/**/*-diagnostics.json"):
        report = json.loads(path.read_text())
        report["changed"] = {}
        path.write_text(json.dumps(report))
    comment = generate_comment(env, tmp_path, now_utc=NOW)
    assert "| no warnings on changed lines |" in comment
    assert "No warnings on lines this pull request changed." in comment


def test_analysis_from_older_artifacts_shows_only_totals(tmp_path: Path) -> None:
    env = write_scenario(tmp_path, "all-passed")
    for path in tmp_path.glob("analysis-artifacts/**/*-diagnostics.json"):
        report = json.loads(path.read_text())
        del report["changed"]
        path.write_text(json.dumps(report))
    comment = generate_comment(env, tmp_path, now_utc=NOW)
    assert "| 76 files analyzed |" in comment
    assert "| `clang-tidy` | 75 | 4 | 0 |" in comment
    assert "on changed lines" not in comment
    for path in tmp_path.glob("analysis-artifacts/**/*-diagnostics.json"):
        path.unlink()
    without = generate_comment(env, tmp_path, now_utc=NOW)
    assert "| `clazy` | 76 | 1 | 0 |" in without
    assert "Warnings are advisory." in without


def test_malformed_and_empty_size_snapshots(tmp_path: Path) -> None:
    write_files(
        tmp_path,
        {
            "pr-sizes.json": json.dumps(
                {
                    "artifacts": [
                        {"name": "QGroundControl.dmg", "size_bytes": MIB},
                        {"name": "", "size_bytes": 10},
                        {"name": "bad", "size_bytes": "x"},
                        "bad-entry",
                    ]
                }
            )
        },
    )
    assert "| `QGroundControl.dmg` | — | 1.00 MB | — | — | — |" in generate_comment(
        {}, tmp_path, NOW
    )
    write_files(tmp_path, {"pr-sizes.json": json.dumps({"head_sha": "new", "artifacts": []})})
    assert "Artifacts:" not in generate_comment({}, tmp_path, NOW)


def test_missing_status_still_renders(tmp_path: Path) -> None:
    comment = generate_comment({"TRIGGERED_BY": "Linux"}, tmp_path, NOW)
    assert comment.startswith("## Build Results\n\n✅ **All builds passed**\n\n| Workflow |")
    assert "| pre-commit | ⚪ Not run | — | — | — | — |" in comment
    assert comment.endswith("<sub>Updated 2026-02-17 12:00 UTC · triggered by Linux</sub>\n")


def test_footer_defaults_to_the_triggering_run_and_collected_commit(tmp_path: Path) -> None:
    event = tmp_path / "event.json"
    event.write_text(json.dumps({"workflow_run": {"name": "iOS", "head_sha": "other"}}))
    status = {"repo": "owner/repo", "head_sha": HEAD, "platforms": []}
    write_files(tmp_path, {"build-status.json": json.dumps(status)})
    env = {"GITHUB_EVENT_PATH": str(event)}

    footer = generate_comment(env, tmp_path, NOW).rsplit("\n\n", 1)[-1]
    assert f"for [`0123456`](https://github.com/owner/repo/commit/{HEAD})" in footer
    assert "triggered by iOS" in footer
    assert "triggered by Linux" in generate_comment({**env, "TRIGGERED_BY": "Linux"}, tmp_path, NOW)
    assert "triggered by unknown" in generate_comment({}, tmp_path, NOW)


def test_input_paths_resolve_against_base_dir_with_env_overrides(tmp_path: Path) -> None:
    status = {"platforms": [_workflow("Linux", "failure", 9001)]}
    write_files(tmp_path, {"elsewhere/status.json": json.dumps(status)})
    assert "❌" not in generate_comment({}, tmp_path, NOW)
    assert "❌" in generate_comment({"BUILD_STATUS_JSON": "elsewhere/status.json"}, tmp_path, NOW)


def test_main_writes_comment_and_job_summary(
    tmp_path: Path, gh_summary: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    for key, value in write_scenario(tmp_path, "all-passed").items():
        monkeypatch.setenv(key, value)
    output = tmp_path / "comment.md"
    assert mod.main(["--base-dir", str(tmp_path), "--output", str(output)]) == 0
    assert output.read_text(encoding="utf-8").startswith("## Build Results\n")
    assert gh_summary.read_text(encoding="utf-8") == output.read_text(encoding="utf-8")
