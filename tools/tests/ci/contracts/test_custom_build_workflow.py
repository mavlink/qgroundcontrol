"""Changes to the custom-plugin overlay build Linux, whose custom-plugin job is its only build."""

from ci.planning.detect_changes import build_patterns

from ..._helpers import load_workflow


def test_custom_example_changes_build_linux():
    job = load_workflow("linux.yml")["jobs"]["custom-plugin"]
    assert "-DQGC_CUSTOM_DIR=custom-example" in str(job["steps"])
    assert any(
        pattern.search("custom-example/src/CustomPlugin.cc") for pattern in build_patterns("linux")
    )
