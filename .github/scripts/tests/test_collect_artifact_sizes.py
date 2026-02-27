"""Tests for collect_artifact_sizes.py."""

from __future__ import annotations

import json
from pathlib import Path

import collect_artifact_sizes as mod


def _run(
    name: str,
    *,
    run_id: int,
    created_at: str = "2026-02-24T00:00:00Z",
    status: str = "completed",
    conclusion: str = "success",
) -> dict[str, object]:
    return {
        "id": run_id,
        "name": name,
        "created_at": created_at,
        "status": status,
        "conclusion": conclusion,
    }


def test_latest_successful_runs_picks_latest_success_per_platform() -> None:
    platforms = ["Linux", "Windows"]
    runs = [
        _run("Linux", run_id=1, created_at="2026-02-24T00:00:00Z"),
        _run("Linux", run_id=2, created_at="2026-02-24T01:00:00Z"),
        _run("Windows", run_id=3, conclusion="failure"),
        _run("Windows", run_id=4, conclusion="success"),
        _run("Other", run_id=5),
    ]

    latest = mod.latest_successful_runs(runs, platforms)
    assert latest["Linux"]["id"] == 2
    assert latest["Windows"]["id"] == 4


def test_latest_successful_runs_handles_iso8601_offsets() -> None:
    platforms = ["Linux"]
    runs = [
        _run("Linux", run_id=1, created_at="2026-02-24T01:30:00+01:00"),
        _run("Linux", run_id=2, created_at="2026-02-24T01:00:00Z"),
    ]

    latest = mod.latest_successful_runs(runs, platforms)
    assert latest["Linux"]["id"] == 2


def test_collect_artifacts_filters_non_product_artifacts() -> None:
    latest = {"Linux": {"id": 11}}

    def fake_list_run_artifacts(repo: str, run_id: int) -> list[dict[str, object]]:
        assert repo == "owner/repo"
        assert run_id == 11
        return [
            {"name": "QGroundControl-x86_64", "size_in_bytes": 1024 * 1024},
            {"name": "test-results-linux", "size_in_bytes": 100},
            {"name": "test-duration-linux_gcc_64", "size_in_bytes": 100},
            {"name": "coverage-report", "size_in_bytes": 100},
            {"name": "size-metrics", "size_in_bytes": 100},
            {"name": "emulator-diagnostics-linux-emulator-1", "size_in_bytes": 100},
        ]

    original = mod.list_run_artifacts
    mod.list_run_artifacts = fake_list_run_artifacts  # type: ignore[assignment]
    try:
        artifacts = mod.collect_artifacts("owner/repo", latest, ["Linux"])
    finally:
        mod.list_run_artifacts = original  # type: ignore[assignment]

    assert artifacts == [
        {
            "name": "QGroundControl-x86_64",
            "size_bytes": 1024 * 1024,
            "size_human": "1.00 MB",
        },
    ]


def test_collect_artifacts_keeps_real_extension() -> None:
    latest = {"Android": {"id": 21}}

    def fake_list_run_artifacts(repo: str, run_id: int) -> list[dict[str, object]]:
        assert repo == "owner/repo"
        assert run_id == 21
        return [{"name": "QGroundControl.apk", "size_in_bytes": 2048}]

    original = mod.list_run_artifacts
    mod.list_run_artifacts = fake_list_run_artifacts  # type: ignore[assignment]
    try:
        artifacts = mod.collect_artifacts("owner/repo", latest, ["Android"])
    finally:
        mod.list_run_artifacts = original  # type: ignore[assignment]

    assert artifacts == [
        {
            "name": "QGroundControl.apk",
            "size_bytes": 2048,
            "size_human": "0.00 MB",
        },
    ]


def test_collect_artifacts_excludes_unknown_non_product_names() -> None:
    latest = {"Linux": {"id": 31}}

    def fake_list_run_artifacts(repo: str, run_id: int) -> list[dict[str, object]]:
        assert repo == "owner/repo"
        assert run_id == 31
        return [
            {"name": "random-tool-output", "size_in_bytes": 4096},
            {"name": "QGroundControl-aarch64", "size_in_bytes": 8192},
        ]

    original = mod.list_run_artifacts
    mod.list_run_artifacts = fake_list_run_artifacts  # type: ignore[assignment]
    try:
        artifacts = mod.collect_artifacts("owner/repo", latest, ["Linux"])
    finally:
        mod.list_run_artifacts = original  # type: ignore[assignment]

    assert artifacts == [
        {
            "name": "QGroundControl-aarch64",
            "size_bytes": 8192,
            "size_human": "0.01 MB",
        }
    ]


def test_collect_artifacts_uses_prefetched_artifact_metadata() -> None:
    latest = {"Linux": {"id": 41}}
    prefetched = {
        41: [
            {"name": "QGroundControl-x86_64", "size_in_bytes": 1234},
            {"name": "test-results-linux_gcc_64", "size_in_bytes": 100},
        ],
    }

    def fail_list_run_artifacts(repo: str, run_id: int) -> list[dict[str, object]]:
        raise AssertionError("list_run_artifacts should not be called when prefetched metadata is provided")

    original = mod.list_run_artifacts
    mod.list_run_artifacts = fail_list_run_artifacts  # type: ignore[assignment]
    try:
        artifacts = mod.collect_artifacts(
            "owner/repo",
            latest,
            ["Linux"],
            artifacts_by_run_id=prefetched,
        )
    finally:
        mod.list_run_artifacts = original  # type: ignore[assignment]

    assert artifacts == [
        {
            "name": "QGroundControl-x86_64",
            "size_bytes": 1234,
            "size_human": "0.00 MB",
        },
    ]


def test_collect_artifacts_continues_when_one_api_call_fails() -> None:
    latest = {
        "Linux": {"id": 51},
        "Windows": {"id": 52},
    }

    def flaky_list_run_artifacts(repo: str, run_id: int) -> list[dict[str, object]]:
        if run_id == 51:
            raise RuntimeError("transient API error")
        return [{"name": "QGroundControl-installer-AMD64", "size_in_bytes": 2048}]

    original = mod.list_run_artifacts
    mod.list_run_artifacts = flaky_list_run_artifacts  # type: ignore[assignment]
    try:
        artifacts = mod.collect_artifacts("owner/repo", latest, ["Linux", "Windows"])
    finally:
        mod.list_run_artifacts = original  # type: ignore[assignment]

    assert artifacts == [
        {
            "name": "QGroundControl-installer-AMD64",
            "size_bytes": 2048,
            "size_human": "0.00 MB",
        },
    ]


def test_main_writes_output_json(tmp_path: Path, monkeypatch) -> None:
    output_file = tmp_path / "sizes.json"
    runs = [
        _run("Linux", run_id=101, created_at="2026-02-24T02:00:00Z"),
        _run("Windows", run_id=102, created_at="2026-02-24T02:00:00Z"),
    ]

    def fake_list_workflow_runs(repo: str, head_sha: str) -> list[dict[str, object]]:
        assert repo == "owner/repo"
        assert head_sha == "abc123"
        return runs

    def fake_list_run_artifacts(repo: str, run_id: int) -> list[dict[str, object]]:
        if run_id == 101:
            return [{"name": "QGroundControl-x86_64", "size_in_bytes": 2 * 1024 * 1024}]
        return [{"name": "QGroundControl-installer-AMD64", "size_in_bytes": 2 * 1024 * 1024}]

    monkeypatch.setattr(mod, "list_workflow_runs", fake_list_workflow_runs)
    monkeypatch.setattr(mod, "list_run_artifacts", fake_list_run_artifacts)

    rc = mod.main(
        [
            "--repo",
            "owner/repo",
            "--head-sha",
            "abc123",
            "--platform-workflows",
            "Linux,Windows",
            "--output-file",
            str(output_file),
        ]
    )
    assert rc == 0
    data = json.loads(output_file.read_text(encoding="utf-8"))
    assert len(data["artifacts"]) == 2
    assert data["artifacts"][0]["name"] == "QGroundControl-installer-AMD64"
    assert data["artifacts"][1]["name"] == "QGroundControl-x86_64"


def test_main_reads_artifacts_file_when_provided(tmp_path: Path, monkeypatch) -> None:
    output_file = tmp_path / "sizes.json"
    artifacts_file = tmp_path / "artifacts.json"
    artifacts_file.write_text(
        json.dumps(
            {
                "runs": {
                    "201": [
                        {"name": "QGroundControl-x86_64", "size_in_bytes": 1024 * 1024},
                        {"name": "test-results-linux_gcc_64", "size_in_bytes": 100},
                    ],
                },
            }
        ),
        encoding="utf-8",
    )
    runs = [_run("Linux", run_id=201, created_at="2026-02-24T03:00:00Z")]

    def fake_list_workflow_runs(repo: str, head_sha: str) -> list[dict[str, object]]:
        assert repo == "owner/repo"
        assert head_sha == "abc123"
        return runs

    def fail_list_run_artifacts(repo: str, run_id: int) -> list[dict[str, object]]:
        raise AssertionError("list_run_artifacts should not be called when artifacts file is provided")

    monkeypatch.setattr(mod, "list_workflow_runs", fake_list_workflow_runs)
    monkeypatch.setattr(mod, "list_run_artifacts", fail_list_run_artifacts)

    rc = mod.main(
        [
            "--repo",
            "owner/repo",
            "--head-sha",
            "abc123",
            "--platform-workflows",
            "Linux",
            "--output-file",
            str(output_file),
            "--artifacts-file",
            str(artifacts_file),
        ]
    )
    assert rc == 0
    data = json.loads(output_file.read_text(encoding="utf-8"))
    assert len(data["artifacts"]) == 1
    assert data["artifacts"][0]["name"] == "QGroundControl-x86_64"


def test_main_returns_one_on_invalid_runs_file(tmp_path: Path) -> None:
    runs_file = tmp_path / "runs.json"
    runs_file.write_text("{not-json", encoding="utf-8")
    output_file = tmp_path / "sizes.json"

    rc = mod.main(
        [
            "--repo",
            "owner/repo",
            "--head-sha",
            "abc123",
            "--output-file",
            str(output_file),
            "--runs-file",
            str(runs_file),
        ]
    )
    assert rc == 1


def test_main_returns_one_on_non_list_runs_file(tmp_path: Path) -> None:
    runs_file = tmp_path / "runs.json"
    runs_file.write_text(json.dumps({"runs": []}), encoding="utf-8")
    output_file = tmp_path / "sizes.json"

    rc = mod.main(
        [
            "--repo",
            "owner/repo",
            "--head-sha",
            "abc123",
            "--output-file",
            str(output_file),
            "--runs-file",
            str(runs_file),
        ]
    )
    assert rc == 1
