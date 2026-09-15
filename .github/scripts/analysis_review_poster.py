#!/usr/bin/env python3
"""Post bounded, untrusted analysis data using only trusted default-branch code."""

from __future__ import annotations

import argparse
import hashlib
import html
import io
import json
import os
import re
import subprocess
import zipfile
from pathlib import Path, PurePosixPath
from typing import Any

import httpx
from ci_bootstrap import ensure_tools_dir
from report_context import api, resolve_context

ensure_tools_dir(__file__)

from common.gh_actions import list_run_artifacts, list_workflow_runs_for_sha, require_repository

TOOLS = ("clang-tidy", "clazy")
MAX_BYTES = 1_000_000
MAX_FINDINGS = 50
MAX_BODY_BYTES = 4000
BOT = "github-actions[bot]"


def current_run(repo: str, source: dict[str, Any]) -> tuple[dict[str, Any], int] | None:
    """Bind the event to live run/PR data, including fork runs with no PR array."""
    run_id = source["id"]
    if type(run_id) is not int or run_id <= 0:
        raise ValueError("Invalid source run ID")
    run = api(f"repos/{repo}/actions/runs/{run_id}")
    workflow = api(f"repos/{repo}/actions/workflows/analysis.yml")
    if (
        any(
            run.get(key) != source.get(key)
            for key in ("id", "run_attempt", "head_sha", "workflow_id")
        )
        or run.get("repository", {}).get("full_name") != repo
        or run.get("workflow_id") != workflow["id"]
        or run.get("path") != ".github/workflows/analysis.yml"
        or run.get("name") != "Code Analysis"
        or run.get("event") != "pull_request"
        or run.get("status") != "completed"
        or run.get("conclusion")
        not in {"success", "failure", "timed_out", "neutral", "skipped", "action_required"}
        or not re.fullmatch(r"[0-9a-f]{40}", run.get("head_sha", ""))
    ):
        return None
    latest = [
        candidate
        for candidate in list_workflow_runs_for_sha(repo, run["head_sha"])
        if candidate["workflow_id"] == run["workflow_id"]
        and candidate["event"] == "pull_request"
        and candidate["head_branch"] == run["head_branch"]
        and candidate["head_repository"]["full_name"] == run["head_repository"]["full_name"]
    ]
    if not latest or max(candidate["id"] for candidate in latest) != run_id:
        return None
    context = resolve_context(repo, run)
    if context["current"] != "true" or not context["pr"]:
        return None
    number = int(context["pr"])
    pr = api(f"repos/{repo}/pulls/{number}")
    if (
        pr["state"] != "open"
        or pr["head"]["sha"] != run["head_sha"]
        or pr["base"]["repo"]["full_name"] != repo
        or pr["head"]["repo"]["full_name"] != run["head_repository"]["full_name"]
        or pr["head"]["ref"] != run["head_branch"]
    ):
        return None
    return run, number


def download_report(repo: str, artifact: dict[str, Any]) -> dict[str, Any]:
    if artifact["expired"] or not 0 < artifact["size_in_bytes"] <= MAX_BYTES:
        raise ValueError("Expired or oversized review artifact")
    identifier = artifact["id"]
    if type(identifier) is not int or identifier <= 0:
        raise ValueError("Invalid artifact ID")
    data = bytearray()
    # httpx strips Authorization when following the cross-origin signed download redirect.
    with httpx.stream(
        "GET",
        f"https://api.github.com/repos/{repo}/actions/artifacts/{identifier}/zip",
        headers={"Authorization": f"Bearer {os.environ['GH_TOKEN']}"},
        follow_redirects=True,
        timeout=60,
    ) as response:
        response.raise_for_status()
        for chunk in response.iter_bytes():
            data.extend(chunk)
            if len(data) > MAX_BYTES:
                raise ValueError("Review download exceeds size limit")
    return read_archive(bytes(data))


def read_archive(data: bytes) -> dict[str, Any]:
    if len(data) > MAX_BYTES:
        raise ValueError("Review archive exceeds size limit")
    with zipfile.ZipFile(io.BytesIO(data)) as archive:
        members = archive.infolist()
        if (
            len(members) != 1
            or members[0].filename != "report.json"
            or members[0].file_size > MAX_BYTES
            or (members[0].external_attr >> 16) & 0o170000 == 0o120000
        ):
            raise ValueError("Review archive must contain only bounded report.json data")
        # Never extract artifact members onto the trusted checkout.
        return json.loads(archive.read(members[0]))


def validate_report(
    report: dict[str, Any], repo: str, run: dict[str, Any], number: int, tool: str
) -> tuple[dict[str, Any], list[dict[str, Any]]]:
    if not isinstance(report, dict) or set(report) != {"manifest", "findings"}:
        raise ValueError("Invalid review report")
    manifest = report["manifest"]
    expected = {
        "schema": 1,
        "repository": repo,
        "run_id": run["id"],
        "run_attempt": run["run_attempt"],
        "pr_number": number,
        "head_sha": run["head_sha"],
        "tool": tool,
    }
    if (
        not isinstance(manifest, dict)
        or set(manifest) != set(expected) | {"truncated", "incomplete"}
        or any(
            type(manifest[key]) is not type(value) or manifest[key] != value
            for key, value in expected.items()
        )
        or type(manifest["truncated"]) is not bool
        or type(manifest["incomplete"]) is not bool
    ):
        raise ValueError("Review manifest does not match the source run and current PR")
    findings = report["findings"]
    if not isinstance(findings, list) or len(findings) > MAX_FINDINGS:
        raise ValueError("Invalid review finding count")
    for finding in findings:
        if not isinstance(finding, dict) or set(finding) != {
            "path",
            "line",
            "check",
            "level",
            "message",
            "context",
        }:
            raise ValueError("Invalid finding fields")
        for key, limit in (
            ("path", 1024),
            ("check", 200),
            ("level", 7),
            ("message", 2000),
            ("context", 1000),
        ):
            value = finding[key]
            if (
                not isinstance(value, str)
                or len(value) > limit
                or any(ord(char) < 32 and char not in "\n\t" for char in value)
            ):
                raise ValueError("Invalid finding text")
        path = finding["path"]
        if (
            not path
            or PurePosixPath(path).is_absolute()
            or any(part in {"", ".", ".."} for part in path.split("/"))
            or any(char in path for char in "\\\n\t")
            or type(finding["line"]) is not int
            or not 0 < finding["line"] <= 10_000_000
            or finding["level"] not in {"warning", "error"}
            or not re.fullmatch(r"[\w.,=-]{1,200}", finding["check"])
            or not finding["message"]
        ):
            raise ValueError("Invalid finding location or diagnostic")
    return manifest, findings


def list_items(path: str) -> list[dict[str, Any]]:
    items = []
    for page in range(1, 31):
        batch = api(f"{path}?per_page=100&page={page}")
        items.extend(batch)
        if len(batch) < 100:
            return items
    raise ValueError("GitHub list exceeds reporting limit")


def changed_lines(files: list[dict[str, Any]]) -> set[tuple[str, int]]:
    """Accept only added RIGHT-side lines proven by the current GitHub PR patch."""
    locations = set()
    for file in files:
        if file["status"] == "removed":
            continue
        number = None
        for text in file.get("patch", "").splitlines():
            hunk = re.match(r"^@@ -\d+(?:,\d+)? \+(\d+)(?:,\d+)? @@", text)
            if hunk:
                number = int(hunk[1])
            elif number is not None:
                if text.startswith("+"):
                    locations.add((file["filename"], number))
                    number += 1
                elif text.startswith(" "):
                    number += 1
                elif not text.startswith(("-", "\\")):
                    number = None
    return locations


def fingerprint(tool: str, sha: str, finding: dict[str, Any]) -> str:
    identity = [tool, sha, *(finding[key] for key in ("path", "line", "check", "message"))]
    return hashlib.sha256(json.dumps(identity, ensure_ascii=False).encode()).hexdigest()


def comment_body(tool: str, finding: dict[str, Any], marker: str) -> str:
    text = f"{finding['check']} ({finding['level']}): {finding['message']}"
    if finding["context"]:
        text += "\n\n" + finding["context"]
    escaped = html.escape(text).replace("@", "@\u200b")
    prefix = f"{marker}\n**{tool}**\n<pre>"
    suffix = "</pre>"
    budget = MAX_BODY_BYTES - len((prefix + suffix).encode())
    if len(escaped.encode()) > budget:
        escaped = escaped.encode()[: budget - 60].decode("utf-8", errors="ignore")
        escaped += "\n… truncated; see the full output artifact."
    return prefix + escaped + suffix


def post_report(
    repo: str,
    source: dict[str, Any],
    run: dict[str, Any],
    number: int,
    tool: str,
    report: dict[str, Any],
    *,
    dry_run: bool,
) -> int:
    manifest, findings = validate_report(report, repo, run, number, tool)
    if not findings:
        return 0
    endpoint = f"repos/{repo}/pulls/{number}"
    locations = changed_lines(list_items(f"{endpoint}/files"))
    review_marker = f"<!-- qgc-analysis:{tool}:{run['id']}:{run['run_attempt']} -->"
    reviews = list_items(f"{endpoint}/reviews")
    if any(
        review["user"]["login"] == BOT
        and review["user"]["type"] == "Bot"
        and review.get("commit_id") == run["head_sha"]
        and review_marker in (review.get("body") or "").splitlines()
        for review in reviews
    ):
        return 0
    previous = list_items(f"{endpoint}/comments")
    comments = []
    seen = set()
    for finding in findings:
        location = (finding["path"], finding["line"])
        if location not in locations:
            continue
        marker = (
            f"<!-- qgc-analysis-finding:{tool}:{fingerprint(tool, run['head_sha'], finding)} -->"
        )
        if marker in seen:
            continue
        seen.add(marker)
        if any(
            comment["user"]["login"] == BOT
            and comment["user"]["type"] == "Bot"
            and comment.get("commit_id") == run["head_sha"]
            and (comment.get("path"), comment.get("line")) == location
            and marker in (comment.get("body") or "").splitlines()
            for comment in previous
        ):
            continue
        comments.append(
            {
                "path": finding["path"],
                "line": finding["line"],
                "side": "RIGHT",
                "body": comment_body(tool, finding, marker),
            }
        )
    if not comments:
        return 0
    url = f"https://github.com/{repo}/actions/runs/{run['id']}/attempts/{run['run_attempt']}"
    summary = (
        f"{review_marker}\n{tool}: {len(comments)} new changed-line findings. "
        f"[Full report and raw logs]({url}) (`{tool}-output`)."
    )
    if manifest["truncated"]:
        summary += "\nResults were truncated to the inline reporting limits."
    if manifest["incomplete"] or run["conclusion"] != "success":
        summary += "\nAnalysis did not complete successfully; these are partial results, not a clean bill of health."
    payload = {
        "event": "COMMENT",
        "commit_id": run["head_sha"],
        "body": summary,
        "comments": comments,
    }
    # Revalidate immediately before the only write, after downloads and diff/API reads.
    if current_run(repo, source) != (run, number):
        return 0
    if dry_run:
        print(f"Dry run: would post {len(comments)} {tool} comments on PR #{number}")
    else:
        subprocess.run(
            ["gh", "api", "--method", "POST", f"{endpoint}/reviews", "--input", "-"],
            input=json.dumps(payload),
            text=True,
            check=True,
            capture_output=True,
        )
    return len(comments)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()
    repo = require_repository()
    event = json.loads(Path(os.environ["GITHUB_EVENT_PATH"]).read_text(encoding="utf-8"))
    if (
        event["repository"]["full_name"] != repo
        or os.environ["GITHUB_EVENT_NAME"] != "workflow_run"
    ):
        raise ValueError("Expected an upstream workflow_run event")
    source = event["workflow_run"]
    context = current_run(repo, source)
    if context is None:
        print("Ignoring ineligible or stale analysis run")
        return 0
    run, number = context
    artifacts = list_run_artifacts(repo, run["id"])
    for tool in TOOLS:
        name = f"analysis-review-{tool}-{run['run_attempt']}"
        matches = [artifact for artifact in artifacts if artifact["name"] == name]
        if not matches:
            continue
        if len(matches) != 1:
            raise ValueError("Ambiguous review artifact")
        artifact = matches[0]
        if (
            artifact["workflow_run"]["id"] != run["id"]
            or artifact["workflow_run"]["head_sha"] != run["head_sha"]
        ):
            raise ValueError("Artifact is not associated with the source run")
        report = download_report(repo, artifact)
        count = post_report(repo, source, run, number, tool, report, dry_run=args.dry_run)
        print(f"{tool}: {count} new inline findings")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
