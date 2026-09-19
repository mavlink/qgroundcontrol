"""The writable poster treats fork artifacts as data, not authority."""

import copy
import io
import json
import zipfile
from pathlib import Path
from unittest.mock import Mock

import analysis_review_poster as poster
import pytest
import report_context
import yaml

REPO = "mavlink/qgroundcontrol"
SHA = "a" * 40


@pytest.fixture
def run():
    return {
        "id": 100,
        "run_attempt": 2,
        "head_sha": SHA,
        "workflow_id": 50,
        "repository": {"full_name": REPO},
        "head_repository": {"full_name": "contributor/qgroundcontrol"},
        "path": ".github/workflows/analysis.yml",
        "name": "Code Analysis",
        "event": "pull_request",
        "status": "completed",
        "conclusion": "success",
        "head_branch": "feature",
        "pull_requests": [],
    }


@pytest.fixture
def live(monkeypatch, run):
    pr = {
        "number": 42,
        "state": "open",
        "head": {"sha": SHA, "ref": "feature", "repo": {"full_name": "contributor/qgroundcontrol"}},
        "base": {"repo": {"full_name": REPO}},
    }
    responses = {
        f"repos/{REPO}/actions/runs/100": run,
        f"repos/{REPO}/actions/workflows/analysis.yml": {"id": 50},
        f"repos/{REPO}/commits/{SHA}/pulls": [{"number": 42}],
        f"repos/{REPO}/pulls/42": pr,
        f"repos/{REPO}/pulls?state=open&head=contributor%3Afeature&per_page=100&page=1": [],
    }
    api = Mock(side_effect=lambda path: copy.deepcopy(responses[path]))
    monkeypatch.setattr(poster, "api", api)
    monkeypatch.setattr(report_context, "api", api)
    monkeypatch.setattr(poster, "list_workflow_runs_for_sha", lambda *args: [run])
    return responses, api


@pytest.fixture
def finding():
    return {
        "path": "src/new é.h",
        "line": 5,
        "check": "clazy-range-loop",
        "level": "warning",
        "message": "Avoid copying",
        "context": "5 | code();\n  | ^",
    }


@pytest.fixture
def report(run, finding):
    return {
        "manifest": {
            "schema": 1,
            "repository": REPO,
            "run_id": run["id"],
            "run_attempt": 2,
            "pr_number": 42,
            "head_sha": SHA,
            "tool": "clazy",
            "truncated": False,
            "incomplete": False,
        },
        "findings": [finding],
    }


def test_fork_empty_pr_array_uses_live_commit_association(run, live):
    assert poster.current_run(REPO, run) == (run, 42)
    assert f"repos/{REPO}/commits/{SHA}/pulls" in [call.args[0] for call in live[1].call_args_list]


def test_fork_without_commit_association_uses_encoded_head(run, live):
    run["head_branch"] = "feat/gps-corrections"
    pr = live[0][f"repos/{REPO}/pulls/42"]
    pr["head"]["ref"] = run["head_branch"]
    live[0][f"repos/{REPO}/commits/{SHA}/pulls"] = []
    query = (
        f"repos/{REPO}/pulls?state=open&head=contributor%3Afeat%2Fgps-corrections"
        "&per_page=100&page=1"
    )
    live[0][query] = [pr]
    assert poster.current_run(REPO, run) == (run, 42)
    assert query in [call.args[0] for call in live[1].call_args_list]


@pytest.mark.parametrize(
    "section,field,value",
    [
        ("head", "sha", "b" * 40),
        ("head", "ref", "other"),
        ("head", "repo", {"full_name": "other/qgroundcontrol"}),
        ("base", "repo", {"full_name": "other/repo"}),
    ],
)
def test_head_lookup_rejects_mismatched_pr_identity(run, live, section, field, value):
    pr = live[0][f"repos/{REPO}/pulls/42"]
    pr[section][field] = value
    live[0][f"repos/{REPO}/commits/{SHA}/pulls"] = []
    live[0][f"repos/{REPO}/pulls?state=open&head=contributor%3Afeature&per_page=100&page=1"] = [pr]
    assert poster.current_run(REPO, run) is None


@pytest.mark.parametrize("state,count", [("open", 0), ("closed", 1), ("open", 2)])
def test_head_lookup_requires_one_open_match(run, live, state, count):
    pr = live[0][f"repos/{REPO}/pulls/42"]
    pr["state"] = state
    live[0][f"repos/{REPO}/commits/{SHA}/pulls"] = []
    live[0][f"repos/{REPO}/pulls?state=open&head=contributor%3Afeature&per_page=100&page=1"] = [
        {**pr, "number": 42 + index} for index in range(count)
    ]
    assert poster.current_run(REPO, run) is None


@pytest.mark.parametrize(
    "field,value",
    [
        ("id", 99),
        ("run_attempt", 1),
        ("head_sha", "b" * 40),
        ("workflow_id", 99),
        ("event", "push"),
        ("status", "in_progress"),
        ("conclusion", "cancelled"),
        ("path", ".github/workflows/other.yml"),
        ("name", "Other"),
        ("repository", {"full_name": "other/repo"}),
        ("head_repository", {"full_name": REPO}),
        ("head_branch", "other"),
    ],
)
def test_rejects_mismatched_or_ineligible_run(run, live, field, value):
    source = copy.deepcopy(run)
    run[field] = value
    assert poster.current_run(REPO, source) is None


@pytest.mark.parametrize("field,value", [("state", "closed"), ("head", {"sha": "b" * 40})])
def test_stale_or_closed_pr(run, live, field, value):
    live[0][f"repos/{REPO}/pulls/42"][field] = value
    assert poster.current_run(REPO, run) is None


def test_rejects_older_run_for_same_head(run, live, monkeypatch):
    monkeypatch.setattr(
        poster, "list_workflow_runs_for_sha", lambda *args: [run, {**run, "id": 101}]
    )
    assert poster.current_run(REPO, run) is None


@pytest.mark.parametrize(
    "field,value",
    [
        ("schema", True),
        ("repository", "fork/repo"),
        ("run_id", 99),
        ("run_attempt", 1),
        ("pr_number", 43),
        ("head_sha", "b" * 40),
        ("tool", "clang-tidy"),
        ("incomplete", "false"),
        ("event", "APPROVE"),
    ],
)
def test_manifest_tampering_is_rejected(run, report, field, value):
    report["manifest"][field] = value
    with pytest.raises(ValueError, match="manifest"):
        poster.validate_report(report, REPO, run, 42, "clazy")


@pytest.mark.parametrize(
    "field,value",
    [
        ("path", "../src/a.h"),
        ("path", "/src/a.h"),
        ("path", "src\\a.h"),
        ("line", True),
        ("line", 0),
        ("side", "LEFT"),
        ("message", "x" * 2001),
        ("message", "\x1b[31m"),
        ("context", "x" * 1001),
        ("check", "<!-- marker -->"),
        ("level", "fatal"),
        ("message", None),
    ],
)
def test_finding_schema_limits(run, report, field, value):
    report["findings"][0][field] = value
    with pytest.raises(ValueError):
        poster.validate_report(report, REPO, run, 42, "clazy")


def archive_data(name="report.json", content=b"{}", mode=0o100644):
    data = io.BytesIO()
    with zipfile.ZipFile(data, "w", compression=zipfile.ZIP_DEFLATED) as archive:
        info = zipfile.ZipInfo(name)
        info.external_attr = mode << 16
        archive.writestr(info, content)
    return data.getvalue()


@pytest.mark.parametrize("name", ["../report.json", "/report.json", "poster.py", "sub/report.json"])
def test_archive_paths_are_never_extracted(name):
    with pytest.raises(ValueError):
        poster.read_archive(archive_data(name))


def test_archive_rejects_symlinks_and_large_payloads():
    with pytest.raises(ValueError):
        poster.read_archive(archive_data(mode=0o120777))
    with pytest.raises(ValueError):
        poster.read_archive(archive_data(content=b"x" * (poster.MAX_BYTES + 1)))
    assert poster.read_archive(archive_data()) == {}


def test_current_diff_anchors_only_added_lines():
    files = [
        {
            "filename": "src/new é.h",
            "status": "renamed",
            "previous_filename": "src/old.h",
            "patch": "@@ -3,3 +3,4 @@\n context\n-old\n+new\n+newer\n context\n@@ -50 +51 @@\n-old\n+new",
        },
        {"filename": "deleted.h", "status": "removed", "patch": "@@ -1 +1 @@\n-x\n+x"},
        {"filename": "binary.h", "status": "modified"},
        {"filename": "deletion.h", "status": "modified", "patch": "@@ -1,2 +1 @@\n-x\n y"},
    ]
    assert poster.changed_lines(files) == {
        ("src/new é.h", 4),
        ("src/new é.h", 5),
        ("src/new é.h", 51),
    }


@pytest.fixture
def posting(monkeypatch, live):
    items = {
        "files": [
            {"filename": "src/new é.h", "status": "modified", "patch": "@@ -5 +5 @@\n-old\n+new"}
        ],
        "reviews": [],
        "comments": [],
    }
    monkeypatch.setattr(poster, "list_items", lambda path: items[path.rsplit("/", 1)[-1]])
    send = Mock()
    monkeypatch.setattr(poster.subprocess, "run", send)
    return items, send


def test_post_is_comment_only_and_replay_is_noop(run, report, posting):
    items, send = posting
    report["findings"] *= 2
    assert poster.post_report(REPO, run, run, 42, "clazy", report, dry_run=False) == 1
    payload = json.loads(send.call_args.kwargs["input"])
    assert payload["event"] == "COMMENT"
    assert payload["commit_id"] == SHA
    assert payload["comments"][0]["side"] == "RIGHT"
    assert send.call_args.args[0] == [
        "gh",
        "api",
        "--method",
        "POST",
        f"repos/{REPO}/pulls/42/reviews",
        "--input",
        "-",
    ]
    items["reviews"] = [
        {"user": {"login": poster.BOT, "type": "Bot"}, "body": payload["body"], "commit_id": SHA}
    ]
    send.reset_mock()
    assert poster.post_report(REPO, run, run, 42, "clazy", report, dry_run=False) == 0
    send.assert_not_called()


def test_repeated_finding_across_attempts_is_noop(run, report, posting, finding):
    marker = f"<!-- qgc-analysis-finding:clazy:{poster.fingerprint('clazy', SHA, finding)} -->"
    posting[0]["comments"] = [
        {
            "user": {"login": poster.BOT, "type": "Bot"},
            "body": marker,
            "commit_id": SHA,
            "path": finding["path"],
            "line": finding["line"],
        }
    ]
    assert poster.post_report(REPO, run, run, 42, "clazy", report, dry_run=False) == 0
    posting[1].assert_not_called()


def test_human_marker_does_not_suppress_or_modify_review(run, report, posting):
    posting[0]["reviews"] = [
        {
            "user": {"login": "human", "type": "User"},
            "commit_id": SHA,
            "body": "<!-- qgc-analysis:clazy:100:2 -->",
        }
    ]
    assert poster.post_report(REPO, run, run, 42, "clazy", report, dry_run=False) == 1
    assert posting[1].call_count == 1


@pytest.mark.parametrize("mode", ["empty", "outside-diff", "dry-run", "stale-before-write"])
def test_noop_paths_never_write(run, report, posting, monkeypatch, mode):
    if mode == "empty":
        report["findings"] = []
    elif mode == "outside-diff":
        report["findings"][0]["line"] = 4
    elif mode == "stale-before-write":
        monkeypatch.setattr(poster, "current_run", lambda *args: None)
    count = poster.post_report(REPO, run, run, 42, "clazy", report, dry_run=mode == "dry-run")
    assert count == (1 if mode == "dry-run" else 0)
    posting[1].assert_not_called()


def test_body_bounds_and_untrusted_markup(finding):
    finding["message"] = "<>&" * 600 + "@everyone"
    finding["context"] = "漢" * 1000
    body = poster.comment_body("clazy", finding, "<!-- trusted-marker -->")
    assert len(body.encode()) <= poster.MAX_BODY_BYTES
    assert "<>&" not in body and "@everyone" not in body
    assert "truncated" in body


def test_partial_and_truncated_report_is_disclosed(run, report, posting):
    report["manifest"].update(incomplete=True, truncated=True)
    poster.post_report(REPO, run, run, 42, "clazy", report, dry_run=False)
    body = json.loads(posting[1].call_args.kwargs["input"])["body"]
    assert "partial results" in body and "truncated" in body and "/runs/100/attempts/2" in body


def test_workflow_trust_and_permissions():
    root = Path(__file__).resolve().parents[3]
    producer = yaml.safe_load((root / ".github/workflows/analysis.yml").read_text())
    workflow = yaml.safe_load((root / ".github/workflows/analysis-review.yml").read_text())
    trigger = workflow.get("on", workflow.get(True))
    assert set(trigger) == {"workflow_run"}
    assert trigger["workflow_run"]["workflows"] == ["Code Analysis"]
    job = workflow["jobs"]["post"]
    assert job["permissions"] == {"contents": "read", "actions": "read", "pull-requests": "write"}
    checkout = next(
        step for step in job["steps"] if step.get("uses", "").startswith("actions/checkout@")
    )
    assert checkout["with"]["ref"] == "${{ github.sha }}"
    assert checkout["with"]["persist-credentials"] is False
    assert all(value != "write" for value in producer["permissions"].values())
    assert producer["jobs"]["analyze"]["defaults"]["run"]["shell"] == "bash"
    steps = producer["jobs"]["analyze"]["steps"]
    upload = next(step for step in steps if step["name"] == "Upload inline review data")
    assert upload["if"].startswith("always()")
    assert (
        "${{ matrix.tool }}" in upload["with"]["name"]
        and "${{ github.run_attempt }}" in upload["with"]["name"]
    )
    assert "--review-output" in next(
        step["run"] for step in steps if step["name"] == "Run compiler analysis"
    )
    assert "continue-on-error" not in next(
        step for step in steps if step["name"] == "Run compiler analysis"
    )


@pytest.mark.parametrize(
    "mode", ["valid", "wrong-run", "wrong-sha", "old-attempt", "duplicate", "missing"]
)
def test_main_downloads_only_exact_source_artifact(
    run, live, report, posting, monkeypatch, tmp_path, mode
):
    event = tmp_path / "event.json"
    event.write_text(json.dumps({"repository": {"full_name": REPO}, "workflow_run": run}))
    monkeypatch.setenv("GITHUB_EVENT_PATH", str(event))
    monkeypatch.setenv("GITHUB_EVENT_NAME", "workflow_run")
    monkeypatch.setenv("GITHUB_REPOSITORY", REPO)
    monkeypatch.delenv("GH_REPO", raising=False)
    monkeypatch.setattr("sys.argv", ["poster", "--dry-run"])
    artifact = {
        "id": 123,
        "name": "analysis-review-clazy-2",
        "workflow_run": {"id": 100, "head_sha": SHA},
    }
    if mode == "wrong-run":
        artifact["workflow_run"]["id"] = 99
    elif mode == "wrong-sha":
        artifact["workflow_run"]["head_sha"] = "b" * 40
    elif mode == "old-attempt":
        artifact["name"] = "analysis-review-clazy-1"
    artifacts = [] if mode == "missing" else [artifact] * (2 if mode == "duplicate" else 1)
    listed = Mock(return_value=artifacts)
    download = Mock(return_value=report)
    monkeypatch.setattr(poster, "list_run_artifacts", listed)
    monkeypatch.setattr(poster, "download_report", download)
    if mode in {"wrong-run", "wrong-sha", "duplicate"}:
        with pytest.raises(ValueError):
            poster.main()
    else:
        assert poster.main() == 0
    listed.assert_called_once_with(REPO, 100)
    assert download.call_count == (1 if mode == "valid" else 0)
    posting[1].assert_not_called()


def test_findings_count_limit(run, report):
    report["findings"] *= poster.MAX_FINDINGS + 1
    with pytest.raises(ValueError, match="count"):
        poster.validate_report(report, REPO, run, 42, "clazy")


def test_duplicate_archive_members_are_rejected():
    data = io.BytesIO()
    with zipfile.ZipFile(data, "w") as archive:
        archive.writestr("report.json", "{}")
        archive.writestr("extra.json", "{}")
    with pytest.raises(ValueError):
        poster.read_archive(data.getvalue())


@pytest.mark.parametrize("expired,size", [(True, 100), (False, poster.MAX_BYTES + 1), (False, 0)])
def test_download_rejects_bad_metadata_before_network(expired, size, monkeypatch):
    network = Mock()
    monkeypatch.setattr(poster.httpx, "stream", network)
    with pytest.raises(ValueError):
        poster.download_report(REPO, {"expired": expired, "size_in_bytes": size})
    network.assert_not_called()
