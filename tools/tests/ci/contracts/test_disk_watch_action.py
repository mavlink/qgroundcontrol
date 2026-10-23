"""disk-watch samples the workspace volume in the background and reports the job's peak at its end."""

from __future__ import annotations

import json
import os
import re
import shutil
import signal
import subprocess
import time
from pathlib import Path
from typing import TYPE_CHECKING

import pytest

from ..._helpers import REPO_ROOT, get_steps, load_action

if TYPE_CHECKING:
    from collections.abc import Iterator

ACTION = REPO_ROOT / ".github/actions/disk-watch"
MIB = 1024**2
GIB = 1024**3
PIB = 1024**5


def _node() -> str:
    node = shutil.which("node")
    if not node:
        pytest.skip("node is not installed")
    version = subprocess.run([node, "--version"], capture_output=True, text=True, check=True)
    major, minor = (int(part) for part in version.stdout.strip().lstrip("v").split(".")[:2])
    if (major, minor) < (18, 15):
        pytest.skip("fs.statfsSync needs Node 18.15 or later")
    return node


def _alive(pid: int) -> bool:
    try:
        os.kill(pid, 0)
    except ProcessLookupError:
        return False
    return True


def _wait_for(condition, timeout: float = 10.0) -> bool:
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if condition():
            return True
        time.sleep(0.05)
    return condition()


@pytest.fixture
def runner(tmp_path: Path) -> dict[str, str]:
    (tmp_path / "temp").mkdir()
    (tmp_path / "workspace").mkdir()
    (tmp_path / "state").touch()
    env = {
        key: value
        for key, value in os.environ.items()
        if not key.startswith(("GITHUB_", "STATE_", "INPUT_"))
    }
    return {
        **env,
        "RUNNER_TEMP": str(tmp_path / "temp"),
        "GITHUB_WORKSPACE": str(tmp_path / "workspace"),
        "GITHUB_STATE": str(tmp_path / "state"),
        "GITHUB_STEP_SUMMARY": str(tmp_path / "summary.md"),
        "INPUT_INTERVAL-SECONDS": "0.05",
    }


@pytest.fixture
def samplers() -> Iterator[list[int]]:
    """Sampler PIDs to stop if a test fails before its post step does."""
    pids: list[int] = []
    yield pids
    for pid in pids:
        if _alive(pid):
            os.kill(pid, signal.SIGTERM)


def _run(node: str, script: str, env: dict[str, str]) -> str:
    result = subprocess.run(
        [node, str(ACTION / script)],
        env=env,
        capture_output=True,
        text=True,
        timeout=30,
        check=True,
    )
    return result.stdout


def _saved_state(env: dict[str, str]) -> dict[str, str]:
    lines = Path(env["GITHUB_STATE"]).read_text(encoding="utf-8").splitlines()
    return dict(line.split("=", 1) for line in lines if "=" in line)


def test_post_reports_a_peak_reached_between_steps_and_stops_the_sampler(
    runner: dict[str, str], samplers: list[int]
) -> None:
    node = _node()
    started = _run(node, "main.mjs", runner)
    assert started.startswith(f"Watching disk use of {runner['GITHUB_WORKSPACE']} every 0.05s:")
    saved = _saved_state(runner)
    pid = int(saved["pid"])
    samplers.append(pid)
    state_path = saved["state"]
    assert _alive(pid)

    def samples() -> int:
        try:
            return json.loads(Path(state_path).read_text(encoding="utf-8"))["samples"]
        except (OSError, ValueError, KeyError):
            return 0

    # A later step fills the volume for a while, then cleans up before the job ends.
    first = samples()
    blob = os.path.join(runner["GITHUB_WORKSPACE"], "blob")
    with open(blob, "wb") as handle:
        handle.write(b"\0" * 64 * MIB)
        handle.flush()
        os.fsync(handle.fileno())
    assert _wait_for(lambda: samples() >= first + 3)
    os.remove(blob)

    reported = _run(node, "post.mjs", {**runner, "STATE_pid": str(pid), "STATE_state": state_path})
    assert _wait_for(lambda: not _alive(pid))

    state = json.loads(Path(state_path).read_text(encoding="utf-8"))
    assert state["peak"]["used"] - state["start"]["used"] >= 32 * MIB
    # The test machine's own volume may be nearly full, which adds a warning and a ⚠️.
    assert re.fullmatch(
        rf"Disk use of {re.escape(runner['GITHUB_WORKSPACE'])} \([\d.]+ GiB volume, \d+ samples\): "
        r"start [\d.]+ GiB, peak [\d.]+ GiB \(\d+%\) at \S+ with [\d.]+ GiB free, end [\d.]+ GiB\n"
        r"(?:::warning title=Disk nearly full::.+\n)?",
        reported,
    )
    # Laid out like every other job-summary section (common.markdown.summary_section).
    summary = Path(runner["GITHUB_STEP_SUMMARY"]).read_text(encoding="utf-8")
    assert re.fullmatch(
        r"### Disk\n\n\| Item \| Value \|\n\| --- \| --- \|\n"
        r"\| Volume \| [\d.]+ GiB \(.+\) \|\n\| Start \| [\d.]+ GiB used \|\n"
        r"\| Peak \| (?:⚠️ )?[\d.]+ GiB used \(\d+%\), [\d.]+ GiB free, at \S+ \|\n"
        r"\| End \| [\d.]+ GiB used \|\n\n",
        summary,
    )


def test_disk_watch_never_fails_the_job(runner: dict[str, str]) -> None:
    node = _node()
    missing = {**runner, "GITHUB_WORKSPACE": os.path.join(runner["RUNNER_TEMP"], "missing")}
    assert _run(node, "main.mjs", missing).startswith("::warning::Disk watch could not start:")
    # A post step without a started sampler reports nothing and still succeeds.
    assert _run(node, "post.mjs", runner) == "Disk watch recorded no samples\n"


def test_low_free_threshold_is_a_tenth_of_the_volume_between_5_and_10_gib() -> None:
    node = _node()
    # Hosted macOS reports its whole 320 GiB disk, where 25 GiB free is normal.
    totals = [20 * GIB, 50 * GIB, 80 * GIB, 130 * GIB, 320 * GIB]
    script = (
        f'import {{ lowFreeThreshold }} from "{(ACTION / "disk.mjs").as_uri()}";\n'
        f"console.log(JSON.stringify({totals}.map(lowFreeThreshold)));"
    )
    result = subprocess.run(
        [node, "--input-type=module", "-e", script],
        capture_output=True,
        text=True,
        timeout=30,
        check=True,
    )
    assert json.loads(result.stdout) == [5 * GIB, 5 * GIB, 8 * GIB, 10 * GIB, 10 * GIB]


@pytest.mark.parametrize(
    ("free", "warns"),
    [
        pytest.param(4 * GIB, True, id="below-5-GiB"),
        pytest.param(9 * GIB, True, id="below-the-10-GiB-cap"),
        # 10% of a large volume would warn with plenty left (hosted macOS reports 320 GiB).
        pytest.param(PIB // 20, False, id="large-volume-above-the-cap"),
        pytest.param(PIB // 2, False, id="half-free"),
    ],
)
def test_post_warns_when_the_least_free_space_falls_below_the_threshold(
    runner: dict[str, str], free: int, warns: bool
) -> None:
    node = _node()
    # A recorded peak on a 1 PiB volume, which the post step's own sample cannot exceed.
    peak = {"total": PIB, "used": PIB - free, "free": free, "at": "2026-10-23T12:00:00.000Z"}
    state_path = Path(runner["RUNNER_TEMP"], "disk-watch.json")
    workspace = runner["GITHUB_WORKSPACE"]
    state_path.write_text(
        json.dumps({"path": workspace, "start": peak, "peak": peak, "last": peak, "samples": 1}),
        encoding="utf-8",
    )
    reported = _run(node, "post.mjs", {**runner, "STATE_state": str(state_path)})

    warnings = [line for line in reported.splitlines() if line.startswith("::warning")]
    summary = Path(runner["GITHUB_STEP_SUMMARY"]).read_text(encoding="utf-8")
    (peak_row,) = (line for line in summary.splitlines() if line.startswith("| Peak |"))
    if warns:
        threshold = 10.0
        assert warnings == [
            f"::warning title=Disk nearly full::Only {free / GIB:.1f} GiB of 1048576.0 GiB was "
            f"free on {workspace} at {peak['at']} ({round(100 * (PIB - free) / PIB)}%25 used; "
            f"warning below {threshold:.1f} GiB: 10%25 of the volume, between 5 and 10 GiB)"
        ]
        assert peak_row.startswith("| Peak | ⚠️ ")
    else:
        assert warnings == []
        assert "⚠️" not in summary


def test_interval_input_reaches_the_sampler_under_the_runner_s_variable_name() -> None:
    # The runner passes a JavaScript action's inputs as INPUT_<NAME>, upper-cased, hyphens kept.
    inputs = load_action("disk-watch")["inputs"]
    assert set(inputs) == {"interval-seconds"}
    assert 'process.env["INPUT_INTERVAL-SECONDS"]' in (ACTION / "main.mjs").read_text()


def test_action_reports_from_its_post_step_with_node_builtins_only() -> None:
    runs = load_action("disk-watch")["runs"]
    assert runs == {
        "using": "node24",
        "main": "main.mjs",
        "post": "post.mjs",
        "post-if": "always()",
    }
    # No package.json or dist/: every import must be a Node built-in or a sibling module.
    for script in ACTION.glob("*.mjs"):
        for module in re.findall(r'^import .* from "([^"]+)";$', script.read_text(), re.MULTILINE):
            assert module.startswith(("node:", "./")), f"{script.name}: {module}"
            if module.startswith("./"):
                assert (ACTION / module).is_file(), f"{script.name}: {module}"


@pytest.mark.parametrize("action", ["build-setup", "docker"])
def test_build_jobs_watch_disk_use_from_their_first_step(action: str) -> None:
    # Started first, its post step runs last: after packaging and every cache save.
    first = get_steps(load_action(action))[0]
    assert first == {"name": "Watch disk use", "uses": "./.github/actions/disk-watch"}
