"""Exercise the runtime backend probe and its compiler-cache limit consumer."""

from __future__ import annotations

import json
import os
import subprocess
from pathlib import Path

import pytest
import yaml

REPO_ROOT = Path(__file__).resolve().parents[3]
GITHUB_ENDPOINT = "https://results-receiver.actions.githubusercontent.com/"
PROXY_ENDPOINT = "http://10.1.2.3:6871/"


def _steps(action: str) -> list[dict]:
    return yaml.safe_load((REPO_ROOT / f".github/actions/{action}/action.yml").read_text())["runs"][
        "steps"
    ]


@pytest.mark.parametrize(
    ("runner", "bucket", "endpoint", "original", "expected"),
    [
        ("", "", GITHUB_ENDPOINT, "", "github"),
        ("windows", "bucket", GITHUB_ENDPOINT, "", "github"),
        ("linux", "bucket", PROXY_ENDPOINT, GITHUB_ENDPOINT, "s3"),
        ("windows", "bucket", PROXY_ENDPOINT, GITHUB_ENDPOINT, "s3"),
        ("windows", "bucket", GITHUB_ENDPOINT, GITHUB_ENDPOINT, "github"),
        ("windows", "bucket", PROXY_ENDPOINT, "", "github"),
        ("linux", "bucket", "invalid", GITHUB_ENDPOINT, "github"),
        ("linux", "bucket", "file:///cache", GITHUB_ENDPOINT, "github"),
        ("linux", "", PROXY_ENDPOINT, GITHUB_ENDPOINT, "github"),
        ("", "bucket", PROXY_ENDPOINT, GITHUB_ENDPOINT, "github"),
    ],
)
def test_backend_uses_javascript_action_environment(runner, bucket, endpoint, original, expected):
    steps = _steps("setup-python")
    script = steps[0]["with"]["script"]
    env = {
        "RUNS_ON_RUNNER_NAME": runner,
        "RUNS_ON_S3_BUCKET_CACHE": bucket,
        "ACTIONS_RESULTS_URL": endpoint,
        "ZCTIONS_RESULTS_URL": original,
    }
    harness = """
        const [script, env] = JSON.parse(require('fs').readFileSync(0, 'utf8'));
        const outputs = {};
        const warnings = [];
        const core = {
            exportVariable: (key, value) => { outputs[key] = value; },
            warning: value => warnings.push(value),
            info: () => {},
        };
        new Function('core', 'process', script)(core, {env});
        console.log(JSON.stringify({outputs, warnings}));
    """
    result = subprocess.run(
        ["node", "-e", harness],
        input=json.dumps([script, env]),
        text=True,
        capture_output=True,
        check=True,
    )
    observed = json.loads(result.stdout)
    assert observed["outputs"] == {"QGC_ACTIONS_CACHE_BACKEND": expected}
    assert bool(observed["warnings"]) == bool(runner and bucket and expected == "github")
    assert steps[1]["uses"].startswith("astral-sh/setup-uv@")


@pytest.mark.parametrize(
    ("backend", "event", "host", "build_type", "variant", "expected"),
    [
        ("", "push", "linux", "Release", "", "1G"),
        ("github", "pull_request", "linux", "Debug", "coverage", "3G"),
        ("github", "pull_request", "linux", "Debug", "sanitizers", "3G"),
        ("github", "pull_request", "windows", "Release", "", "3G"),
        ("github", "push", "mac", "Release", "", "2G"),
        ("s3", "push", "linux", "Release", "", "5G"),
        ("s3", "pull_request", "windows", "Release", "", "5G"),
    ],
)
def test_compiler_limit_follows_resolved_backend(
    tmp_path, backend, event, host, build_type, variant, expected
):
    script = next(
        step["run"]
        for step in _steps("cache")
        if step.get("name") == "Configure ccache environment"
    )
    output = tmp_path / "github-env"
    subprocess.run(
        ["bash", "-e", "-c", script],
        env=dict(
            os.environ,
            QGC_ACTIONS_CACHE_BACKEND=backend,
            EVENT_NAME=event,
            RUNS_ON_S3_BUCKET_CACHE="bucket",
            CONFIGURED_MAX_SIZE="5G",
            GITHUB_WORKSPACE=str(REPO_ROOT),
            GITHUB_ENV=str(output),
            HOST=host,
            BUILD_TYPE=build_type,
            VARIANT=variant,
        ),
        text=True,
        capture_output=True,
        check=True,
    )
    assert f"CCACHE_MAXSIZE={expected}\n" in output.read_text()
