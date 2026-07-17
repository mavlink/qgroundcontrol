"""Contract tests for artifact-size collection."""

from __future__ import annotations

import json
from typing import TYPE_CHECKING

import collect_artifact_sizes as mod
import pytest
from _helpers import workflow_run

if TYPE_CHECKING:
    from pathlib import Path


def _main_args(output: Path, *extra: str, platforms: str = "Linux,Windows") -> list[str]:
    return [
        "--repo",
        "owner/repo",
        "--head-sha",
        "abc123",
        "--platform-workflows",
        platforms,
        "--output-file",
        str(output),
        *extra,
    ]


def test_collect_artifacts_filters_products_and_deduplicates_largest() -> None:
    latest = {"MacOS": {"id": 11}, "Android": {"id": 12}}
    artifacts = {
        11: [
            {"name": "QGroundControl", "size_in_bytes": 300 * 1024 * 1024},
            {"name": "test-results-macos", "size_in_bytes": 100},
            {"name": "random-tool-output", "size_in_bytes": 100},
        ],
        12: [
            {"name": "QGroundControl", "size_in_bytes": 200 * 1024 * 1024},
            {"name": "QGroundControl.apk", "size_in_bytes": 2 * 1024 * 1024},
        ],
    }

    result = mod.collect_artifacts(
        "owner/repo", latest, ["MacOS", "Android"], artifacts_by_run_id=artifacts
    )

    assert [(item["name"], item["size_bytes"]) for item in result] == [
        ("QGroundControl", 300 * 1024 * 1024),
        ("QGroundControl.apk", 2 * 1024 * 1024),
    ]


def test_collect_artifacts_continues_after_api_failure(monkeypatch: pytest.MonkeyPatch) -> None:
    def list_artifacts(repo: str, run_id: int) -> list[dict[str, object]]:
        assert repo == "owner/repo"
        if run_id == 21:
            raise RuntimeError("transient API error")
        return [{"name": "QGroundControl-installer-AMD64", "size_in_bytes": 2048}]

    monkeypatch.setattr(mod, "list_run_artifacts", list_artifacts)

    result = mod.collect_artifacts(
        "owner/repo", {"Linux": {"id": 21}, "Windows": {"id": 22}}, ["Linux", "Windows"]
    )
    assert [item["name"] for item in result] == ["QGroundControl-installer-AMD64"]


def test_main_collects_and_writes_sorted_output(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    output = tmp_path / "sizes.json"
    monkeypatch.setattr(
        mod,
        "list_workflow_runs_for_sha",
        lambda repo, sha: [workflow_run("Linux", 31), workflow_run("Windows", 32)],
    )
    monkeypatch.setattr(
        mod,
        "list_run_artifacts",
        lambda repo, run_id: [
            {
                "name": "QGroundControl-x86_64"
                if run_id == 31
                else "QGroundControl-installer-AMD64",
                "size_in_bytes": 2 * 1024 * 1024,
            }
        ],
    )

    assert mod.main(_main_args(output)) == 0
    data = json.loads(output.read_text())
    assert [item["name"] for item in data["artifacts"]] == [
        "QGroundControl-installer-AMD64",
        "QGroundControl-x86_64",
    ]


def test_main_uses_prefetched_artifacts(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    output = tmp_path / "sizes.json"
    artifacts_file = tmp_path / "artifacts.json"
    artifacts_file.write_text(
        json.dumps(
            {
                "runs": {
                    "41": [
                        {"name": "QGroundControl-x86_64", "size_in_bytes": 1024 * 1024},
                        {"name": "test-results-linux", "size_in_bytes": 100},
                    ]
                }
            }
        )
    )
    monkeypatch.setattr(
        mod, "list_workflow_runs_for_sha", lambda repo, sha: [workflow_run("Linux", 41)]
    )
    monkeypatch.setattr(
        mod,
        "list_run_artifacts",
        lambda repo, run_id: pytest.fail("prefetched artifacts should avoid the API"),
    )

    assert (
        mod.main(
            _main_args(
                output,
                "--artifacts-file",
                str(artifacts_file),
                platforms="Linux",
            )
        )
        == 0
    )
    assert [item["name"] for item in json.loads(output.read_text())["artifacts"]] == [
        "QGroundControl-x86_64"
    ]


def test_main_falls_back_to_api_for_invalid_prefetched_artifacts(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    output = tmp_path / "sizes.json"
    artifacts_file = tmp_path / "artifacts.json"
    artifacts_file.write_text(json.dumps({"runs": {"invalid": []}}))
    monkeypatch.setattr(
        mod, "list_workflow_runs_for_sha", lambda repo, sha: [workflow_run("Linux", 41)]
    )
    monkeypatch.setattr(
        mod,
        "list_run_artifacts",
        lambda repo, run_id: [{"name": "QGroundControl-x86_64", "size_in_bytes": 1024}],
    )

    assert (
        mod.main(
            _main_args(
                output,
                "--artifacts-file",
                str(artifacts_file),
                platforms="Linux",
            )
        )
        == 0
    )
    assert json.loads(output.read_text())["artifacts"][0]["name"] == "QGroundControl-x86_64"


def test_main_rejects_invalid_runs_file(tmp_path: Path) -> None:
    runs_file = tmp_path / "runs.json"
    for payload in ("{not-json", json.dumps({"runs": []})):
        runs_file.write_text(payload)
        assert mod.main(_main_args(tmp_path / "sizes.json", "--runs-file", str(runs_file))) == 1
