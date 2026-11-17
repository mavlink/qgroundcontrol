"""GPS Standalone workflow contracts: what triggers it, how it fuzzes, and what the nightly run keeps."""

from __future__ import annotations

import re

from ci.cache import fuzz_corpus_artifact

from ..._helpers import REPO_ROOT, get_step, load_workflow

WORKFLOW = load_workflow("gps-standalone.yml")
JOB = WORKFLOW["jobs"]["standalone"]
STEPS = JOB["steps"]
ARTIFACTS = "gps-fuzz-artifacts"
CORPUS = "gps-fuzz-corpus"
NIGHTLY = "(github.event_name == 'schedule' || inputs.nightly == true)"
BUILT = "!cancelled() && steps.build.outcome == 'success'"
FUZZ = [step for step in STEPS if "-max_total_time=" in step.get("run", "")]


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


def _nightly_or_else(expression: str, condition: str) -> tuple[int, int]:
    """The nightly run's value and every other run's, of an expression that chooses by *condition*."""
    (values,) = re.findall(
        rf"^\$\{{\{{ {re.escape(condition)} && (\d+) \|\| (\d+) \}}\}}$", expression
    )
    return int(values[0]), int(values[1])


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


def test_each_libfuzzer_harness_fuzzes_and_keeps_a_failing_input() -> None:
    harnesses = _harnesses()
    assert {
        "QGCGPSProtocolFuzz",
        "QGCNTRIPSourceTableFuzz",
        "QGCRTCMStationPositionFuzz",
        "ntrip_http_decoder_fuzzer",
    } <= harnesses
    # Without halt_on_error a UBSan report prints and the fuzzer exits 0.
    assert "halt_on_error=1" in JOB["env"]["UBSAN_OPTIONS"].split(":")

    # A change gets a brief run; the nightly run has the time to go deeper.
    nightly_seconds, seconds = _nightly_or_else(JOB["env"]["FUZZ_SECONDS"], NIGHTLY)
    assert 10 <= seconds <= 90
    assert nightly_seconds >= 10 * seconds
    nightly_job_minutes, job_minutes = _nightly_or_else(JOB["timeout-minutes"], NIGHTLY)
    assert len(FUZZ) == len(harnesses)
    assert len(FUZZ) * seconds < job_minutes * 60
    assert len(FUZZ) * nightly_seconds < nightly_job_minutes * 60

    corpora = set()
    for harness in sorted(harnesses):
        (step,) = [step for step in FUZZ if re.search(rf"/{harness}\s", step["run"])]
        assert f'/{harness} -max_total_time="${{FUZZ_SECONDS}}" ' in step["run"]
        nightly_minutes, minutes = _nightly_or_else(
            step["timeout-minutes"], "env.NIGHTLY == 'true'"
        )
        assert seconds < minutes * 60 < job_minutes * 60
        assert nightly_seconds < nightly_minutes * 60 < nightly_job_minutes * 60
        # A hang must end as a saved input, well before the step timeout cancels the fuzzer.
        assert int(re.findall(r"-timeout=(\d+)", step["run"])[0]) < seconds
        assert f'-artifact_prefix="${{RUNNER_TEMP}}/{ARTIFACTS}/' in step["run"]
        assert step.get("continue-on-error", False) is False
        # A failed test does not skip the fuzzers.
        assert step["if"] == f"${{{{ {BUILT} }}}}"
        # libFuzzer writes the inputs it keeps into the first directory after its options: the
        # harness's own under the corpus the nightly run saves, never its seeds.
        (corpus,) = re.findall(
            rf'-artifact_prefix="\S+"\s+(?:\\\n\s+)?"\$\{{RUNNER_TEMP\}}/{CORPUS}/([a-z0-9-]+)"\s',
            step["run"],
        )
        corpora.add(corpus)
        # The restore drops what its script does not take for a harness's input.
        assert fuzz_corpus_artifact.INPUT_MEMBER.fullmatch(f"{corpus}/{'0' * 40}")
        max_len = int(re.findall(r"-max_len=(\d+)", step["run"])[0])
        assert max_len <= fuzz_corpus_artifact.MAX_INPUT_BYTES
    assert len(corpora) == len(harnesses)

    upload = get_step(STEPS, "Upload fuzzing diagnostics")
    assert STEPS.index(upload) > max(STEPS.index(step) for step in FUZZ)
    assert upload["uses"].startswith("actions/upload-artifact@")
    assert upload["if"] == "${{ !cancelled() }}"
    assert upload["with"]["path"] == f"${{{{ runner.temp }}}}/{ARTIFACTS}"
    # Passing runs save no input.
    assert upload["with"]["if-no-files-found"] == "ignore"


def test_the_nightly_run_starts_from_the_last_one_s_corpus_and_saves_its_own() -> None:
    on = WORKFLOW["on"]
    (schedule,) = on["schedule"]
    minute, hour, *rest = schedule["cron"].split()
    assert minute.isdigit() and hour.isdigit() and rest == ["*", "*", "*"]
    # A manual run can fuzz as the nightly run does, to check a change to it.
    assert on["workflow_dispatch"]["inputs"]["nightly"] == {
        "description": on["workflow_dispatch"]["inputs"]["nightly"]["description"],
        "type": "boolean",
        "default": False,
    }
    assert JOB["env"]["NIGHTLY"] == f"${{{{ {NIGHTLY.strip('()')} }}}}"
    # The restore trusts the events of a nightly run, and only this workflow's runs.
    assert set(on) - {"pull_request", "push"} == fuzz_corpus_artifact.TRUSTED_EVENTS
    assert fuzz_corpus_artifact.PRODUCER_WORKFLOW == ".github/workflows/gps-standalone.yml"
    # Queued behind master's pushes, a nightly run would delay them or be cancelled by the next one: every run but
    # a pull request's has a group of its own.
    assert WORKFLOW["concurrency"]["group"].endswith(
        "${{ github.event_name == 'pull_request' && github.ref || github.run_id }}"
    )

    restore = get_step(STEPS, "Restore the fuzzing corpus")
    assert STEPS.index(restore) < min(STEPS.index(step) for step in FUZZ)
    assert restore["if"] == f"${{{{ {BUILT} && env.NIGHTLY == 'true' }}}}"
    assert restore["run"] == (
        'python3 "${GITHUB_WORKSPACE}/tools/ci/cache/fuzz_corpus_artifact.py" '
        f'--destination "${{RUNNER_TEMP}}/{CORPUS}"'
    )
    # A failed restore warns and the fuzzers start from their seeds.
    assert restore.get("continue-on-error", False) is False
    assert restore["env"] == {"GH_TOKEN": "${{ github.token }}"}
    assert JOB["permissions"] == {"contents": "read", "actions": "read"}

    save = get_step(STEPS, "Save the fuzzing corpus")
    assert STEPS.index(save) > max(STEPS.index(step) for step in FUZZ)
    assert save["uses"].startswith("actions/upload-artifact@")
    # A failed fuzzer's corpus is saved too.
    assert save["if"] == "${{ !cancelled() && env.NIGHTLY == 'true' }}"
    assert save["with"]["name"] == fuzz_corpus_artifact.ARTIFACT_NAME
    assert save["with"]["path"] == f"${{{{ runner.temp }}}}/{CORPUS}"
    # The corpus must outlive a few nights without a run.
    assert save["with"]["retention-days"] >= 7
