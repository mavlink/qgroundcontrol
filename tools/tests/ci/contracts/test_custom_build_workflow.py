"""Build the custom-plugin overlay once: unit tests and a boot everywhere, its AppImage off pull requests."""

from ci.planning.detect_changes import build_patterns

from ..._helpers import load_workflow


def _job():
    return load_workflow("linux.yml")["jobs"]["custom-plugin"]


def test_custom_plugin_is_one_debug_build():
    job = _job()
    assert job["name"] == "Custom Plugin"
    assert "strategy" not in job
    assert job["needs"] == "changes"
    assert job["if"] == "needs.changes.outputs.should_build == 'true'"
    assert "matrix." not in str(job)

    steps = job["steps"]
    actions = {step.get("uses"): step for step in steps}
    setup = actions["./.github/actions/build-setup"]["with"]
    assert setup["build-type"] == "Debug"
    assert setup["cache-variant"] == "custom"
    # The variant separates the compiler caches; the CPM sources are Linux's own.
    assert "cache-key-suffix" not in setup
    configure = actions["./.github/actions/cmake-configure"]["with"]
    # Linux-debug sets QGC_BUILD_TESTING=ON.
    assert configure == {"preset": "Linux-debug", "extra-args": "-DQGC_CUSTOM_DIR=custom-example"}
    assert sum(step.get("uses") == "./.github/actions/cmake-build" for step in steps) == 1


def test_custom_plugin_tests_and_boots_everywhere_and_packages_off_pull_requests():
    steps = _job()["steps"]
    by_name = {step.get("name"): step for step in steps}
    names = list(by_name)
    built = "${{ !cancelled() && steps.build.outcome == 'success' }}"
    packaged = (
        "${{ !cancelled() && github.event_name != 'pull_request' && "
        "steps.build.outcome == 'success' }}"
    )

    # The stock Linux legs run the integration suites; the custom build runs the unit suite.
    assert "./.github/actions/test-phase" not in {step.get("uses") for step in steps}
    unit = by_name["Run unit tests"]
    assert unit["if"] == built
    assert unit["with"]["include-labels"] == "Unit"
    assert unit["with"]["exclude-labels"] == "Flaky|Network|StockUI"
    boot = by_name["Verify custom executable"]
    assert boot["if"] == built
    assert boot["with"]["binary-path"] == "${{ runner.temp }}/build/Debug/Custom-QGroundControl"

    install = by_name["Create AppImage"]
    assert install["if"] == packaged
    assert install["working-directory"] == "${{ runner.temp }}/build"
    # Unstripped, the Debug binary alone is ~800 MB.
    assert install["run"] == "cmake --install . --config Debug --strip"
    appimage = by_name["Verify Custom AppImage"]
    assert appimage["if"] == packaged
    assert appimage["with"] == {
        "binary-path": "${{ runner.temp }}/build/Custom-QGroundControl-x86_64.AppImage",
        "type": "appimage",
    }
    # Packaging follows the tests and the build-tree boot, which pull requests rely on alone.
    assert names.index("Verify custom executable") < names.index("Create AppImage")


def test_custom_example_changes_build_linux():
    assert any(
        pattern.search("custom-example/src/CustomPlugin.cc") for pattern in build_patterns("linux")
    )
