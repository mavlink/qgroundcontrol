"""Keep custom-plugin PR builds lean while retaining production package validation."""

from ..._helpers import load_workflow


def _workflow():
    return load_workflow("custom-build.yml")


def test_custom_build_matrix_runs_debug_only_on_pull_requests():
    workflow = _workflow()
    assert set(workflow["on"]) == {"push", "pull_request", "merge_group", "workflow_dispatch"}
    assert workflow["on"]["push"]["branches"] == ["master"]
    assert set(workflow["jobs"]) == {"changes", "build", "custom-build-ok"}
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
    configure = actions["$/.github/actions/cmake-configure"]["with"]
    assert configure["extra-args"] == "-DQGC_CUSTOM_DIR=custom-example"
    assert configure["testing"] == "${{ matrix.build-type == 'Debug' }}"
    assert configure["preset"] == "${{ matrix.build-type == 'Debug' && 'Linux-debug' || 'Linux' }}"
    for action in ("build-setup", "cmake-configure", "cmake-build"):
        assert actions[f"$/.github/actions/{action}"]["with"]["build-type"] == (
            "${{ matrix.build-type }}"
        )
    setup = actions["$/.github/actions/build-setup"]["with"]
    assert setup["cache-variant"] == "custom"
    assert setup["cache-key-suffix"] == (
        "${{ matrix.build-type == 'Debug' && 'custom-debug' || 'custom-release' }}"
    )
    tests = actions["$/.github/actions/test-phase"]
    assert tests["if"] == (
        "${{ !cancelled() && matrix.build-type == 'Debug' && steps.build.outcome == 'success' }}"
    )
    assert tests["with"]["integration-exclude-labels"] == "Flaky|Network|StockUI"
    assert actions["$/.github/actions/verify-executable"]["if"] == "matrix.build-type == 'Release'"
    (install,) = (step for step in steps if step.get("name") == "Create AppImage")
    assert install["if"] == "matrix.build-type == 'Release'"
    assert install["working-directory"] == "${{ runner.temp }}/build"
    assert install["run"] == "cmake --install . --config Release"
    assert sum(step.get("uses") == "$/.github/actions/cmake-build" for step in steps) == 1
