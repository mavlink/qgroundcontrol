#!/usr/bin/env python3
"""Build the GitHub Pages site: CI health dashboard, schemas, master downloads, and API docs.

``build --out DIR`` writes the whole site, because every Pages deploy replaces the last one:

- ``index.html`` and ``data/history.json``: the CI health dashboard of master push builds
  (pages_history.py collects, pages_render.py draws). The deployed history is fetched and
  extended; any fetch failure stops the build instead of deploying a truncated history. While
  the site has none (HTTP 404) and pages.yml never deployed it, or with ``--new-history``, it
  starts empty and backfills from the latest runs whose artifacts remain. Collection stops after
  15 minutes; the next build collects the runs left.
- ``schemas/``: the JSON schemas, each of whose ``$id`` must be its URL on the site.
- ``downloads/``: the latest master packages on S3 with their published SHA-256 sidecars and the
  commit and run their provenance attestation names; a missing sidecar shows "—".
- ``api/``: the Doxygen HTML of the latest successful non-PR Doxygen run on master, or a page
  linking api.qgroundcontrol.com when no artifact is available or its download stalls.

GitHub is read through gh (GH_TOKEN). The repository defaults to GITHUB_REPOSITORY, and
``--new-history`` to PAGES_NEW_HISTORY.
"""

from __future__ import annotations

import argparse
import base64
import binascii
import json
import os
import re
import shutil
import subprocess
import sys
import time
import zipfile
from dataclasses import dataclass
from datetime import datetime, timezone
from html.parser import HTMLParser
from http.client import HTTPException
from pathlib import Path
from typing import TYPE_CHECKING, Any
from urllib.error import HTTPError
from urllib.parse import unquote, urlencode, urlsplit

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import _bootstrap as _bootstrap
from common.build_config import get_build_config_value
from common.format import format_bytes, format_count
from common.gh_actions import (
    download_artifact_zip,
    env_flag,
    gh_api_json,
    gh_error,
    gh_warning,
    list_run_artifacts,
    parse_csv_list,
    write_step_summary,
)
from common.io import extract_zip_safe, read_json, write_json
from common.markdown import code_span, md_table, summary_section
from common.net import fetch_bytes
from common.paths import REPO_ROOT
from qgc_tools.build_artifacts import PUBLISHED_BUILDS, PublishedBuild, published_build_url

from ci.publish.pages_history import (
    ANALYSIS_WORKFLOW,
    BACKFILL_RUNS,
    COLLECT_SECONDS,
    HISTORY_PATH,
    RECENT_RUNS,
    HistoryError,
    collect_new_records,
    empty_history,
    fetch_history,
    merge_history,
    record_key,
    require_first_deploy,
    run_url,
)
from ci.publish.pages_render import dashboard, esc, link, page, table

if TYPE_CHECKING:
    from collections.abc import Iterable

SITE_URL = "https://mavlink.github.io/qgroundcontrol/"
# (repository path, published name under schemas/)
SCHEMAS = (("src/FactSystem/factmetadata.schema.json", "factmetadata.schema.json"),)
DOWNLOADS_REF = "master"
API_DOCS_URL = "https://api.qgroundcontrol.com"
DOXYGEN_WORKFLOW = "doxygen.yml"
DOXYGEN_ARTIFACT = "doxygen_build"
# The ~280 MB archive normally downloads in seconds; a stalled transfer gets the link page.
DOXYGEN_TIMEOUT_SECONDS = 150.0
DOXYGEN_ATTEMPTS = 2
# Pull request runs (which can come from a fork's master branch) never feed the site.
NON_PR_EVENTS = frozenset({"push", "schedule", "workflow_dispatch"})
SLSA_PROVENANCE = "https://slsa.dev/provenance/"
MISSING = "—"

_SHA256 = re.compile(r"[0-9a-f]{64}")
_COMMIT = re.compile(r"[0-9a-f]{40}")
_GH_ERRORS = (subprocess.SubprocessError, RuntimeError, ValueError, OSError)


class SiteError(RuntimeError):
    """The site cannot be built correctly."""


@dataclass(frozen=True)
class Download:
    build: PublishedBuild
    url: str
    sha256: str | None = None
    commit: str | None = None
    run: str | None = None


def schema_url(name: str) -> str:
    return f"{SITE_URL}schemas/{name}"


def publish_schemas(out: Path, repo: str, *, now: datetime) -> list[dict[str, str]]:
    """Copy each schema to ``schemas/`` and list them; a ``$id`` other than its URL is an error."""
    target = out / "schemas"
    target.mkdir(parents=True, exist_ok=True)
    schemas = []
    for source, name in SCHEMAS:
        data = read_json(REPO_ROOT / source)
        if not isinstance(data, dict) or data.get("$id") != schema_url(name):
            found = data.get("$id") if isinstance(data, dict) else None
            raise SiteError(f"{source} has $id {found!r}; the site serves it at {schema_url(name)}")
        shutil.copyfile(REPO_ROOT / source, target / name)
        schemas.append(
            {
                "name": name,
                "title": str(data.get("title", name)),
                "description": str(data.get("description", "")),
                "source": source,
            }
        )
    rows = [
        [
            link(schema["name"], schema["name"]),
            esc(schema["title"]),
            esc(schema["description"]),
            link(schema["source"], f"https://github.com/{repo}/blob/master/{schema['source']}"),
        ]
        for schema in schemas
    ]
    body = (
        "<p>JSON schemas published by QGroundControl. Each schema's <code>$id</code> is its URL "
        "here, so editors and validators can resolve it.</p>"
        + table(
            ["Schema", "Title", "Description", "Source"], rows, caption="Schemas", numeric=set()
        )
    )
    write_page(
        target,
        page("Schemas", body, depth=1, description="QGroundControl JSON schemas", generated=now),
    )
    return schemas


def read_checksum(url: str, name: str) -> str | None:
    """Return the SHA-256 in a ``<name>.sha256`` sidecar, or None when it is missing or invalid."""
    try:
        body = fetch_bytes(url, timeout=30)
    except HTTPError as exc:
        if exc.code not in (403, 404):  # S3 answers 403 for a missing public object
            gh_warning(f"Cannot fetch {url}: HTTP {exc.code}")
        return None
    except (HTTPException, OSError, RuntimeError) as exc:
        gh_warning(f"Cannot fetch {url}: {exc}")
        return None
    fields = body.decode("utf-8", "replace").split()
    digest = fields[0].lower() if fields else ""
    if not _SHA256.fullmatch(digest) or (len(fields) > 1 and fields[1].lstrip("*") != name):
        gh_warning(f"{url} is not a SHA-256 sidecar of {name}")
        return None
    return digest


def _provenance_source(statement: dict[str, Any], repo: str, digest: str) -> tuple[str, str] | None:
    if not str(statement.get("predicateType", "")).startswith(SLSA_PROVENANCE):
        return None
    subjects = statement.get("subject") or []
    if not any(
        isinstance(s, dict) and (s.get("digest") or {}).get("sha256") == digest for s in subjects
    ):
        return None
    predicate = statement.get("predicate") or {}
    build = predicate.get("buildDefinition") or {}
    workflow = (build.get("externalParameters") or {}).get("workflow") or {}
    if workflow.get("repository") != f"https://github.com/{repo}":
        return None
    if workflow.get("ref") != f"refs/heads/{DOWNLOADS_REF}":
        return None
    commit = next(
        (
            commit
            for dependency in build.get("resolvedDependencies") or []
            if isinstance(dependency, dict)
            and isinstance(commit := (dependency.get("digest") or {}).get("gitCommit"), str)
            and _COMMIT.fullmatch(commit)
        ),
        None,
    )
    invocation = str(
        ((predicate.get("runDetails") or {}).get("metadata") or {}).get("invocationId", "")
    )
    match = re.fullmatch(
        rf"https://github\.com/{re.escape(repo)}/actions/runs/(\d+)(?:/attempts/\d+)?", invocation
    )
    return (commit, run_url(repo, int(match.group(1)))) if commit and match else None


def build_provenance(repo: str, digest: str) -> tuple[str, str] | None:
    """Return the commit and run URL of the master build attested for a package's SHA-256."""
    try:
        data = gh_api_json(f"repos/{repo}/attestations/sha256:{digest}?per_page=30")
    except _GH_ERRORS as exc:
        gh_warning(f"Cannot read the attestations of sha256:{digest}: {exc}")
        return None
    for attestation in (data or {}).get("attestations") or []:
        envelope = ((attestation or {}).get("bundle") or {}).get("dsseEnvelope") or {}
        try:
            statement = json.loads(base64.b64decode(envelope.get("payload") or "", validate=True))
        except (binascii.Error, ValueError):
            continue
        if isinstance(statement, dict) and (source := _provenance_source(statement, repo, digest)):
            return source
    return None


def publish_downloads(out: Path, repo: str, *, now: datetime) -> list[Download]:
    """Write ``downloads/`` for the packages published to S3 ``builds/master/``."""
    downloads = []
    for build in PUBLISHED_BUILDS:
        url = published_build_url(DOWNLOADS_REF, build.name)
        digest = read_checksum(f"{url}.sha256", build.name)
        source = build_provenance(repo, digest) if digest else None
        commit, run = source if source else (None, None)
        downloads.append(Download(build, url, digest, commit, run))
    rows = [
        [
            esc(d.build.platform),
            link(d.build.name, d.url)
            + f'<br><span class="muted">{esc(d.build.description)}</span>',
            f"<code>{esc(d.sha256)}</code>" if d.sha256 else MISSING,
            (
                link(d.commit[:7], f"https://github.com/{repo}/commit/{d.commit}")
                + " · "
                + link("run", d.run)
                if d.commit and d.run
                else MISSING
            ),
        ]
        for d in downloads
    ]
    example = PUBLISHED_BUILDS[0].name
    example_url = published_build_url(DOWNLOADS_REF, example)
    body = (
        "<p>Packages of the latest <code>master</code> commit that the platform workflows "
        "published. They are development builds: for flying, use a "
        f"{link('stable release', f'https://github.com/{repo}/releases')}, described in the "
        + link(
            "user guide",
            "https://docs.qgroundcontrol.com/master/en/qgc-user-guide/getting_started/download_and_install.html",
        )
        + ".</p>\n"
        + table(
            ["Platform", "Package", "SHA-256", "Built from"],
            rows,
            caption="Latest master builds",
            numeric=set(),
        )
        + "\n<h2>Verify a download</h2>\n<p>Each package has a build provenance attestation; "
        "verify it with the GitHub CLI, and compare the checksum with the published sidecar:</p>\n"
        f"<pre><code>curl -LO {esc(example_url)}\n"
        f"curl -LO {esc(example_url)}.sha256\n"
        f"gh attestation verify {esc(example)} --repo {esc(repo)}\n"
        f"sha256sum -c {esc(example)}.sha256</code></pre>\n"
        "<p>On macOS use <code>shasum -a 256 -c</code>. Replace the file name for other "
        f"packages: <code>gh attestation verify &lt;file&gt; --repo {esc(repo)}</code>.</p>"
    )
    target = out / "downloads"
    target.mkdir(parents=True, exist_ok=True)
    write_page(
        target,
        page(
            "Master downloads",
            body,
            depth=1,
            description="Latest QGroundControl master builds",
            generated=now,
        ),
    )
    return downloads


def latest_doxygen_artifact(repo: str) -> tuple[dict[str, Any], dict[str, Any]] | None:
    """Return the latest successful non-PR Doxygen run on master with a live doxygen_build."""
    query = urlencode({"branch": "master", "status": "success", "per_page": 20})
    data = gh_api_json(f"repos/{repo}/actions/workflows/{DOXYGEN_WORKFLOW}/runs?{query}")
    for run in data.get("workflow_runs") or []:
        if (
            run.get("event") not in NON_PR_EVENTS
            or run.get("head_branch") != "master"
            or (run.get("head_repository") or {}).get("full_name") != repo
        ):
            continue
        for artifact in list_run_artifacts(repo, int(run["id"])):
            if artifact.get("name") == DOXYGEN_ARTIFACT and not artifact.get("expired"):
                return run, artifact
    return None


def publish_api_docs(out: Path, repo: str, work_dir: Path, *, now: datetime) -> str | None:
    """Write ``api/`` from the latest Doxygen artifact and return its run URL, else a link page."""
    target = out / "api"
    try:
        found = latest_doxygen_artifact(repo)
        if found:
            run, artifact = found
            archive = work_dir / "doxygen.zip"
            download_artifact_zip(
                repo,
                artifact["id"],
                archive,
                attempts=DOXYGEN_ATTEMPTS,
                timeout=DOXYGEN_TIMEOUT_SECONDS,
            )
            extract_zip_safe(archive, target)
            archive.unlink()
            if (target / "index.html").is_file():
                return run_url(repo, int(run["id"]))
            gh_warning(f"The {DOXYGEN_ARTIFACT} artifact has no index.html")
        else:
            gh_warning(f"No {DOXYGEN_ARTIFACT} artifact from a successful master Doxygen run")
    except (*_GH_ERRORS, zipfile.BadZipFile) as exc:
        gh_warning(f"Cannot publish the Doxygen API docs: {exc}")
    shutil.rmtree(target, ignore_errors=True)
    target.mkdir(parents=True)
    body = (
        "<p>The API reference generated by Doxygen is published at "
        f"{link(API_DOCS_URL, API_DOCS_URL)}.</p>"
    )
    write_page(
        target,
        page(
            "API reference",
            body,
            depth=1,
            description="QGroundControl API reference",
            generated=now,
        ),
    )
    return None


def write_page(directory: Path, html: str) -> None:
    directory.mkdir(parents=True, exist_ok=True)
    (directory / "index.html").write_text(html, encoding="utf-8")


class _LinkParser(HTMLParser):
    def __init__(self) -> None:
        super().__init__()
        self.links: list[str] = []
        self.ids: set[str] = set()

    def handle_starttag(self, tag: str, attrs: list[tuple[str, str | None]]) -> None:
        for name, value in attrs:
            if value is None:
                continue
            if name in {"href", "src"}:
                self.links.append(value)
            elif name == "id":
                self.ids.add(value)


def broken_links(out: Path, pages: Iterable[Path]) -> list[str]:
    """Return the relative links and same-page anchors in *pages* that lead nowhere in *out*."""
    root = out.resolve()
    broken = []
    for path in pages:
        parser = _LinkParser()
        parser.feed(path.read_text(encoding="utf-8"))
        for ref in parser.links:
            parts = urlsplit(ref)
            if parts.scheme or parts.netloc:
                continue
            if not parts.path:
                if parts.fragment and parts.fragment not in parser.ids:
                    broken.append(f"{path.relative_to(out).as_posix()}: {ref}")
                continue
            target = (path.parent / unquote(parts.path)).resolve()
            if target.is_dir():
                target /= "index.html"
            if not target.is_relative_to(root) or not target.is_file():
                broken.append(f"{path.relative_to(out).as_posix()}: {ref}")
    return broken


def _site_size(out: Path) -> tuple[int, int]:
    files = [path for path in out.rglob("*") if path.is_file()]
    return len(files), sum(path.stat().st_size for path in files)


def build_site(
    out: Path, *, repo: str, work_dir: Path, now: datetime, new_history: bool = False
) -> str:
    """Build the site into *out* and return its job-summary section.

    *new_history* lets a site without a history (HTTP 404) start a new one although pages.yml
    deployed it before, which drops the earlier records.
    """
    collect_deadline = time.monotonic() + COLLECT_SECONDS
    if out.exists() and any(out.iterdir()):
        raise SiteError(f"{out} is not empty")
    workflows = parse_csv_list(get_build_config_value("build.platform_workflows"))
    if not workflows:
        raise SiteError("build.platform_workflows is missing from build-config.json")
    workflows.append(ANALYSIS_WORKFLOW)

    # Read the deployed history before anything else: without it there is nothing to deploy.
    deployed = fetch_history(SITE_URL + HISTORY_PATH)
    if deployed is not None and deployed.get("repository", repo) != repo:
        raise SiteError(f"the deployed history belongs to {deployed.get('repository')}")
    if deployed is None:
        if new_history:
            gh_warning("The site has no history; starting a new one as requested")
        else:
            require_first_deploy(repo)
    history = deployed or empty_history(repo)
    per_workflow = RECENT_RUNS if deployed is not None else BACKFILL_RUNS
    out.mkdir(parents=True, exist_ok=True)
    try:
        new_records, pending = collect_new_records(
            repo,
            history,
            workflows,
            work_dir,
            per_workflow=per_workflow,
            deadline=collect_deadline,
        )
        merged = merge_history(history, new_records, now=now)
        api_run = publish_api_docs(out, repo, work_dir, now=now)
    finally:
        shutil.rmtree(work_dir, ignore_errors=True)
    write_json(out / HISTORY_PATH, merged, indent=None)
    dashboard_html = dashboard(merged, repo=repo, workflows=workflows, now=now)
    (out / "index.html").write_text(dashboard_html, encoding="utf-8")
    schemas = publish_schemas(out, repo, now=now)
    downloads = publish_downloads(out, repo, now=now)
    (out / ".nojekyll").write_text("", encoding="utf-8")

    pages = [out / "index.html", out / "downloads/index.html", out / "schemas/index.html"]
    if api_run is None:
        pages.append(out / "api/index.html")
    if broken := broken_links(out, pages):
        raise SiteError("broken links: " + ", ".join(broken))

    kept = {record_key(record) for record in merged["records"]}
    dropped = len({record_key(record) for record in history["records"]} - kept)
    files, size = _site_size(out)
    facts = [
        (
            "History",
            f"{format_count(len(merged['records']))} runs of "
            f"{format_count(len({key[0] for key in kept}))} commits; {len(new_records)} new, "
            f"{dropped} past the caps"
            + (f", {pending} left for the next build" if pending else "")
            + ("" if deployed is not None else " (backfilled)"),
        ),
        ("Dashboard", format_bytes(len(dashboard_html.encode()))),
        (
            "Downloads",
            f"{sum(d.sha256 is not None for d in downloads)} of {len(downloads)} checksums, "
            f"{sum(d.commit is not None for d in downloads)} with their commit",
        ),
        ("API docs", f"[Doxygen run]({api_run})" if api_run else f"link to {API_DOCS_URL}"),
        ("Schemas", ", ".join(code_span(schema["name"]) for schema in schemas)),
        ("Site", f"{format_count(files)} files, {format_bytes(size)}"),
    ]
    listing = md_table(
        ["Package", "SHA-256", "Commit"],
        [
            [d.build.name, d.sha256 or MISSING, d.commit[:7] if d.commit else MISSING]
            for d in downloads
        ],
    )
    return summary_section("Pages site", facts, collapsed=[("Downloads", listing)])


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    sub = parser.add_subparsers(dest="command", required=True)
    build = sub.add_parser("build", help="Build the whole site into --out")
    build.add_argument("--out", type=Path, required=True, help="Empty or new site directory")
    repo = os.environ.get("GITHUB_REPOSITORY", "")
    build.add_argument(
        "--repo", default=repo, required=not repo, help="owner/name (default: $GITHUB_REPOSITORY)"
    )
    build.add_argument(
        "--work-dir",
        type=Path,
        default=None,
        help="Scratch directory for artifact downloads, removed afterwards "
        "(default: $RUNNER_TEMP/pages-work, else pages-work beside --out)",
    )
    build.add_argument(
        "--new-history",
        action="store_true",
        default=env_flag("PAGES_NEW_HISTORY"),
        help="Start a new history when the site has none (HTTP 404) although pages.yml deployed "
        "it before, dropping the earlier records (default: $PAGES_NEW_HISTORY)",
    )
    args = parser.parse_args(argv)
    if args.work_dir is None:
        args.work_dir = Path(os.environ.get("RUNNER_TEMP") or args.out.parent) / "pages-work"
    return args


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    try:
        summary = build_site(
            args.out,
            repo=args.repo,
            work_dir=args.work_dir,
            now=datetime.now(timezone.utc),
            new_history=args.new_history,
        )
    except (SiteError, HistoryError) as exc:
        gh_error(str(exc), title="Pages site")
        return 1
    write_step_summary(summary)
    print(f"Built the site in {args.out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
