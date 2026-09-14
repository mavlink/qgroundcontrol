"""Keep custom-plugin PR builds lean while retaining production package validation."""

import yaml
from _helpers import REPO_ROOT


def _workflow():
    return yaml.safe_load((REPO_ROOT / ".github/workflows/custom-build.yml").read_text())


def test_custom_build_matrix_runs_debug_only_on_pull_requests():
    workflow = _workflow()
    assert set(workflow[True]) == {"push", "pull_request", "merge_group", "workflow_dispatch"}
    assert workflow[True]["push"]["branches"] == ["master"]
    assert set(workflow["jobs"]) == {"changes", "build"}
    job = workflow["jobs"]["build"]
    assert job["strategy"]["fail-fast"] is False
    assert job["strategy"]["matrix"] == {
        "build-type": "${{ fromJSON(github.event_name == 'pull_request' && "
        '\'["Debug"]\' || \'["Debug", "Release"]\') }}'
    }
    assert job["if"] == "needs.changes.outputs.should_build == 'true'"
    assert job["name"] == (
        "${{ matrix.build-type == 'Debug' && 'Custom Plugin Test Debug' || 'Custom Plugin Build' }}"
    )


def test_custom_matrix_keeps_tests_and_production_packaging_separate():
    steps = _workflow()["jobs"]["build"]["steps"]
    actions = {step.get("uses"): step for step in steps}
    configure = actions["./.github/actions/cmake-configure"]["with"]
    assert configure["extra-args"] == "-DQGC_CUSTOM_DIR=custom-example"
    assert configure["testing"] == "${{ matrix.build-type == 'Debug' }}"
    assert configure["preset"] == "${{ matrix.build-type == 'Debug' && 'Linux-debug' || 'Linux' }}"
    for action in ("build-setup", "cmake-configure", "cmake-build", "cmake-install"):
        assert actions[f"./.github/actions/{action}"]["with"]["build-type"] == (
            "${{ matrix.build-type }}"
        )
    setup = actions["./.github/actions/build-setup"]["with"]
    assert setup["cache-variant"] == "custom"
    assert setup["cache-key-suffix"] == (
        "${{ matrix.build-type == 'Debug' && 'custom-debug' || 'custom-release' }}"
    )
    tests = actions["./.github/actions/test-phase"]
    assert tests["if"] == (
        "${{ !cancelled() && matrix.build-type == 'Debug' && steps.build.outcome == 'success' }}"
    )
    assert tests["with"]["integration-exclude-labels"] == "Flaky|Network|StockUI"
    for action in ("cmake-install", "verify-executable"):
        assert actions[f"./.github/actions/{action}"]["if"] == "matrix.build-type == 'Release'"
    assert sum(step.get("uses") == "./.github/actions/cmake-build" for step in steps) == 1
