"""Keep custom-plugin PR builds lean while retaining production package validation."""

from ci.planning.detect_changes import build_patterns

from ..._helpers import load_workflow


def _job():
    return load_workflow("linux.yml")["jobs"]["custom-plugin"]


def test_custom_plugin_runs_debug_only_on_pull_requests():
    job = _job()
    assert job["strategy"]["fail-fast"] is False
    assert job["strategy"]["matrix"] == {
        "build-type": "${{ fromJSON(github.event_name == 'pull_request' && "
        '\'["Debug"]\' || \'["Debug", "Release"]\') }}'
    }
    assert job["needs"] == "changes"
    assert job["if"] == "needs.changes.outputs.should_build == 'true'"
    assert job["name"] == (
        "${{ matrix.build-type == 'Debug' && 'Custom Plugin Test Debug' || 'Custom Plugin Build' }}"
    )


def test_custom_plugin_unit_tests_and_boots_debug_and_packages_release():
    steps = _job()["steps"]
    actions = {step.get("uses"): step for step in steps}
    configure = actions["./.github/actions/cmake-configure"]["with"]
    assert configure["extra-args"] == "-DQGC_CUSTOM_DIR=custom-example"
    # The presets set the build type and QGC_BUILD_TESTING (Linux-debug ON, Linux OFF).
    assert configure["preset"] == "${{ matrix.build-type == 'Debug' && 'Linux-debug' || 'Linux' }}"
    assert "with" not in actions["./.github/actions/cmake-build"]
    setup = actions["./.github/actions/build-setup"]["with"]
    assert setup["build-type"] == "${{ matrix.build-type }}"
    assert setup["cache-variant"] == "custom"
    # The variant and build type separate the compiler caches; the CPM sources are Linux's own.
    assert "cache-key-suffix" not in setup

    debug_only = (
        "${{ !cancelled() && matrix.build-type == 'Debug' && steps.build.outcome == 'success' }}"
    )
    by_name = {step.get("name"): step for step in steps}
    # The stock Linux legs run the integration suites; the custom build runs the unit suite and boots.
    assert "./.github/actions/test-phase" not in actions
    unit = by_name["Run unit tests"]
    assert unit["if"] == debug_only
    assert unit["with"]["include-labels"] == "Unit"
    assert unit["with"]["exclude-labels"] == "Flaky|Network|StockUI"
    boot = by_name["Verify custom executable"]
    assert boot["if"] == debug_only
    assert boot["with"]["binary-path"] == "${{ runner.temp }}/build/Debug/Custom-QGroundControl"

    appimage = by_name["Verify Custom AppImage"]
    assert appimage["if"] == "matrix.build-type == 'Release'"
    (install,) = (step for step in steps if step.get("name") == "Create AppImage")
    assert install["if"] == "matrix.build-type == 'Release'"
    assert install["working-directory"] == "${{ runner.temp }}/build"
    assert install["run"] == "cmake --install . --config Release"
    assert sum(step.get("uses") == "./.github/actions/cmake-build" for step in steps) == 1


def test_custom_example_changes_build_linux():
    assert any(
        pattern.search("custom-example/src/CustomPlugin.cc") for pattern in build_patterns("linux")
    )
