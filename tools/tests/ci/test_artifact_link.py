"""Tests for artifact_link.py."""

from __future__ import annotations

import pytest
from ci.packaging import artifact_link

pytestmark = pytest.mark.windows

URL = "https://github.com/mavlink/qgroundcontrol/actions/runs/1/artifacts/2"


def test_links_the_artifact_after_the_previous_section(gh_summary, capsys):
    gh_summary.write_text("### Boot test\n\n| Item | Value |\n\n")

    assert (
        artifact_link.main(
            ["--label", "Boot test diagnostics", "--name", "boot-test-1", "--url", URL]
        )
        == 0
    )

    assert gh_summary.read_text() == (
        f"### Boot test\n\n| Item | Value |\n\nBoot test diagnostics: [`boot-test-1`]({URL})\n\n"
    )
    assert f"Boot test diagnostics: {URL}" in capsys.readouterr().out


def test_reads_the_name_and_url_from_the_step_environment(gh_summary, monkeypatch):
    monkeypatch.setenv("ARTIFACT_NAME", "ios-boot-test")
    monkeypatch.setenv("ARTIFACT_URL", URL)

    assert artifact_link.main(["--label", "Boot test log"]) == 0

    assert gh_summary.read_text() == f"Boot test log: [`ios-boot-test`]({URL})\n\n"


def test_empty_url_writes_nothing(gh_summary):
    """upload-artifact leaves artifact-url empty when it found nothing to upload."""
    assert artifact_link.main(["--label", "Boot test log", "--name", "ios-boot-test"]) == 0
    assert gh_summary.read_text() == ""


@pytest.mark.parametrize(
    ("args", "message"),
    [
        (["--url", "http://example.com/a"], "must be an https URL"),
        (["--url", f"{URL}) [x](https://evil"], "must be an https URL"),
        (["--url", URL], "--name (or ARTIFACT_NAME) is required"),
    ],
)
def test_rejects_unsafe_urls_and_missing_names(gh_summary, capsys, args, message):
    with pytest.raises(SystemExit) as raised:
        artifact_link.main(["--label", "Diagnostics", *args])
    assert raised.value.code == 2
    assert message in capsys.readouterr().err
    assert gh_summary.read_text() == ""


def test_label_and_name_cannot_inject_markup():
    link = artifact_link.artifact_link("Diag [x](y)", "name`with`ticks", URL)
    assert link == f"Diag \\[x\\]\\(y\\): [``name`with`ticks``]({URL})\n\n"
