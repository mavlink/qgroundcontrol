"""GPS Standalone workflow contracts: what triggers it, and how it fuzzes."""

from __future__ import annotations

import re

from ..._helpers import REPO_ROOT, get_step, load_workflow

WORKFLOW = load_workflow("gps-standalone.yml")
JOB = WORKFLOW["jobs"]["standalone"]
STEPS = JOB["steps"]
ARTIFACTS = "gps-fuzz-artifacts"


def _source_directories() -> list[str]:
    """Directories under src/ that the standalone project adds."""
    project = (REPO_ROOT / "test/GPS/Standalone/CMakeLists.txt").read_text(encoding="utf-8")
    (listed,) = re.findall(r"foreach\(\s*directory\s+([^)]+)\)", project)
    return [f"src/{directory}" for directory in listed.split()]


def _harnesses() -> set[str]:
    """Targets under test/GPS that link libFuzzer's driver."""
    return {
        target
        for path in (REPO_ROOT / "test/GPS").rglob("CMakeLists.txt")
        for target in re.findall(
            r"target_link_options\(\s*(\w+)\s+PRIVATE\s+-fsanitize=fuzzer\s*\)",
            path.read_text(encoding="utf-8"),
        )
    }


def test_standalone_build_inputs_trigger_the_workflow() -> None:
    on = WORKFLOW["on"]
    paths = on["pull_request"]["paths"]
    assert on["push"]["paths"] == paths
    directories = _source_directories()
    assert {"src/GPS/Core", "src/GPS/NTRIP/HTTP", "src/Utilities/Math"} <= set(directories)
    for source in [
        *directories,
        "test/GPS/Standalone",
        # The project reads its minimum Qt version there, and qt-install the version to install.
        ".github/build-config.json",
        ".github/workflows/gps-standalone.yml",
    ]:
        assert any(
            source == path or (path.endswith("/**") and f"{source}/".startswith(path[:-2]))
            for path in paths
        ), f"{source} is part of the standalone build but does not trigger gps-standalone.yml"


def test_each_libfuzzer_harness_fuzzes_briefly_and_keeps_a_failing_input() -> None:
    harnesses = _harnesses()
    assert {"QGCGPSProtocolFuzz", "ntrip_http_decoder_fuzzer"} <= harnesses
    # Without halt_on_error a UBSan report prints and the fuzzer exits 0.
    assert "halt_on_error=1" in JOB["env"]["UBSAN_OPTIONS"].split(":")

    fuzz = [step for step in STEPS if "-max_total_time=" in step.get("run", "")]
    assert len(fuzz) == len(harnesses)
    for harness in sorted(harnesses):
        (step,) = [step for step in fuzz if re.search(rf"/{harness}\s", step["run"])]
        seconds = int(re.findall(r"-max_total_time=(\d+)", step["run"])[0])
        assert 10 <= seconds <= 90
        assert seconds < step["timeout-minutes"] * 60 < JOB["timeout-minutes"] * 60
        # A hang must end as a saved input, well before the step timeout cancels the fuzzer.
        assert int(re.findall(r"-timeout=(\d+)", step["run"])[0]) < seconds
        assert f'-artifact_prefix="${{RUNNER_TEMP}}/{ARTIFACTS}/' in step["run"]
        assert step.get("continue-on-error", False) is False
        # A failed test does not skip the fuzzers.
        assert step["if"] == "${{ !cancelled() && steps.build.outcome == 'success' }}"

    upload = get_step(STEPS, "Upload fuzzing diagnostics")
    assert STEPS.index(upload) > max(STEPS.index(step) for step in fuzz)
    assert upload["uses"].startswith("actions/upload-artifact@")
    assert upload["if"] == "${{ !cancelled() }}"
    assert upload["with"]["path"] == f"${{{{ runner.temp }}}}/{ARTIFACTS}"
    # Passing runs save no input.
    assert upload["with"]["if-no-files-found"] == "ignore"
