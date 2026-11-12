"""CI Scripts workflow contracts."""

from __future__ import annotations

from typing import TYPE_CHECKING

from ..._helpers import load_workflow
from .test_android_serial_jni_contract import (
    CPP_HEADER,
    CPP_SOURCE,
    JAVA_SOURCE,
    PORT_INFO_SOURCE,
    READER_SOURCE,
    RESOURCE_ERROR_SOURCE,
    RESULT_SOURCE,
)

if TYPE_CHECKING:
    import pytest

VEHICLE_CONFIG_PAGES = "src/**/*.VehicleConfig.json"


def test_python_test_checkouts_include_the_files_tests_enumerate(
    pytestconfig: pytest.Config,
) -> None:
    # test_config_qml_generator.py parametrizes over these pages; without them collection fails
    # instead of silently skipping the audit.
    assert pytestconfig.getini("empty_parameter_set_mark") == "fail_at_collect"
    workflow = load_workflow("ci-scripts.yml")
    paths = workflow["on"]["pull_request"]["paths"]
    assert workflow["on"]["push"]["paths"] == paths
    assert {VEHICLE_CONFIG_PAGES, "docs/.vitepress/config.mjs"} <= set(paths)
    for job in ("test-ci-scripts", "test-tools-windows"):
        steps = workflow["jobs"][job]["steps"]
        checkout = next(s for s in steps if s.get("uses", "").startswith("actions/checkout@"))
        assert checkout["with"]["sparse-checkout-cone-mode"] is False
        assert VEHICLE_CONFIG_PAGES in checkout["with"]["sparse-checkout"].splitlines()


def test_linux_tests_check_out_and_follow_the_android_serial_jni_sources() -> None:
    # test_android_serial_jni_contract.py reads these files when it runs, which only the Linux job
    # does: the Windows job collects the module and runs none of its tests.
    workflow = load_workflow("ci-scripts.yml")
    sources = {
        CPP_HEADER,
        CPP_SOURCE,
        JAVA_SOURCE,
        PORT_INFO_SOURCE,
        READER_SOURCE,
        RESOURCE_ERROR_SOURCE,
        RESULT_SOURCE,
    }
    assert sources <= set(workflow["on"]["pull_request"]["paths"])
    steps = workflow["jobs"]["test-ci-scripts"]["steps"]
    checkout = next(s for s in steps if s.get("uses", "").startswith("actions/checkout@"))
    assert sources <= set(checkout["with"]["sparse-checkout"].splitlines())
