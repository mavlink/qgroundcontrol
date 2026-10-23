"""Tests for pages_site.py: schemas, downloads, API docs, link checks, and the whole build."""

from __future__ import annotations

import base64
import io
import json
import subprocess
import time
import zipfile
from datetime import datetime, timezone
from typing import TYPE_CHECKING, Any
from urllib.error import HTTPError

import pytest
from ci.publish import pages_history
from ci.publish import pages_site as mod
from qgc_tools.build_artifacts import PUBLISHED_BUILDS

from .._helpers import REPO_ROOT

if TYPE_CHECKING:
    from pathlib import Path

REPO = "mavlink/qgroundcontrol"
NOW = datetime(2026, 10, 23, 23, 0, tzinfo=timezone.utc)
COMMIT = "6f5b75cf8721ae45c2ea28d1f09b880cbbe72a5d"
DIGEST = "c42c37f850cf8372c85c73823f08da02a8d500606a172e10bd938259d1772005"
APPIMAGE = "QGroundControl-x86_64.AppImage"


# ---------------------------------------------------------------------------
# Schemas
# ---------------------------------------------------------------------------


def test_published_schema_ids_are_their_site_urls() -> None:
    for source, name in mod.SCHEMAS:
        schema = json.loads((REPO_ROOT / source).read_text(encoding="utf-8"))
        assert schema["$id"] == f"{mod.SITE_URL}schemas/{name}"
    assert mod.schema_url("factmetadata.schema.json") == (
        "https://mavlink.github.io/qgroundcontrol/schemas/factmetadata.schema.json"
    )


def test_publish_schemas_copies_and_lists_them(tmp_path: Path) -> None:
    schemas = mod.publish_schemas(tmp_path, REPO, now=NOW)
    source = REPO_ROOT / "src/FactSystem/factmetadata.schema.json"
    assert (tmp_path / "schemas/factmetadata.schema.json").read_bytes() == source.read_bytes()
    assert schemas[0]["title"] == "QGC FactMetaData"
    index = (tmp_path / "schemas/index.html").read_text(encoding="utf-8")
    assert '<a href="factmetadata.schema.json">factmetadata.schema.json</a>' in index


def test_a_schema_id_other_than_its_url_fails(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    (tmp_path / "x.schema.json").write_text(
        json.dumps({"$id": "https://qgroundcontrol.com/schemas/x.schema.json"}), encoding="utf-8"
    )
    monkeypatch.setattr(mod, "REPO_ROOT", tmp_path)
    monkeypatch.setattr(mod, "SCHEMAS", (("x.schema.json", "x.schema.json"),))
    with pytest.raises(mod.SiteError, match="the site serves it at"):
        mod.publish_schemas(tmp_path / "site", REPO, now=NOW)


# ---------------------------------------------------------------------------
# Downloads
# ---------------------------------------------------------------------------


def _statement(**overrides: Any) -> dict[str, Any]:
    statement: dict[str, Any] = {
        "_type": "https://in-toto.io/Statement/v1",
        "subject": [{"name": APPIMAGE, "digest": {"sha256": DIGEST}}],
        "predicateType": "https://slsa.dev/provenance/v1",
        "predicate": {
            "buildDefinition": {
                "buildType": "https://actions.github.io/buildtypes/workflow/v1",
                "externalParameters": {
                    "workflow": {
                        "ref": "refs/heads/master",
                        "repository": f"https://github.com/{REPO}",
                        "path": ".github/workflows/linux.yml",
                    }
                },
                "resolvedDependencies": [
                    {
                        "uri": f"git+https://github.com/{REPO}@refs/heads/master",
                        "digest": {"gitCommit": COMMIT},
                    }
                ],
            },
            "runDetails": {
                "metadata": {
                    "invocationId": f"https://github.com/{REPO}/actions/runs/40021772122/attempts/1"
                }
            },
        },
    }
    statement.update(overrides)
    return statement


def _attestations(*statements: dict[str, Any]) -> dict[str, Any]:
    return {
        "attestations": [
            {
                "bundle": {
                    "dsseEnvelope": {
                        "payload": base64.b64encode(json.dumps(s).encode()).decode(),
                        "payloadType": "application/vnd.in-toto+json",
                    }
                }
            }
            for s in statements
        ]
    }


def _sbom() -> dict[str, Any]:
    return {**_statement(), "predicateType": "https://spdx.dev/Document/v2.3", "predicate": {}}


def test_provenance_names_the_commit_and_run() -> None:
    assert mod._provenance_source(_statement(), REPO, DIGEST) == (
        COMMIT,
        f"https://github.com/{REPO}/actions/runs/40021772122",
    )


@pytest.mark.parametrize(
    "statement",
    [
        _sbom(),
        _statement(subject=[{"name": APPIMAGE, "digest": {"sha256": "0" * 64}}]),
        _statement(
            predicate={
                **_statement()["predicate"],
                "runDetails": {
                    "metadata": {"invocationId": "https://github.com/x/y/actions/runs/1"}
                },
            }
        ),
    ],
)
def test_provenance_of_another_subject_or_repository_is_ignored(statement: dict) -> None:
    assert mod._provenance_source(statement, REPO, DIGEST) is None


def test_provenance_from_another_branch_is_ignored() -> None:
    statement = _statement()
    workflow = statement["predicate"]["buildDefinition"]["externalParameters"]["workflow"]
    workflow["ref"] = "refs/heads/feature"
    assert mod._provenance_source(statement, REPO, DIGEST) is None


@pytest.fixture
def s3(monkeypatch: pytest.MonkeyPatch) -> dict[str, Any]:
    """Fake CloudFront sidecars by URL (bytes, or an HTTP status) and attestations by digest."""
    state: dict[str, Any] = {"sidecars": {}, "attestations": {}}

    def fetch(url: str, *, timeout: float) -> bytes:
        found = state["sidecars"].get(url, 404)
        if isinstance(found, int):
            raise HTTPError(url, found, "error", {}, None)  # type: ignore[arg-type]
        return found

    def api(path: str) -> Any:
        digest = path.split("sha256:")[1].split("?")[0]
        if digest not in state["attestations"]:
            raise subprocess.CalledProcessError(1, ["gh", "api", path], stderr="HTTP 404")
        return state["attestations"][digest]

    monkeypatch.setattr(mod, "fetch_bytes", fetch)
    monkeypatch.setattr(mod, "gh_api_json", api)
    return state


def _sidecar_url(name: str) -> str:
    return f"https://d176tv9ibo4jno.cloudfront.net/builds/master/{name}.sha256"


def test_downloads_page_shows_checksums_and_sources_and_dashes_for_missing(
    s3: dict[str, Any], tmp_path: Path, capsys: pytest.CaptureFixture[str]
) -> None:
    s3["sidecars"][_sidecar_url(APPIMAGE)] = f"{DIGEST}  {APPIMAGE}\n".encode()
    s3["sidecars"][_sidecar_url("QGroundControl.dmg")] = f"{'a' * 64}  other.dmg\n".encode()
    s3["sidecars"][_sidecar_url("QGroundControl.apk")] = 500
    # CloudFront answers 403 for an object the bucket lacks.
    s3["sidecars"][_sidecar_url("QGroundControl-installer-ARM64.exe")] = 403
    s3["attestations"][DIGEST] = _attestations(_sbom(), _statement())

    downloads = mod.publish_downloads(tmp_path, REPO, now=NOW)

    assert [d.build.name for d in downloads] == [b.name for b in PUBLISHED_BUILDS]
    by_name = {d.build.name: d for d in downloads}
    assert by_name[APPIMAGE].sha256 == DIGEST
    assert by_name[APPIMAGE].commit == COMMIT
    assert by_name["QGroundControl.dmg"].sha256 is None
    assert by_name["QGroundControl.apk"].sha256 is None
    html = (tmp_path / "downloads/index.html").read_text(encoding="utf-8")
    assert f"<code>{DIGEST}</code>" in html
    assert f'href="https://github.com/{REPO}/commit/{COMMIT}">6f5b75c</a>' in html
    assert html.count("<td>—</td>") == 2 * (len(PUBLISHED_BUILDS) - 1)
    assert f"gh attestation verify {PUBLISHED_BUILDS[0].name} --repo {REPO}" in html
    assert f'href="https://github.com/{REPO}/releases"' in html
    out = capsys.readouterr().out
    # A missing sidecar is expected; a server error or a sidecar of another file is not.
    assert "QGroundControl-installer-AMD64.exe.sha256" not in out
    assert "QGroundControl-installer-ARM64.exe.sha256" not in out
    assert "QGroundControl.apk.sha256: HTTP 500" in out
    assert "QGroundControl.dmg.sha256 is not a SHA-256 sidecar" in out


# ---------------------------------------------------------------------------
# API docs
# ---------------------------------------------------------------------------


def _zip(members: dict[str, str]) -> bytes:
    buffer = io.BytesIO()
    with zipfile.ZipFile(buffer, "w") as archive:
        for name, text in members.items():
            archive.writestr(name, text)
    return buffer.getvalue()


def _doxygen_run(run_id: int, event: str, head_repo: str = REPO) -> dict[str, Any]:
    return {
        "id": run_id,
        "event": event,
        "head_branch": "master",
        "conclusion": "success",
        "head_repository": {"full_name": head_repo},
    }


@pytest.fixture
def doxygen(monkeypatch: pytest.MonkeyPatch) -> dict[str, Any]:
    state: dict[str, Any] = {
        "runs": [
            _doxygen_run(1, "pull_request"),
            _doxygen_run(2, "pull_request", "someone/qgroundcontrol"),
            _doxygen_run(3, "schedule"),
        ],
        "artifacts": {3: [{"id": 30, "name": "doxygen_build", "expired": False}]},
        "zip": _zip({"index.html": "<html>Doxygen</html>", "search/search.js": ""}),
    }

    def api(path: str) -> Any:
        assert path.startswith(f"repos/{REPO}/actions/workflows/doxygen.yml/runs?branch=master")
        return {"workflow_runs": state["runs"]}

    def download(
        repo: str, artifact_id: int, destination: Path, *, attempts: int, timeout: float
    ) -> None:
        state["limits"] = (attempts, timeout)
        if isinstance(state["zip"], Exception):
            raise state["zip"]
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_bytes(state["zip"])

    monkeypatch.setattr(mod, "gh_api_json", api)
    monkeypatch.setattr(
        mod, "list_run_artifacts", lambda repo, run_id: state["artifacts"].get(run_id, [])
    )
    monkeypatch.setattr(mod, "download_artifact_zip", download)
    return state


def test_api_docs_come_from_the_latest_non_pr_master_run(doxygen: dict, tmp_path: Path) -> None:
    # Pull request runs, a fork's from its master branch included, are never published.
    doxygen["artifacts"][1] = doxygen["artifacts"][2] = [{"id": 9, "name": "doxygen_build"}]
    run = mod.publish_api_docs(tmp_path / "site", REPO, tmp_path / "work", now=NOW)
    assert run == f"https://github.com/{REPO}/actions/runs/3"
    assert (tmp_path / "site/api/index.html").read_text() == "<html>Doxygen</html>"
    assert (tmp_path / "site/api/search/search.js").is_file()
    assert doxygen["limits"] == (mod.DOXYGEN_ATTEMPTS, mod.DOXYGEN_TIMEOUT_SECONDS)


@pytest.mark.parametrize(
    "change",
    [
        {"artifacts": {}},
        {"artifacts": {3: [{"id": 30, "name": "doxygen_build", "expired": True}]}},
        {"zip": b"corrupt"},
        {"zip": _zip({"readme.txt": "no index"})},
        {"zip": RuntimeError("Cannot download artifact 30: transfer timed out after 150s")},
    ],
)
def test_api_docs_fall_back_to_a_link(
    doxygen: dict, tmp_path: Path, change: dict, capsys: pytest.CaptureFixture[str]
) -> None:
    doxygen.update(change)
    assert mod.publish_api_docs(tmp_path / "site", REPO, tmp_path / "work", now=NOW) is None
    html = (tmp_path / "site/api/index.html").read_text(encoding="utf-8")
    assert '<a href="https://api.qgroundcontrol.com">https://api.qgroundcontrol.com</a>' in html
    assert sorted(p.name for p in (tmp_path / "site/api").iterdir()) == ["index.html"]
    assert "::warning::" in capsys.readouterr().out


def test_api_docs_fall_back_when_github_fails(
    monkeypatch: pytest.MonkeyPatch, tmp_path: Path
) -> None:
    def fail(path: str) -> Any:
        raise subprocess.CalledProcessError(1, ["gh", "api", path], stderr="HTTP 401")

    monkeypatch.setattr(mod, "gh_api_json", fail)
    assert mod.publish_api_docs(tmp_path, REPO, tmp_path / "work", now=NOW) is None
    html = (tmp_path / "api/index.html").read_text(encoding="utf-8")
    assert '<a href="https://api.qgroundcontrol.com">' in html


# ---------------------------------------------------------------------------
# Links and the whole site
# ---------------------------------------------------------------------------


def test_broken_links_finds_missing_targets_and_anchors(tmp_path: Path) -> None:
    (tmp_path / "downloads").mkdir()
    (tmp_path / "downloads/index.html").write_text("<p>ok</p>")
    (tmp_path / "index.html").write_text(
        '<h2 id="linux">Linux</h2><a href="#linux">ok</a><a href="downloads/">ok</a>'
        '<a href="https://github.com/x">external</a><a href="#windows">no anchor</a>'
        '<a href="schemas/">missing</a><a href="../outside.html">outside</a>'
    )
    (tmp_path.parent / "outside.html").write_text("")
    assert mod.broken_links(tmp_path, [tmp_path / "index.html"]) == [
        "index.html: #windows",
        "index.html: schemas/",
        "index.html: ../outside.html",
    ]


@pytest.fixture
def offline_site(monkeypatch: pytest.MonkeyPatch, s3: dict, doxygen: dict) -> dict[str, Any]:
    """Every network call of a site build faked: no history yet, one Linux run, no Doxygen."""
    state: dict[str, Any] = {
        "history": 404,
        "deploys": 0,
        "deploy_checks": [],
        "per_workflow": [],
        "pending": 0,
    }
    doxygen["runs"] = []

    def fetch_history(url: str) -> dict | None:
        assert url == "https://mavlink.github.io/qgroundcontrol/data/history.json"
        if isinstance(state["history"], Exception):
            raise state["history"]
        return None if state["history"] == 404 else state["history"]

    def pages_runs(path: str) -> dict:
        state["deploy_checks"].append(path)
        if isinstance(state["deploys"], Exception):
            raise state["deploys"]
        return {"total_count": state["deploys"], "workflow_runs": []}

    def collect(
        repo: str,
        history: dict,
        workflows: list,
        work_dir: Path,
        *,
        per_workflow: int,
        deadline: float,
    ) -> pages_history.Collection:
        state["per_workflow"].append(per_workflow)
        state["workflows"] = workflows
        state["deadline"] = deadline
        record = {
            "commit": COMMIT,
            "workflow": "Linux",
            "run_id": 40021772122,
            "attempt": 1,
            "created_at": "2026-10-23T20:46:14Z",
            "conclusion": "success",
            "wall_seconds": 773,
        }
        return pages_history.Collection([record], state["pending"])

    monkeypatch.setattr(mod, "fetch_history", fetch_history)
    monkeypatch.setattr(pages_history, "gh_api_json", pages_runs)
    monkeypatch.setattr(mod, "collect_new_records", collect)
    monkeypatch.delenv("PAGES_NEW_HISTORY", raising=False)
    return state


PAGES_DEPLOYS = f"repos/{REPO}/actions/workflows/pages.yml/runs?status=success&per_page=1"


def test_build_site_writes_every_part(offline_site: dict, tmp_path: Path) -> None:
    out = tmp_path / "_site"
    summary = mod.build_site(out, repo=REPO, work_dir=tmp_path / "work", now=NOW)

    files = sorted(path.relative_to(out).as_posix() for path in out.rglob("*") if path.is_file())
    assert files == [
        ".nojekyll",
        "api/index.html",
        "data/history.json",
        "downloads/index.html",
        "index.html",
        "schemas/factmetadata.schema.json",
        "schemas/index.html",
    ]
    # No deployed history: backfill as far as BACKFILL_RUNS per workflow.
    assert offline_site["per_workflow"] == [pages_history.BACKFILL_RUNS]
    assert offline_site["deploy_checks"] == [PAGES_DEPLOYS]
    assert offline_site["workflows"][-1] == "Code Analysis"
    history = json.loads((out / "data/history.json").read_text(encoding="utf-8"))
    assert history["version"] == 1
    assert [record["run_id"] for record in history["records"]] == [40021772122]
    assert not (tmp_path / "work").exists()
    assert summary.startswith("### Pages site\n")
    assert "| History | 1 runs of 1 commits; 1 new, 0 past the caps (backfilled) |" in summary
    assert "| Downloads | 0 of 6 checksums, 0 with their commit |" in summary
    assert "| API docs | link to https://api.qgroundcontrol.com |" in summary


def test_build_site_extends_the_deployed_history(offline_site: dict, tmp_path: Path) -> None:
    older = {
        "commit": "a" * 40,
        "workflow": "Windows",
        "run_id": 1,
        "created_at": "2026-10-20T00:00:00Z",
    }
    offline_site["history"] = {"version": 1, "repository": REPO, "records": [older]}
    mod.build_site(tmp_path / "_site", repo=REPO, work_dir=tmp_path / "work", now=NOW)
    assert offline_site["per_workflow"] == [pages_history.RECENT_RUNS]
    assert offline_site["deploy_checks"] == []
    history = json.loads((tmp_path / "_site/data/history.json").read_text(encoding="utf-8"))
    assert [record["run_id"] for record in history["records"]] == [1, 40021772122]


def test_collection_stops_collect_seconds_after_the_build_starts(
    offline_site: dict, tmp_path: Path
) -> None:
    before = time.monotonic()
    mod.build_site(tmp_path / "_site", repo=REPO, work_dir=tmp_path / "work", now=NOW)
    started = offline_site["deadline"] - pages_history.COLLECT_SECONDS
    assert before <= started <= time.monotonic()


OLDER = {
    "commit": "a" * 40,
    "workflow": "Windows",
    "run_id": 1,
    "created_at": "2026-10-20T00:00:00Z",
}


def test_a_partial_collection_keeps_the_deployed_history(
    offline_site: dict, tmp_path: Path
) -> None:
    offline_site["history"] = {"version": 1, "repository": REPO, "records": [OLDER]}
    offline_site["pending"] = 4
    summary = mod.build_site(tmp_path / "_site", repo=REPO, work_dir=tmp_path / "work", now=NOW)
    history = json.loads((tmp_path / "_site/data/history.json").read_text(encoding="utf-8"))
    assert [record["run_id"] for record in history["records"]] == [1, 40021772122]
    assert "| 2 runs of 2 commits; 1 new, 0 past the caps, 4 left for the next build |" in summary


def test_a_deadline_mid_collection_still_deploys_with_every_stored_record(
    offline_site: dict, tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    # The real collection: the deadline passes while the second newest new run downloads.
    monkeypatch.setattr(mod, "collect_new_records", pages_history.collect_new_records)

    def record(index: int) -> dict[str, Any]:
        return {
            "commit": f"{index:040x}",
            "workflow": "Linux",
            "run_id": index,
            "attempt": 1,
            "created_at": f"2026-10-1{index}T00:00:00Z",
            "conclusion": "success",
        }

    runs = [
        {"id": i, "name": "Linux", "head_sha": f"{i:040x}", "created_at": record(i)["created_at"]}
        for i in range(1, 6)
    ]
    offline_site["history"] = {"version": 1, "repository": REPO, "records": [record(1), record(2)]}
    monkeypatch.setattr(pages_history, "list_master_push_runs", lambda *a, **k: runs)

    def collect(repo: str, run: dict, work_dir: Path, *, deadline: float) -> dict:
        if run["id"] == 4:
            raise pages_history.CollectionDeadline("the collection deadline passed")
        return record(run["id"])

    monkeypatch.setattr(pages_history, "collect_record", collect)
    summary = mod.build_site(tmp_path / "_site", repo=REPO, work_dir=tmp_path / "work", now=NOW)
    history = json.loads((tmp_path / "_site/data/history.json").read_text(encoding="utf-8"))
    assert [record["run_id"] for record in history["records"]] == [1, 2, 5]
    assert "; 1 new, 0 past the caps, 2 left for the next build |" in summary


def test_a_404_after_a_deploy_fails_before_writing_anything(
    offline_site: dict, tmp_path: Path, gh_summary: Path, capsys: pytest.CaptureFixture[str]
) -> None:
    # A Pages outage or an unpublish answers 404 too; a new history would drop every record.
    offline_site["deploys"] = 412
    out = tmp_path / "_site"
    assert mod.main(["build", "--out", str(out), "--repo", REPO]) == 1
    assert not out.exists()
    assert offline_site["deploy_checks"] == [PAGES_DEPLOYS]
    assert offline_site["per_workflow"] == []
    assert "::error title=Pages site::the site has no history (HTTP 404) but 412" in (
        capsys.readouterr().out
    )
    assert gh_summary.read_text(encoding="utf-8") == ""


def test_a_failed_deploy_check_fails_the_build(
    offline_site: dict, tmp_path: Path, capsys: pytest.CaptureFixture[str]
) -> None:
    offline_site["deploys"] = subprocess.CalledProcessError(1, ["gh", "api"], stderr="HTTP 502")
    out = tmp_path / "_site"
    assert mod.main(["build", "--out", str(out), "--repo", REPO]) == 1
    assert not out.exists()
    assert offline_site["per_workflow"] == []
    assert "cannot tell whether the site was deployed before" in capsys.readouterr().out


@pytest.mark.parametrize(
    ("args", "env"), [(["--new-history"], None), ([], "true")], ids=["flag", "env"]
)
def test_new_history_starts_over_after_a_deploy(
    offline_site: dict,
    tmp_path: Path,
    monkeypatch: pytest.MonkeyPatch,
    capsys: pytest.CaptureFixture[str],
    args: list[str],
    env: str | None,
) -> None:
    if env is not None:
        monkeypatch.setenv("PAGES_NEW_HISTORY", env)
    offline_site["deploys"] = 412
    out = tmp_path / "_site"
    assert mod.main(["build", "--out", str(out), "--repo", REPO, *args]) == 0
    assert offline_site["deploy_checks"] == []
    assert offline_site["per_workflow"] == [pages_history.BACKFILL_RUNS]
    assert "::warning::The site has no history; starting a new one" in capsys.readouterr().out


def test_new_history_never_drops_a_deployed_history(offline_site: dict, tmp_path: Path) -> None:
    offline_site["history"] = {"version": 1, "repository": REPO, "records": [OLDER]}
    mod.build_site(
        tmp_path / "_site", repo=REPO, work_dir=tmp_path / "work", now=NOW, new_history=True
    )
    history = json.loads((tmp_path / "_site/data/history.json").read_text(encoding="utf-8"))
    assert [record["run_id"] for record in history["records"]] == [1, 40021772122]
    assert offline_site["per_workflow"] == [pages_history.RECENT_RUNS]


def test_new_history_is_off_unless_set(monkeypatch: pytest.MonkeyPatch) -> None:
    # pages.yml passes the boolean dispatch input, which is empty for every other event.
    for value, expected in [(None, False), ("", False), ("false", False), ("true", True)]:
        if value is None:
            monkeypatch.delenv("PAGES_NEW_HISTORY", raising=False)
        else:
            monkeypatch.setenv("PAGES_NEW_HISTORY", value)
        args = mod.parse_args(["build", "--out", "_site", "--repo", REPO])
        assert args.new_history is expected, value


def test_history_fetch_failure_fails_before_writing_anything(
    offline_site: dict, tmp_path: Path, gh_summary: Path, capsys: pytest.CaptureFixture[str]
) -> None:
    offline_site["history"] = pages_history.HistoryError("cannot fetch …: HTTP 503")
    out = tmp_path / "_site"
    assert mod.main(["build", "--out", str(out), "--repo", REPO]) == 1
    assert not out.exists()
    assert offline_site["per_workflow"] == []
    assert "::error title=Pages site::cannot fetch" in capsys.readouterr().out
    assert gh_summary.read_text(encoding="utf-8") == ""


def test_main_writes_the_job_summary(
    offline_site: dict, tmp_path: Path, gh_summary: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    monkeypatch.setenv("GITHUB_REPOSITORY", REPO)
    monkeypatch.setenv("RUNNER_TEMP", str(tmp_path / "runner"))
    assert mod.main(["build", "--out", str(tmp_path / "_site")]) == 0
    assert gh_summary.read_text(encoding="utf-8").startswith("### Pages site\n")
    assert not (tmp_path / "runner/pages-work").exists()


def test_build_site_refuses_a_non_empty_directory(offline_site: dict, tmp_path: Path) -> None:
    (tmp_path / "stale.html").write_text("")
    with pytest.raises(mod.SiteError, match="is not empty"):
        mod.build_site(tmp_path, repo=REPO, work_dir=tmp_path / "work", now=NOW)


def test_a_history_of_another_repository_is_refused(offline_site: dict, tmp_path: Path) -> None:
    offline_site["history"] = {"version": 1, "repository": "someone/fork", "records": []}
    with pytest.raises(mod.SiteError, match="someone/fork"):
        mod.build_site(tmp_path / "_site", repo=REPO, work_dir=tmp_path / "work", now=NOW)


def test_site_pages_link_only_to_existing_pages(offline_site: dict, tmp_path: Path) -> None:
    out = tmp_path / "_site"
    mod.build_site(out, repo=REPO, work_dir=tmp_path / "work", now=NOW)
    pages = [out / "index.html", out / "downloads/index.html", out / "schemas/index.html"]
    assert mod.broken_links(out, [*pages, out / "api/index.html"]) == []


def test_a_page_with_a_broken_link_fails_the_build(
    offline_site: dict, tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    monkeypatch.setattr(mod, "dashboard", lambda *a, **k: '<a href="missing/">x</a>')
    with pytest.raises(mod.SiteError, match=r"broken links: index\.html: missing/"):
        mod.build_site(tmp_path / "_site", repo=REPO, work_dir=tmp_path / "work", now=NOW)
