"""Tests for gh_pr_size_label.py."""

from __future__ import annotations

import json
from typing import TYPE_CHECKING, Any

import pytest
from ci.reporting import gh_pr_size_label as mod

from .._helpers import completed, load_workflow

if TYPE_CHECKING:
    import subprocess


class FakeGitHub:
    """Serve the gh api calls made by `apply` from in-memory PR state."""

    def __init__(
        self,
        files: list[tuple[str, int, int]],
        labels: list[str],
        comments: list[dict[str, Any]] | None = None,
        fail_writes: bool = False,
    ) -> None:
        self.files = files
        self.labels = labels
        self.comments = comments or []
        self.fail_writes = fail_writes
        self.writes: list[tuple[str, ...]] = []

    def __call__(self, *args: str, **_kwargs: Any) -> subprocess.CompletedProcess:
        path = next(arg for arg in args if arg.startswith("repos/"))
        if "GET" in args:
            if path.endswith("/files"):
                assert "--paginate" in args
                assert "per_page=100" in args
                rows = [{"filename": f, "additions": a, "deletions": d} for f, a, d in self.files]
                return completed("".join(json.dumps(row) + "\n" for row in rows))
            if path.endswith("/labels"):
                return completed("".join(f"{label}\n" for label in self.labels))
            if path.endswith("/comments"):
                assert "--paginate" in args
                ids = [
                    str(comment["id"])
                    for comment in self.comments
                    if comment["user"] == mod.NOTICE_AUTHOR
                    and comment["body"].startswith(mod.XL_NOTICE_HEADING)
                ]
                return completed("".join(f"{i}\n" for i in ids))
            raise AssertionError(f"unexpected read {args}")
        self.writes.append(args)
        if self.fail_writes:
            return completed(returncode=1, stderr="HTTP 500: internal")
        return completed()


@pytest.fixture
def pr_env(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setenv("GH_REPO", "owner/repo")
    monkeypatch.setenv("PR_NUMBER", "42")


@pytest.mark.parametrize(
    ("lines", "label"),
    [
        (0, "size/XS"),
        (149, "size/XS"),
        (150, "size/S"),
        (499, "size/S"),
        (500, "size/M"),
        (999, "size/M"),
        (1000, "size/L"),
        (1499, "size/L"),
        (1500, "size/XL"),
    ],
)
def test_size_label_thresholds_are_exclusive_upper_bounds(lines: int, label: str) -> None:
    assert mod.size_label(lines) == label


def test_count_changed_lines_skips_lock_files_and_translations(
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    fake = FakeGitHub(
        files=[
            ("src/Vehicle/Vehicle.cc", 10, 5),
            ("docs/package-lock.json", 1000, 1000),
            ("tools/uv.lock", 1000, 1000),
            ("translations/qgc_de_DE.ts", 1000, 1000),
            ("translations/nested/extra.ts", 1000, 1000),
            ("tools/vendor/package-lock.json", 3, 4),
        ],
        labels=[],
    )
    monkeypatch.setattr(mod, "gh", fake)
    assert mod.count_changed_lines("owner/repo", "42") == 22


def test_list_size_labels_filters_to_size_prefix(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr(mod, "gh", lambda *a, **kw: completed("size/M\nsize/L\n"))
    assert mod.list_size_labels("owner/repo", "42") == ["size/L", "size/M"]


def test_list_size_labels_exits_on_gh_failure(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr(mod, "gh", lambda *a, **kw: completed("", returncode=1, stderr="boom"))
    with pytest.raises(SystemExit):
        mod.list_size_labels("owner/repo", "42")


def test_remove_label_url_encodes_slash(monkeypatch: pytest.MonkeyPatch) -> None:
    calls: list[tuple[Any, ...]] = []

    def fake_gh(*args: str, **kwargs: Any) -> subprocess.CompletedProcess:
        calls.append(args)
        return completed()

    monkeypatch.setattr(mod, "gh", fake_gh)
    assert mod.remove_label("owner/repo", "42", "size/XL") is True
    assert "repos/owner/repo/issues/42/labels/size%2FXL" in calls[0]


def test_remove_label_treats_404_as_success(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr(
        mod,
        "gh",
        lambda *a, **kw: completed(returncode=1, stderr="HTTP 404: Not Found"),
    )
    assert mod.remove_label("owner/repo", "42", "size/M") is True


@pytest.mark.usefixtures("pr_env")
def test_apply_swaps_stale_size_labels(monkeypatch: pytest.MonkeyPatch) -> None:
    fake = FakeGitHub(files=[("a.cc", 300, 100)], labels=["size/L", "size/XS"])
    monkeypatch.setattr(mod, "gh", fake)

    assert mod.main(["apply"]) == 0
    assert fake.writes == [
        ("api", "--method", "POST", "repos/owner/repo/issues/42/labels", "-f", "labels[]=size/S"),
        ("api", "repos/owner/repo/issues/42/labels/size%2FL", "-X", "DELETE"),
        ("api", "repos/owner/repo/issues/42/labels/size%2FXS", "-X", "DELETE"),
    ]


@pytest.mark.usefixtures("pr_env")
def test_apply_keeps_current_label_without_writes(monkeypatch: pytest.MonkeyPatch) -> None:
    fake = FakeGitHub(files=[("a.cc", 1, 1)], labels=["size/XS"])
    monkeypatch.setattr(mod, "gh", fake)

    assert mod.main(["apply"]) == 0
    assert fake.writes == []


@pytest.mark.usefixtures("pr_env")
def test_apply_posts_xl_notice_once(monkeypatch: pytest.MonkeyPatch) -> None:
    fake = FakeGitHub(files=[("a.cc", 1500, 0)], labels=["size/L"])
    monkeypatch.setattr(mod, "gh", fake)

    assert mod.main(["apply"]) == 0
    notice = fake.writes[-1]
    assert notice[:4] == ("api", "--method", "POST", "repos/owner/repo/issues/42/comments")
    assert notice[-1] == f"body={mod.XL_NOTICE}"
    assert "\n\nConsider:\n- Splitting" in mod.XL_NOTICE


@pytest.mark.parametrize(
    "existing_body",
    [
        mod.XL_NOTICE,
        # Notices from codelytv/pr-size-labeler were flattened onto one line.
        "## Large PR Warning  This PR has **more than 1500 lines** changed, ...",
    ],
)
@pytest.mark.usefixtures("pr_env")
def test_apply_does_not_repeat_xl_notice(
    monkeypatch: pytest.MonkeyPatch, existing_body: str
) -> None:
    fake = FakeGitHub(
        files=[("a.cc", 2000, 0)],
        labels=[],
        comments=[{"id": 7, "user": mod.NOTICE_AUTHOR, "body": existing_body}],
    )
    monkeypatch.setattr(mod, "gh", fake)

    assert mod.main(["apply"]) == 0
    assert fake.writes == [
        ("api", "--method", "POST", "repos/owner/repo/issues/42/labels", "-f", "labels[]=size/XL")
    ]


@pytest.mark.usefixtures("pr_env")
def test_apply_ignores_xl_notice_quoted_by_people(monkeypatch: pytest.MonkeyPatch) -> None:
    fake = FakeGitHub(
        files=[("a.cc", 2000, 0)],
        labels=["size/XL"],
        comments=[{"id": 7, "user": "octocat", "body": mod.XL_NOTICE}],
    )
    monkeypatch.setattr(mod, "gh", fake)

    assert mod.main(["apply"]) == 0
    assert [write[3] for write in fake.writes] == ["repos/owner/repo/issues/42/comments"]


@pytest.mark.usefixtures("pr_env")
def test_apply_reports_write_failures(
    monkeypatch: pytest.MonkeyPatch, capsys: pytest.CaptureFixture[str]
) -> None:
    fake = FakeGitHub(files=[("a.cc", 1, 0)], labels=["size/M"], fail_writes=True)
    monkeypatch.setattr(mod, "gh", fake)

    assert mod.main(["apply"]) == 1
    out = capsys.readouterr().out
    assert "failed to add label 'size/XS'" in out
    assert "failed to remove label 'size/M'" in out


def test_main_exits_when_repo_missing(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.delenv("GH_REPO", raising=False)
    with pytest.raises(SystemExit):
        mod.main(["apply", "--pr-number", "42"])


def test_main_exits_when_pr_number_missing(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setenv("GH_REPO", "owner/repo")
    monkeypatch.delenv("PR_NUMBER", raising=False)
    with pytest.raises(SystemExit):
        mod.main(["apply"])


def test_label_job_runs_trusted_script_without_third_party_labeler() -> None:
    workflow = load_workflow("pr-checks.yml")
    assert set(workflow["on"]) == {"pull_request_target"}
    assert set(workflow["jobs"]) == {"title", "label"}
    job = workflow["jobs"]["label"]
    assert job["permissions"] == {"contents": "read", "pull-requests": "write"}
    checkout = next(step for step in job["steps"] if step.get("uses", "").startswith("actions/"))
    assert checkout["uses"].startswith("actions/checkout@")
    # pull_request_target must never check out the PR head.
    assert "ref" not in checkout["with"]
    assert checkout["with"]["persist-credentials"] is False
    runs = [step["run"] for step in job["steps"] if "run" in step]
    assert runs == ["python3 tools/ci/reporting/gh_pr_size_label.py apply"]
    assert not any("pr-size-labeler" in step.get("uses", "") for step in job["steps"])
    # Path labels follow the advisory size step, so a size-label failure cannot skip them.
    labeler = job["steps"][-1]
    assert labeler["uses"] == "actions/labeler@v7"
    assert labeler["with"] == {"repo-token": "${{ github.token }}", "sync-labels": True}
