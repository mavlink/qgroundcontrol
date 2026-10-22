"""Exercise the runtime Actions cache backend probe."""

from __future__ import annotations

import json
import subprocess

import pytest

from ..._helpers import get_steps, load_action

GITHUB_ENDPOINT = "https://results-receiver.actions.githubusercontent.com/"
PROXY_ENDPOINT = "http://10.1.2.3:6871/"


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
    steps = get_steps(load_action("setup-python"))
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
    expected_outputs = {"QGC_ACTIONS_CACHE_BACKEND": expected}
    if expected == "s3":
        expected_outputs["UV_PYTHON_PREFERENCE"] = "only-managed"
    assert observed["outputs"] == expected_outputs
    assert bool(observed["warnings"]) == bool(runner and bucket and expected == "github")
    assert steps[1]["uses"].startswith("astral-sh/setup-uv@")
