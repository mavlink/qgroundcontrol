"""Tests for download_artifacts.py."""

from __future__ import annotations

import json
import subprocess
from pathlib import Path
from unittest.mock import patch

import pytest
from ci.reporting import download_artifacts as mod

from .._helpers import completed


def test_parse_args_defaults() -> None:
    args = mod.parse_args(
        ["--repo", "owner/repo", "--head-sha", "abc123", "--artifact-prefixes", "coverage-report"]
    )
    assert args.repo == "owner/repo"
    assert args.head_sha == "abc123"
    assert args.output_dir == Path("artifacts")
    assert args.workflows == "Linux,Windows,MacOS,Android,iOS"
    assert args.event == ""
    assert args.artifact_prefixes == "coverage-report"
    assert args.sizes_out is None
    assert args.require_sizes is False
    assert args.allow_missing is False


def test_parse_args_reads_optional_action_inputs(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    required = ["--repo", "owner/repo", "--head-sha", "abc123", "--artifact-prefixes", "x"]
    runs = tmp_path / "runs.json"
    runs.write_text("[]", encoding="utf-8")
    inputs = {
        "INPUT_RUNS_FILE": str(runs),
        "INPUT_INCLUDE_FAILED": "true",
        "INPUT_ALLOW_MISSING": "true",
        "INPUT_SIZES_FILE": "sizes.json",
        "INPUT_REQUIRE_SIZES": "true",
    }
    for name, value in inputs.items():
        monkeypatch.setenv(name, value)

    args = mod.parse_args(required)
    assert args.runs_file == str(runs)
    assert args.include_failed and args.allow_missing and args.require_sizes
    assert args.sizes_out == Path("sizes.json")

    # The action passes a cache path that an earlier step may not have written.
    monkeypatch.setenv("INPUT_RUNS_FILE", str(tmp_path / "missing.json"))
    monkeypatch.setenv("INPUT_SIZES_FILE", "")
    for flag in ("INCLUDE_FAILED", "ALLOW_MISSING", "REQUIRE_SIZES"):
        monkeypatch.setenv(f"INPUT_{flag}", "false")
    args = mod.parse_args(required)
    assert args.runs_file == ""
    assert not (args.include_failed or args.allow_missing or args.require_sizes)
    assert args.sizes_out is None


def test_artifact_prefixes_are_required() -> None:
    with pytest.raises(SystemExit):
        mod.parse_args(["--repo", "owner/repo", "--head-sha", "abc123"])
    with patch.object(mod, "list_workflow_runs_for_sha") as query:
        rc = mod.main(
            ["--repo", "owner/repo", "--head-sha", "abc123", "--artifact-prefixes", " , "]
        )
    assert rc == 1
    query.assert_not_called()


def test_latest_successful_runs_filter_by_name_status_and_conclusion() -> None:
    all_runs = [
        {"id": 1, "name": "Linux", "status": "completed", "conclusion": "success"},
        {"id": 2, "name": "Linux", "status": "in_progress", "conclusion": None},
        {"id": 3, "name": "Windows", "status": "completed", "conclusion": "failure"},
        {"id": 4, "name": "Other", "status": "completed", "conclusion": "success"},
    ]

    runs = mod.select_latest_successful_runs(all_runs, ["Linux", "Windows"])

    assert len(runs) == 1
    assert runs[0]["id"] == 1
    assert runs[0]["name"] == "Linux"


def test_latest_successful_runs_keep_latest_run_per_workflow() -> None:
    all_runs = [
        {
            "id": 10,
            "name": "Linux",
            "status": "completed",
            "conclusion": "success",
            "created_at": "2026-01-01T00:00:00Z",
        },
        {
            "id": 11,
            "name": "Linux",
            "status": "completed",
            "conclusion": "success",
            "created_at": "2026-01-02T00:00:00Z",
        },
        {
            "id": 20,
            "name": "Windows",
            "status": "completed",
            "conclusion": "success",
            "created_at": "2026-01-01T00:00:00Z",
        },
    ]

    runs = mod.select_latest_successful_runs(all_runs, ["Linux", "Windows"])

    by_name = {run["name"]: run["id"] for run in runs}
    assert by_name["Linux"] == 11
    assert by_name["Windows"] == 20


def test_latest_successful_runs_filter_by_event() -> None:
    all_runs = [
        {
            "id": 10,
            "name": "Linux",
            "status": "completed",
            "conclusion": "success",
            "event": "push",
        },
        {
            "id": 11,
            "name": "Linux",
            "status": "completed",
            "conclusion": "success",
            "event": "pull_request",
        },
    ]

    runs = mod.select_latest_successful_runs(all_runs, ["Linux"], event="pull_request")

    assert [run["id"] for run in runs] == [11]


def test_download_run_artifacts_failure_returns_false() -> None:
    with patch.object(mod, "gh", return_value=completed(stderr="bad", returncode=1)):
        ok = mod.download_run_artifacts(123, "owner/repo", Path("artifacts"), ["coverage-report"])
    assert ok is False


def test_download_run_artifacts_bounds_the_download() -> None:
    with patch.object(mod, "gh", return_value=completed()) as mock_gh:
        assert mod.download_run_artifacts(123, "owner/repo", Path("artifacts"), ["a"]) is True
    assert mock_gh.call_args.kwargs["timeout"] == mod.DOWNLOAD_TIMEOUT_SECONDS


def test_download_run_artifacts_timeout_returns_false() -> None:
    timeout = subprocess.TimeoutExpired(["gh", "run", "download"], mod.DOWNLOAD_TIMEOUT_SECONDS)
    with patch.object(mod, "gh", side_effect=timeout):
        assert mod.download_run_artifacts(123, "owner/repo", Path("artifacts"), ["a"]) is False


def test_download_run_artifacts_passes_selected_names() -> None:
    with patch.object(mod, "gh", return_value=completed()) as mock_gh:
        ok = mod.download_run_artifacts(
            123,
            "owner/repo",
            Path("artifacts"),
            ["coverage-report", "test-results-linux_gcc_64"],
        )
    assert ok is True
    args = mock_gh.call_args.args
    assert args[:3] == ("run", "download", "123")
    assert "-n" in args
    assert "coverage-report" in args
    assert "test-results-linux_gcc_64" in args


def test_download_run_artifacts_isolates_a_single_artifact() -> None:
    with patch.object(mod, "gh", return_value=completed()) as mock_gh:
        assert mod.download_run_artifacts(1, "o/r", Path("artifacts"), ["coverage-report"])
        assert mod.download_run_artifacts(1, "o/r", Path("artifacts"), ["a", "b"])
    single, several = (call.args for call in mock_gh.call_args_list)
    assert single[single.index("--dir") + 1] == str(Path("artifacts") / "coverage-report")
    assert several[several.index("--dir") + 1] == "artifacts"


def test_manifest_maps_downloads_to_workflows_and_keeps_earlier_entries(tmp_path: Path) -> None:
    runs = [{"id": 7, "name": "Linux", "status": "completed", "conclusion": "failure"}]
    artifacts = [{"name": "test-results-linux-unit"}, {"name": "coverage-report"}]
    (tmp_path / "pre-commit-results").mkdir()
    mod.write_manifest(tmp_path, [{"name": "older", "workflow": "MacOS", "run_id": 3}])

    def download(run_id: int, repo: str, output_dir: Path, names: list[str]) -> bool:
        for name in names:
            (output_dir / name).mkdir(parents=True)
            (output_dir / name / "file.txt").write_text("x")
        return True

    with (
        patch.object(mod, "list_workflow_runs_for_sha", return_value=runs),
        patch.object(mod, "list_run_artifacts", return_value=artifacts),
        patch.object(mod, "download_run_artifacts", side_effect=download),
    ):
        rc = mod.main(
            [
                "--repo",
                "o/r",
                "--head-sha",
                "abc",
                "--workflows",
                "Linux",
                "--output-dir",
                str(tmp_path),
                "--artifact-prefixes",
                "coverage-report,test-results-",
                "--include-failed",
            ]
        )
    assert rc == 0
    assert json.loads((tmp_path / mod.MANIFEST_NAME).read_text()) == [
        {"name": "coverage-report", "workflow": "Linux", "run_id": 7},
        {"name": "older", "workflow": "MacOS", "run_id": 3},
        {"name": "test-results-linux-unit", "workflow": "Linux", "run_id": 7},
    ]
    assert mod.MANIFEST_NAME not in {path.name for path in mod.list_downloaded_files(tmp_path)}


def test_select_artifact_names_filters_by_prefix() -> None:
    artifacts = [
        {"name": "coverage-report"},
        {"name": "test-results-linux_gcc_64"},
        {"name": "emulator-diagnostics-123"},
        {"name": "QGroundControl-x86_64.AppImage"},
    ]
    names = mod.select_artifact_names(artifacts, ["coverage-report", "test-results-"])
    assert names == ["coverage-report", "test-results-linux_gcc_64"]


def test_list_downloaded_files_lists_nested_files(tmp_path: Path) -> None:
    (tmp_path / "b.txt").write_bytes(b"x")
    nested = tmp_path / "nested"
    nested.mkdir()
    (nested / "a.xml").write_bytes(b"x")

    assert mod.list_downloaded_files(tmp_path) == [tmp_path / "b.txt", nested / "a.xml"]


def test_main_returns_zero_when_no_runs_found() -> None:
    with (
        patch.object(mod, "list_workflow_runs_for_sha", return_value=[]),
        patch.object(
            mod,
            "select_latest_successful_runs",
            return_value=[],
        ),
    ):
        rc = mod.main(
            [
                "--repo",
                "owner/repo",
                "--head-sha",
                "abc123",
                "--artifact-prefixes",
                "coverage-report",
            ]
        )
    assert rc == 0


def test_main_returns_two_when_no_artifacts_match_prefixes(tmp_path: Path) -> None:
    runs = [{"id": 42, "name": "Linux", "status": "completed", "conclusion": "success"}]
    with (
        patch.object(mod, "list_workflow_runs_for_sha", return_value=runs),
        patch.object(
            mod,
            "select_latest_successful_runs",
            return_value=runs,
        ),
        patch.object(
            mod,
            "list_run_artifacts",
            return_value=[{"name": "unrelated-artifact", "size_in_bytes": 1}],
        ),
        patch.object(
            mod,
            "download_run_artifacts",
            return_value=True,
        ),
        patch.object(
            mod,
            "list_downloaded_files",
            return_value=[],
        ),
    ):
        rc = mod.main(
            [
                "--repo",
                "owner/repo",
                "--head-sha",
                "abc123",
                "--output-dir",
                str(tmp_path),
                "--workflows",
                "Linux",
                "--artifact-prefixes",
                "coverage-report",
            ],
        )
    assert rc == 2


def test_main_returns_one_on_invalid_runs_file(tmp_path: Path) -> None:
    runs_file = tmp_path / "runs.json"
    runs_file.write_text("{not-json", encoding="utf-8")

    rc = mod.main(
        [
            "--repo",
            "owner/repo",
            "--head-sha",
            "abc123",
            "--artifact-prefixes",
            "coverage-report",
            "--runs-file",
            str(runs_file),
        ],
    )
    assert rc == 1


@pytest.mark.parametrize("conclusion", ["success", "failure", "cancelled"])
def test_optional_diagnostics_record_sizes_without_downloads(
    tmp_path: Path, conclusion: str, capsys: pytest.CaptureFixture[str]
) -> None:
    runs = [{"id": 42, "name": "Linux", "status": "completed", "conclusion": conclusion}]
    artifacts = [{"name": "QGroundControl.AppImage", "size_in_bytes": 100}]
    sizes_path = tmp_path / "sizes.json"
    with (
        patch.object(mod, "list_workflow_runs_for_sha", return_value=runs),
        patch.object(mod, "list_run_artifacts", return_value=artifacts) as listing,
        patch.object(mod, "download_run_artifacts") as download,
    ):
        rc = mod.main(
            [
                "--repo",
                "owner/repo",
                "--head-sha",
                "abc123",
                "--workflows",
                "Linux",
                "--artifact-prefixes",
                "coverage-report,test-results-",
                "--sizes-out",
                str(sizes_path),
                "--include-failed",
                "--allow-missing",
            ]
        )
    assert rc == 0
    download.assert_not_called()
    listing.assert_called_once_with("owner/repo", 42)
    expected = (
        [
            {
                "name": "QGroundControl.AppImage",
                "size_bytes": 100,
                "size_human": "0.00 MB",
                "workflow": "Linux",
            }
        ]
        if conclusion == "success"
        else []
    )
    assert json.loads(sizes_path.read_text()) == {"head_sha": "abc123", "artifacts": expected}
    assert "continuing with build status only" in capsys.readouterr().out


def test_optional_diagnostics_write_empty_sizes_without_runs(tmp_path: Path) -> None:
    sizes_path = tmp_path / "sizes.json"
    with patch.object(mod, "list_workflow_runs_for_sha", return_value=[]):
        rc = mod.main(
            [
                "--repo",
                "owner/repo",
                "--head-sha",
                "abc123",
                "--artifact-prefixes",
                "coverage-report,test-results-",
                "--sizes-out",
                str(sizes_path),
                "--allow-missing",
            ]
        )
    assert rc == 0
    assert json.loads(sizes_path.read_text()) == {"head_sha": "abc123", "artifacts": []}


@pytest.mark.parametrize("allow_missing", [False, True])
@pytest.mark.parametrize("partial_download", [False, True])
def test_download_failure_is_not_optional(
    tmp_path: Path, allow_missing: bool, partial_download: bool
) -> None:
    runs = [{"id": 42, "name": "Linux", "status": "completed", "conclusion": "success"}]
    if partial_download:
        (tmp_path / "coverage.xml").write_text("<coverage/>")
    with (
        patch.object(mod, "list_workflow_runs_for_sha", return_value=runs),
        patch.object(mod, "list_run_artifacts", return_value=[{"name": "coverage-report"}]),
        patch.object(mod, "download_run_artifacts", return_value=False),
    ):
        rc = mod.main(
            [
                "--repo",
                "owner/repo",
                "--head-sha",
                "abc123",
                "--workflows",
                "Linux",
                "--output-dir",
                str(tmp_path),
                "--artifact-prefixes",
                "coverage-report",
                *(["--allow-missing"] if allow_missing else []),
            ]
        )
    assert rc == 1


def test_optional_diagnostics_reject_advertised_empty_download(tmp_path: Path) -> None:
    runs = [{"id": 42, "name": "Linux", "status": "completed", "conclusion": "success"}]
    with (
        patch.object(mod, "list_workflow_runs_for_sha", return_value=runs),
        patch.object(mod, "list_run_artifacts", return_value=[{"name": "coverage-report"}]),
        patch.object(mod, "download_run_artifacts", return_value=True),
    ):
        rc = mod.main(
            [
                "--repo",
                "owner/repo",
                "--head-sha",
                "abc123",
                "--workflows",
                "Linux",
                "--output-dir",
                str(tmp_path),
                "--artifact-prefixes",
                "coverage-report",
                "--allow-missing",
            ]
        )
    assert rc == 2


def test_optional_diagnostics_propagate_artifact_query_failure() -> None:
    runs = [{"id": 42, "name": "Linux", "status": "completed", "conclusion": "success"}]
    with (
        patch.object(mod, "list_workflow_runs_for_sha", return_value=runs),
        patch.object(mod, "list_run_artifacts", side_effect=RuntimeError("API failure")),
        pytest.raises(RuntimeError, match="API failure"),
    ):
        mod.main(
            [
                "--repo",
                "owner/repo",
                "--head-sha",
                "abc123",
                "--workflows",
                "Linux",
                "--artifact-prefixes",
                "coverage-report",
                "--allow-missing",
            ]
        )


def test_main_returns_one_on_non_list_runs_file(tmp_path: Path) -> None:
    runs_file = tmp_path / "runs.json"
    runs_file.write_text(json.dumps({"runs": []}), encoding="utf-8")

    rc = mod.main(
        [
            "--repo",
            "owner/repo",
            "--head-sha",
            "abc123",
            "--artifact-prefixes",
            "coverage-report",
            "--runs-file",
            str(runs_file),
        ],
    )
    assert rc == 1


def test_main_falls_back_to_older_successful_run_with_matching_artifacts(tmp_path: Path) -> None:
    runs = [
        {
            "id": 101,
            "name": "Linux",
            "status": "completed",
            "conclusion": "success",
            "created_at": "2026-02-25T10:00:00Z",
        },
        {
            "id": 100,
            "name": "Linux",
            "status": "completed",
            "conclusion": "success",
            "created_at": "2026-02-25T09:00:00Z",
        },
    ]

    def _artifacts_for_run(repo: str, run_id: int) -> list[dict[str, object]]:
        if run_id == 101:
            return [{"name": "unrelated-artifact", "size_in_bytes": 1}]
        if run_id == 100:
            return [{"name": "coverage-report", "size_in_bytes": 42}]
        return []

    downloaded = tmp_path / "coverage.xml"
    downloaded.write_text("<xml/>", encoding="utf-8")

    with (
        patch.object(mod, "list_workflow_runs_for_sha", return_value=runs),
        patch.object(
            mod,
            "list_run_artifacts",
            side_effect=_artifacts_for_run,
        ),
        patch.object(
            mod,
            "download_run_artifacts",
            return_value=True,
        ) as download_mock,
        patch.object(
            mod,
            "list_downloaded_files",
            return_value=[downloaded],
        ),
    ):
        rc = mod.main(
            [
                "--repo",
                "owner/repo",
                "--head-sha",
                "abc123",
                "--output-dir",
                str(tmp_path),
                "--workflows",
                "Linux",
                "--artifact-prefixes",
                "coverage-report",
            ],
        )

    assert rc == 0
    assert download_mock.call_args.args[0] == 100


def test_main_writes_sizes_file(tmp_path: Path) -> None:
    runs = [{"id": 42, "name": "Linux", "status": "completed", "conclusion": "success"}]
    artifacts = [
        {"name": "coverage-report", "size_in_bytes": 100},
        {"name": "QGroundControl-x86_64.AppImage", "size_in_bytes": 200},
    ]
    sizes_path = tmp_path / "sizes.json"
    downloaded = tmp_path / "dummy.txt"
    downloaded.write_text("x", encoding="utf-8")

    with (
        patch.object(mod, "list_workflow_runs_for_sha", return_value=runs),
        patch.object(
            mod,
            "select_latest_successful_runs",
            return_value=runs,
        ),
        patch.object(
            mod,
            "list_run_artifacts",
            return_value=artifacts,
        ),
        patch.object(
            mod,
            "download_run_artifacts",
            return_value=True,
        ),
        patch.object(
            mod,
            "list_downloaded_files",
            return_value=[downloaded],
        ),
    ):
        rc = mod.main(
            [
                "--repo",
                "owner/repo",
                "--head-sha",
                "abc123",
                "--output-dir",
                str(tmp_path),
                "--workflows",
                "Linux",
                "--artifact-prefixes",
                "coverage-report",
                "--sizes-out",
                str(sizes_path),
            ],
        )

    assert rc == 0
    assert json.loads(sizes_path.read_text()) == {
        "head_sha": "abc123",
        "artifacts": [
            {
                "name": "QGroundControl-x86_64.AppImage",
                "size_bytes": 200,
                "size_human": "0.00 MB",
                "workflow": "Linux",
            }
        ],
    }


def _run(
    name: str,
    *,
    run_id: int,
    created_at: str = "2026-02-24T00:00:00Z",
    conclusion: str = "success",
    event: str = "pull_request",
) -> dict[str, object]:
    return {
        "id": run_id,
        "name": name,
        "created_at": created_at,
        "status": "completed",
        "conclusion": conclusion,
        "event": event,
    }


def _size(name: str, size_bytes: int, size_human: str, workflow: str = "") -> dict[str, object]:
    return {"name": name, "size_bytes": size_bytes, "size_human": size_human, "workflow": workflow}


def test_sizes_use_latest_successful_run_per_workflow_and_event() -> None:
    runs = [
        _run("Linux", run_id=1),
        _run("Linux", run_id=2, created_at="2026-02-24T01:00:00Z"),
        _run("Linux", run_id=3, created_at="2026-02-24T02:00:00Z", event="push"),
        _run("Windows", run_id=4, created_at="2026-02-24T01:30:00+01:00"),
        _run("Windows", run_id=5, created_at="2026-02-24T00:40:00Z", conclusion="failure"),
        _run("Windows", run_id=6, created_at="2026-02-24T00:45:00Z"),
        _run("Other", run_id=7),
    ]
    listed = {
        run_id: [{"name": f"QGroundControl-{run_id}", "size_in_bytes": run_id}]
        for run_id in range(1, 8)
    }
    sizes = mod.collect_artifact_sizes(
        "owner/repo", runs, ["Linux", "Windows"], event="pull_request", listed=listed
    )
    assert [size["name"] for size in sizes] == ["QGroundControl-2", "QGroundControl-6"]


def test_sizes_keep_only_distributable_artifacts() -> None:
    listed = {
        11: [
            {"name": "QGroundControl-x86_64", "size_in_bytes": 1024 * 1024},
            {"name": "QGroundControl.apk", "size_in_bytes": 2048},
            {"name": "test-results-linux", "size_in_bytes": 100},
            {"name": "coverage-report", "size_in_bytes": 100},
            {"name": "emulator-diagnostics-linux-emulator-1", "size_in_bytes": 100},
            {"name": "random-tool-output", "size_in_bytes": 4096},
        ]
    }
    sizes = mod.collect_artifact_sizes(
        "owner/repo", [_run("Linux", run_id=11)], ["Linux"], listed=listed
    )
    assert sizes == [
        _size("QGroundControl-x86_64", 1024 * 1024, "1.00 MB", "Linux"),
        _size("QGroundControl.apk", 2048, "0.00 MB", "Linux"),
    ]


def test_sizes_list_unlisted_runs_and_tolerate_api_failures() -> None:
    def list_run_artifacts(repo: str, run_id: int) -> list[dict[str, object]]:
        assert repo == "owner/repo"
        if run_id == 51:
            raise RuntimeError("transient API error")
        return [{"name": "QGroundControl-installer-AMD64", "size_in_bytes": 2048}]

    runs = [_run("Linux", run_id=51), _run("Windows", run_id=52), _run("MacOS", run_id=53)]
    listed = {53: [{"name": "QGroundControl.dmg", "size_in_bytes": 4096}]}
    with patch.object(mod, "list_run_artifacts", side_effect=list_run_artifacts) as listing:
        sizes = mod.collect_artifact_sizes(
            "owner/repo", runs, ["Linux", "Windows", "MacOS"], listed=listed
        )
    assert sorted(call.args[1] for call in listing.call_args_list) == [51, 52]
    assert sizes == [
        _size("QGroundControl-installer-AMD64", 2048, "0.00 MB", "Windows"),
        _size("QGroundControl.dmg", 4096, "0.00 MB", "MacOS"),
    ]


def test_sizes_keep_the_largest_same_named_artifact() -> None:
    listed = {
        61: [{"name": "QGroundControl", "size_in_bytes": 337 * 1024 * 1024}],
        62: [{"name": "QGroundControl", "size_in_bytes": 247 * 1024 * 1024}],
    }
    runs = [_run("MacOS", run_id=61), _run("Android", run_id=62)]
    sizes = mod.collect_artifact_sizes("owner/repo", runs, ["Android", "MacOS"], listed=listed)
    assert sizes == [_size("QGroundControl", 337 * 1024 * 1024, "337.00 MB", "MacOS")]


def _sizes_main(tmp_path: Path, *options: str) -> int:
    return mod.main(
        [
            "--repo",
            "owner/repo",
            "--head-sha",
            "abc123",
            "--output-dir",
            str(tmp_path / "artifacts"),
            "--workflows",
            "Linux,Windows",
            "--artifact-prefixes",
            "coverage-report,test-results-",
            "--sizes-out",
            str(tmp_path / "sizes.json"),
            *options,
        ]
    )


def test_main_sorts_sizes_by_artifact_name(tmp_path: Path) -> None:
    runs = [_run("Linux", run_id=101), _run("Windows", run_id=102)]
    artifacts = {
        101: [{"name": "QGroundControl-x86_64", "size_in_bytes": 2 * 1024 * 1024}],
        102: [{"name": "QGroundControl-installer-AMD64", "size_in_bytes": 2 * 1024 * 1024}],
    }
    with (
        patch.object(mod, "list_workflow_runs_for_sha", return_value=runs),
        patch.object(mod, "list_run_artifacts", side_effect=lambda repo, run_id: artifacts[run_id]),
    ):
        assert _sizes_main(tmp_path, "--allow-missing") == 0
    assert json.loads((tmp_path / "sizes.json").read_text()) == {
        "head_sha": "abc123",
        "artifacts": [
            _size("QGroundControl-installer-AMD64", 2 * 1024 * 1024, "2.00 MB", "Windows"),
            _size("QGroundControl-x86_64", 2 * 1024 * 1024, "2.00 MB", "Linux"),
        ],
    }


@pytest.mark.parametrize("require_sizes", [False, True])
def test_main_never_keeps_stale_sizes(tmp_path: Path, require_sizes: bool) -> None:
    sizes_path = tmp_path / "sizes.json"
    sizes_path.write_text(json.dumps({"head_sha": "old", "artifacts": [_size("QGC", 1, "")]}))
    with patch.object(mod, "list_workflow_runs_for_sha", return_value=[]):
        rc = _sizes_main(tmp_path, *(["--require-sizes"] if require_sizes else []))
    assert rc == int(require_sizes)
    if require_sizes:
        assert not sizes_path.exists()
    else:
        assert json.loads(sizes_path.read_text()) == {"head_sha": "abc123", "artifacts": []}


def test_require_sizes_needs_a_sizes_file() -> None:
    with patch.object(mod, "list_workflow_runs_for_sha") as query:
        rc = mod.main(
            [
                "--repo",
                "owner/repo",
                "--head-sha",
                "abc123",
                "--artifact-prefixes",
                "coverage-report",
                "--require-sizes",
            ]
        )
    assert rc == 1
    query.assert_not_called()
