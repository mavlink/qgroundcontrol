"""android.yml keeps the reports android_lint.py writes after a failed lint."""

from __future__ import annotations

from ci.android.android_lint import DEFAULT_REPORT, LOG_NAME

from ..._helpers import get_step, get_steps, load_workflow


def test_failed_lint_uploads_the_report_and_log_the_script_writes() -> None:
    upload = get_step(get_steps(load_workflow("android.yml"), "build"), "upload-lint")
    paths = upload["with"]["path"].split()
    for written in (DEFAULT_REPORT.as_posix(), LOG_NAME):
        assert any(path.endswith(f"/{written}") for path in paths), written
