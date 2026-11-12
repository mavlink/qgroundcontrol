"""Every job that reports a package's contents reports the package it then uploads."""

from __future__ import annotations

from .._workflows import step_groups

REPORT = "./.github/actions/artifact-report"
UPLOAD = "./.github/actions/attest-and-upload"


def test_reports_compare_the_package_the_job_uploads() -> None:
    reporting = []
    for label, job, steps in step_groups(actions=False):
        for report in (step for step in steps if step.get("uses") == REPORT):
            reporting.append(label)
            # artifact-report compares with master's artifact of the same name, so it must be
            # the name this job uploads the package under.
            (upload,) = (
                step
                for step in steps
                if step.get("uses") == UPLOAD
                and step["with"]["package-name"] == report["with"]["artifact-name"]
            )
            assert steps.index(report) < steps.index(upload), label
            assert report.get("if") == upload.get("if"), label
            assert report["with"]["package"].endswith("/" + upload["with"]["artifact-name"]), label
            # The baseline lookup lists other runs' artifacts.
            assert job["permissions"]["actions"] == "read", label
    assert reporting
