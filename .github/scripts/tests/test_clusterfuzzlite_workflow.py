"""Fuzzing configuration and PR execution contracts."""

import subprocess
import sys

import pytest
import yaml
from _helpers import REPO_ROOT


def test_pr_fuzzing_uses_seed_corpus_and_preserves_diagnostics() -> None:
    workflow = yaml.safe_load(
        (REPO_ROOT / ".github/workflows/clusterfuzzlite.yml").read_text(encoding="utf-8")
    )
    job = workflow["jobs"]["fuzz"]
    steps = job["steps"]
    run = next(
        step for step in steps if "clusterfuzzlite/actions/run_fuzzers@" in step.get("uses", "")
    )
    assert run["env"]["NO_CLUSTERFUZZ_DEPLOYMENT"] == "true"
    assert run["with"]["fuzz-seconds"] == 300
    assert run["with"]["sanitizer"] == "address"
    assert run["with"]["mode"] == "code-change"
    assert run["with"]["output-sarif"] is True
    assert run["with"].get("dry-run", False) is False
    assert run.get("continue-on-error", False) is False
    assert 5 < run["timeout-minutes"] < job["timeout-minutes"]

    upload = next(
        step for step in steps if step.get("uses", "").startswith("actions/upload-artifact@")
    )
    assert upload["if"] == "${{ !cancelled() }}"
    assert set(upload["with"]["path"].splitlines()) == {
        "out/artifacts",
        "cifuzz-sarif/results.sarif",
    }


@pytest.mark.parametrize("multiline", [False, True])
@pytest.mark.parametrize("key", ["GIT_REPO", "GIT_TAG", "DIALECT", "VERSION"])
def test_mavlink_defaults_survive_cmake_formatting(tmp_path, multiline, key):
    value = "https://example.com/mavlink.git" if key == "GIT_REPO" else "test-value"
    separator = "\n    " if multiline else " "
    config = tmp_path / "CustomOptions.cmake"
    config.write_text(
        f'# set(QGC_MAVLINK_{key} "ignored-comment")\n'
        f'set(QGC_MAVLINK_{key}{separator}"{value}"{separator}CACHE STRING "Description")\n'
    )
    result = subprocess.run(
        [
            sys.executable,
            str(REPO_ROOT / ".clusterfuzzlite/read_mavlink_config.py"),
            str(config),
            key,
        ],
        capture_output=True,
        text=True,
        check=True,
    )
    assert result.stdout.strip() == value


@pytest.mark.parametrize(
    "defaults", ["", 'set(QGC_MAVLINK_DIALECT "")', 'set(QGC_MAVLINK_DIALECT "all")\n' * 2]
)
def test_mavlink_defaults_reject_missing_empty_or_ambiguous_values(tmp_path, defaults):
    config = tmp_path / "CustomOptions.cmake"
    config.write_text(defaults)
    result = subprocess.run(
        [
            sys.executable,
            str(REPO_ROOT / ".clusterfuzzlite/read_mavlink_config.py"),
            str(config),
            "DIALECT",
        ],
        capture_output=True,
        text=True,
        check=False,
    )
    assert result.returncode != 0
    assert "Expected one nonempty quoted default" in result.stderr
    assert not result.stdout


def test_python_fuzzer_build_preserves_flags_and_packages_seed(tmp_path, monkeypatch):
    import importlib.util
    import zipfile
    from pathlib import Path

    monkeypatch.syspath_prepend(str(REPO_ROOT / ".clusterfuzzlite"))
    spec = importlib.util.spec_from_file_location(
        "fuzzer_build", REPO_ROOT / ".clusterfuzzlite/build.py"
    )
    assert spec and spec.loader
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    source, work, output = (tmp_path / name for name in ("source tree", "work", "output"))
    for path in (
        source / "qgroundcontrol/cmake",
        source / "qgroundcontrol/test/Fuzz",
        work,
        output,
    ):
        path.mkdir(parents=True)
    (source / "qgroundcontrol/cmake/CustomOptions.cmake").write_text(
        'set(QGC_MAVLINK_DIALECT "all")\nset(QGC_MAVLINK_VERSION "2.0")\n'
    )
    (source / "qgroundcontrol/test/Fuzz/MAVLinkParserFuzzer.dict").write_text('"heartbeat"\n')
    for key, value in {
        "SRC": str(source),
        "WORK": str(work),
        "OUT": str(output),
        "CXX": "clang++",
        "CXXFLAGS": '-O1 -DNAME="with spaces"',
        "LIB_FUZZING_ENGINE": '"/engine path/libFuzzer.a"',
    }.items():
        monkeypatch.setenv(key, value)
    calls = []

    def run(command, **kwargs):
        calls.append((command, kwargs))
        if command[0] == str(work / "mavlink_seed_generator"):
            Path(command[-1]).write_bytes(b"heartbeat packet")

    monkeypatch.setattr(module.subprocess, "run", run)
    module.build()
    assert "-DNAME=with spaces" in calls[1][0]
    assert "/engine path/libFuzzer.a" in calls[1][0]
    assert "/engine path/libFuzzer.a" not in calls[2][0]
    assert calls[0][1]["env"]["PYTHONPATH"] == str(source / "mavlink")
    with zipfile.ZipFile(output / "mavlink_parser_fuzzer_seed_corpus.zip") as archive:
        assert archive.namelist() == ["heartbeat"]
        assert archive.read("heartbeat") == b"heartbeat packet"
