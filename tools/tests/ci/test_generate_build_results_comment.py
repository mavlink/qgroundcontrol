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

from .._helpers import write_files
from .test_junit_report import ASAN_FAILURE, QT_FAILURE, TIMEOUT_OUTPUT, junit_xml

if TYPE_CHECKING:
    from pathlib import Path

    from syrupy.assertion import SnapshotAssertion

NOW = datetime(2026, 2, 17, 12, 0, 0, tzinfo=timezone.utc)
HEAD = "0123456789abcdef0123456789abcdef01234567"
BASE = "a" * 40
OLDER_MASTER = "b" * 40
RUNS = "https://github.com/mavlink/qgroundcontrol/actions/runs"
MIB = 1024 * 1024


def _passing(
    prefix: str, count: int, seconds: float = 0.5
) -> list[tuple[str, str, float, str, str]]:
    return [(f"{prefix}{index:03d}Test", "passed", seconds, "", "") for index in range(count)]


SLOW = [
    ("MissionControllerTest", "passed", 95.2, "", ""),
    ("VehicleLinkManagerTest", "passed", 61.0, "", ""),
    ("ParameterManagerTest", "passed", 48.7, "", ""),
    ("FTPManagerTest", "passed", 33.1, "", ""),
    ("InitialConnectTest", "passed", 30.2, "", ""),
    ("TerrainQueryTest", "passed", 22.4, "", ""),
]


def _job(
    name: str,
    run_id: int,
    job_id: int,
    *,
    conclusion: str = "success",
    seconds: float = 600.0,
    failed_steps: tuple[tuple[str, int], ...] = (),
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
    }


def _workflow(
    name: str,
    state: str,
    run_id: int | None,
    *,
    seconds: float | None = None,
    jobs: tuple[dict[str, object], ...] = (),
) -> dict[str, object]:
    return {
        "name": name,
        "state": state,
        "run_id": str(run_id or ""),
        "url": f"{RUNS}/{run_id}" if run_id else "",
        "run_number": run_id - 9000 if run_id else None,
        "duration_seconds": seconds,
        "jobs": list(jobs),
    }


def _coverage(lines: tuple[int, int], functions: tuple[int, int], branches: tuple[int, int]) -> str:
    def pct(covered: int, total: int) -> float:
        return round(100.0 * covered / total, 1) if total else 0.0

    return json.dumps(
        {
            "line_covered": lines[0],
            "line_total": lines[1],
            "line_percent": pct(*lines),
            "function_covered": functions[0],
            "function_total": functions[1],
            "function_percent": pct(*functions),
            "branch_covered": branches[0],
            "branch_total": branches[1],
            "branch_percent": pct(*branches),
        }
    )


def _sizes(head: str, sizes: list[tuple[str, str, int]]) -> str:
    return json.dumps(
        {
            "head_sha": head,
            "artifacts": [
                {"name": name, "workflow": workflow, "size_bytes": size}
                for name, workflow, size in sizes
            ],
        }
    )


def _timings(tool: str, files: int, findings: list[str], errors: list[str] = ()) -> str:  # type: ignore[assignment]
    rows = [{"file": f"src/File{index:03d}.cc", "status": "passed"} for index in range(files)]
    rows += [{"file": name, "status": "findings"} for name in findings]
    rows += [{"file": name, "status": "error"} for name in errors]
    return json.dumps({"tool": tool, "selected_files": len(rows), "files": rows})


def _diagnostics(tool: str, by_file: dict[str, list[list[object]]]) -> str:
    return json.dumps({"tool": tool, "shard": 1, "diagnostics": by_file})


# A header warning repeats across shards; error diagnostics and malformed rows are not warnings.
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
    ),
    "analysis-artifacts/clang-tidy-clazy-output-1/clazy-diagnostics.json": _diagnostics(
        "clazy", {"src/Vehicle/Vehicle.cc": [[5, 1, "warning", "-Wclazy-non-pod-global-static"]]}
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
    ("QGroundControl-aarch64.AppImage", "", 192_190_000),
    ("QGroundControl-x86_64.AppImage", "", 203_088_195),
    ("QGroundControl-installer-AMD64.exe", "", 102_700_000),
    ("QGroundControl.dmg", "", 351_400_000),
    ("QGroundControl-linux-arm64-v8a-armeabi-v7a.apk", "", 170_000_000),
    ("QGroundControl.ipa", "", 81_183_320),
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
    files: dict[str, str] = {
        "artifacts/download-manifest.json": json.dumps(
            [{"name": name, "workflow": workflow, "run_id": 1} for name, workflow in MANIFEST]
        ),
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
        "artifacts/coverage-report/coverage-summary.json": _coverage(
            (151_234, 246_310), (21_480, 29_870), (60_211, 141_502)
        ),
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
        "pr-sizes.json": _sizes(HEAD, PR_SIZES),
        "baseline-sizes.json": _sizes(BASE, BASE_SIZES),
        "baseline-coverage-summary.json": _coverage(
            (150_802, 246_102), (21_431, 29_850), (60_300, 141_480)
        ),
    }
    platforms = [
        _workflow("Linux", "success", 9901, seconds=1610, jobs=_linux_jobs(9901)),
        _workflow("Windows", "success", 9902, seconds=1862),
        _workflow("MacOS", "success", 9903, seconds=1688),
        _workflow("Android", "success", 9904, seconds=1324),
        _workflow("iOS", "success", 9907, seconds=2314),
    ]
    precommit = _workflow("pre-commit", "success", 9905, seconds=110)
    analysis = _workflow("Code Analysis", "success", 9906, seconds=372)
    env = {
        "BUILD_STATUS_JSON": "build-status.json",
        "REPO": "mavlink/qgroundcontrol",
        "PR_NUMBER": "15370",
        "COMMIT_SHA": HEAD,
        "BASELINE_SHA": BASE,
        "BASELINE_KEY": f"build-baseline-v2-{BASE}-41-1",
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
        files["pr-sizes.json"] = _sizes(
            HEAD,
            [
                ("QGroundControl-aarch64.AppImage", "Linux", 192_280_813),
                ("QGroundControl-x86_64.AppImage", "Linux", 207_502_131),
                ("QGroundControl.dmg", "MacOS", 351_272_960),
                ("QGroundControl-linux-arm64-v8a.apk", "Android", 89_128_960),
            ],
        )
        env["BASELINE_KEY"] = f"build-baseline-v2-{OLDER_MASTER}-40-1"
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
        env["TRIGGERED_BY"] = "Linux"
    elif scenario == "pending":
        for name in [
            *(path for path in files if "linux" in path or "windows" in path),
            "artifacts/coverage-report/coverage-summary.json",
            "baseline-sizes.json",
            "baseline-coverage-summary.json",
        ]:
            files.pop(name, None)
        files = {name: text for name, text in files.items() if not name.startswith("analysis-")}
        files["pr-sizes.json"] = _sizes(HEAD, [("QGroundControl.dmg", "MacOS", 351_272_960)])
        del env["BASELINE_KEY"]
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
                    _job("build-matrix", 9904, 1, seconds=11),
                    _job("Android (${{ matrix.host }})", 9904, 2, conclusion="skipped", seconds=0),
                    _job("Android", 9904, 3, seconds=3),
                ),
            ),
            _workflow("iOS", "running", 9907),
        ]
        analysis = _workflow("Code Analysis", "running", 9906)
        env["TRIGGERED_BY"] = "MacOS"
    elif scenario == "no-baseline":
        for name in ["baseline-sizes.json", "baseline-coverage-summary.json"]:
            del files[name]
        del env["BASELINE_KEY"]
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
        )
        files["baseline-sizes.json"] = _sizes(
            BASE, [(f"QGroundControl-{index:02d}.zip", "", 40 * MIB) for index in range(40)]
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
    }
    files["build-status.json"] = json.dumps(status)
    write_files(base, files)
    return env


SCENARIOS = ["all-passed", "failures", "pending", "no-baseline", "truncated"]


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


def test_verdicts(tmp_path: Path) -> None:
    passed = generate_comment(write_scenario(tmp_path / "a", "all-passed"), tmp_path / "a", NOW)
    assert _verdict(passed).startswith("✅ **All builds passed** · [`0123456`](")
    assert _verdict(passed).endswith("· 5 of 5 platform workflows done")
    failed = generate_comment(write_scenario(tmp_path / "f", "failures"), tmp_path / "f", NOW)
    assert _verdict(failed).startswith("❌ **3 workflows failed**")
    pending = generate_comment(write_scenario(tmp_path / "p", "pending"), tmp_path / "p", NOW)
    assert _verdict(pending).startswith("⏳ **Builds in progress**")
    assert _verdict(pending).endswith(
        "2 of 5 platform workflows done (1 skipped by change detection, 3 pending)"
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


def test_failures_list_failed_steps_tests_and_hooks_first(tmp_path: Path) -> None:
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
    assert "- **pre-commit** · failed hooks: `clang-format`, `ruff check`" in comment
    assert "**Failed tests** (3 of " in comment
    assert "FAIL!  : MissionControllerTest::_testGimbalRecalc()" in comment
    assert "   Loc: [src/MissionManager/MissionControllerTest.cc(412)]" in comment
    assert "==4242==ERROR: AddressSanitizer: heap-use-after-free" in comment
    # A failure without QtTest or sanitizer reports shows its message and output tail.
    assert "Timeout\nPASS   : FTPManagerTest::initTestCase()" in comment


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
                }
            ),
        },
    )
    comment = generate_comment({}, tmp_path, NOW)
    # Outside code spans and blocks, markup is escaped and mentions are broken.
    prose = re.sub(
        r"(`{2,})(?:(?!\1).)+\1|`[^`]+`", "", re.sub(r"(?s)````text.*?````", "", comment)
    )
    assert "<img" not in prose
    assert "<script>" not in prose
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


def test_coverage_notes_name_the_baseline(tmp_path: Path) -> None:
    def coverage_note(**env: str) -> str:
        comment = generate_comment(env, tmp_path, NOW)
        section = comment.split("<details><summary>Code coverage")[1]
        return section.split("</details>")[0].strip().splitlines()[-1]

    write_files(
        tmp_path,
        {
            "artifacts/coverage-report/coverage-summary.json": _coverage((5, 10), (0, 0), (1, 4)),
        },
    )
    assert coverage_note().startswith("No coverage baseline yet")
    write_files(tmp_path, {"baseline-coverage-summary.json": _coverage((4, 10), (0, 0), (1, 4))})
    assert coverage_note(BASELINE_SHA=BASE, BASELINE_KEY=f"build-baseline-v2-{BASE}-1-1") == (
        "Baseline: PR base `aaaaaaa`."
    )
    assert coverage_note(
        BASELINE_SHA=BASE, BASELINE_KEY=f"build-baseline-v2-{OLDER_MASTER}-1-1"
    ).startswith("Baseline: `master` at `bbbbbbb`, the newest snapshot")


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
            ),
            "baseline-sizes.json": _sizes(
                BASE,
                [
                    ("QGroundControl-linux-arm64-v8a-armeabi-v7a", "", 162 * MIB),
                    ("QGroundControl-mac-arm64-v8a", "", 87 * MIB),
                ],
            ),
        },
    )
    out = generate_comment({"BASELINE_SHA": BASE}, tmp_path, NOW)
    assert "| `QGroundControl-linux-arm64-v8a` | Android | 85.00 MB | — | — |" in out
    assert "| `QGroundControl-mac-arm64-v8a` | Android | 85.00 MB | -2.00 MB | -2.30% |" in out
    assert "Artifact sizes: 2 artifacts, 170.00 MB, -2.00 MB vs base" in out
    assert "Compared with PR base `aaaaaaa`" in out


@pytest.mark.parametrize("cached_sha", ["older-master", None])
def test_size_delta_rejects_stale_or_unidentified_baseline(
    tmp_path: Path, cached_sha: str | None, caplog: pytest.LogCaptureFixture
) -> None:
    baseline: dict[str, object] = {
        "artifacts": [{"name": "QGroundControl", "size_bytes": 78 * MIB}]
    }
    if cached_sha is not None:
        baseline["head_sha"] = cached_sha
    write_files(
        tmp_path,
        {
            "pr-sizes.json": json.dumps(
                {"artifacts": [{"name": "QGroundControl", "size_bytes": MIB}]}
            ),
            "baseline-sizes.json": json.dumps(baseline),
        },
    )
    out = generate_comment({"BASELINE_SHA": BASE}, tmp_path, NOW)
    assert "Ignoring size baseline" in caplog.text
    assert "| `QGroundControl` | — | 1.00 MB | — | — |" in out
    assert "No size baseline for `aaaaaaa` yet" in out


def test_analysis_counts_each_warning_once_across_shards(tmp_path: Path) -> None:
    write_files(tmp_path, DIAGNOSTICS)
    assert mod.load_analysis_warnings(tmp_path) == {
        "clang-tidy": {
            ("src/Vehicle/Vehicle.h", 10, 5, "modernize-use-nodiscard"),
            ("src/Vehicle/Vehicle.h", 12, 1, "readability-identifier-naming"),
            ("src/Vehicle/Vehicle.cc", 40, 3, "readability-implicit-bool-conversion"),
            ("src/MissionManager/MissionController.cc", 7, 1, "modernize-use-nodiscard"),
        },
        "clazy": {("src/Vehicle/Vehicle.cc", 5, 1, "-Wclazy-non-pod-global-static")},
    }

    comment = generate_comment(write_scenario(tmp_path, "all-passed"), tmp_path, now_utc=NOW)
    assert "| 5 unique warnings in 3 files |" in comment
    assert "| `modernize-use-nodiscard` | clang-tidy | 2 |" in comment
    assert "| `src/Vehicle/Vehicle.cc` | 2 |" in comment


def test_analysis_without_diagnostics_lists_files_with_findings(tmp_path: Path) -> None:
    env = write_scenario(tmp_path, "all-passed")
    for path in tmp_path.glob("analysis-artifacts/**/*-diagnostics.json"):
        path.unlink()

    comment = generate_comment(env, tmp_path, now_utc=NOW)

    assert "| 76 files, 1 with findings |" in comment
    assert "Files with findings (warnings are advisory):" in comment
    assert "unique warnings" not in comment


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
    assert "| `QGroundControl.dmg` | — | 1.00 MB | — | — |" in generate_comment({}, tmp_path, NOW)
    write_files(tmp_path, {"pr-sizes.json": json.dumps({"head_sha": "new", "artifacts": []})})
    assert "Artifact sizes" not in generate_comment({}, tmp_path, NOW)


def test_missing_status_still_renders(tmp_path: Path) -> None:
    comment = generate_comment({"TRIGGERED_BY": "Linux"}, tmp_path, NOW)
    assert comment.startswith("## Build Results\n\n✅ **All builds passed**\n\n| Workflow |")
    assert "| pre-commit | ⚪ Not run | — | — | — | — |" in comment
    assert comment.endswith("<sub>Updated 2026-02-17 12:00 UTC · triggered by Linux</sub>\n")


def test_main_writes_comment_and_job_summary(
    tmp_path: Path, gh_summary: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    for key, value in write_scenario(tmp_path, "all-passed").items():
        monkeypatch.setenv(key, value)
    output = tmp_path / "comment.md"
    assert mod.main(["--base-dir", str(tmp_path), "--output", str(output)]) == 0
    assert output.read_text(encoding="utf-8").startswith("## Build Results\n")
    assert gh_summary.read_text(encoding="utf-8") == output.read_text(encoding="utf-8")
