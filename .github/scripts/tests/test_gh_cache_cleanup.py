"""Contract tests for GitHub cache cleanup and pruning."""

from __future__ import annotations

import subprocess
from typing import TYPE_CHECKING, Any

import gh_cache_cleanup as mod
import pytest
from _helpers import completed

if TYPE_CHECKING:
    from pathlib import Path


def _usage(
    key: str,
    mb: int,
    *,
    cache_id: int = 1,
    ref: str = "refs/heads/master",
    accessed: str = "2026-01-01",
) -> Any:
    return mod.CacheUsage(
        cache_id=cache_id,
        key=key,
        ref=ref,
        size_bytes=mb * 1024 * 1024,
        last_accessed=accessed,
    )


def test_list_caches_parses_rows_and_normalizes_ref(
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    payload = '[{"key":"key-1","sizeInBytes":104857600,"ref":"refs/heads/main"}]'
    for branch, expected_ref in (
        ("", None),
        ("feature-x", "refs/heads/feature-x"),
        ("refs/pull/1/merge", "refs/pull/1/merge"),
    ):
        calls: list[list[str]] = []

        def run(
            command: list[str], calls: list[list[str]] = calls, **kwargs: Any
        ) -> subprocess.CompletedProcess:
            calls.append(command)
            return completed(payload)

        monkeypatch.setattr(subprocess, "run", run)
        rows = mod.list_caches("owner/repo", branch)
        assert [(row.key, row.size, row.ref) for row in rows] == [
            ("key-1", "100.0 MiB", "refs/heads/main")
        ]
        if expected_ref is None:
            assert "--ref" not in calls[0]
        else:
            assert calls[0][calls[0].index("--ref") + 1] == expected_ref


def test_delete_caches_skips_empty_keys_and_counts_outcomes(
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    outcomes = iter([0, 1, 0])
    calls: list[list[str]] = []

    def run(command: list[str], **kwargs: Any) -> subprocess.CompletedProcess:
        calls.append(command)
        return completed(returncode=next(outcomes))

    monkeypatch.setattr(subprocess, "run", run)
    assert mod.delete_caches("owner/repo", "", ["a", "", "b", "c"]) == (2, 1)
    assert len(calls) == 3


def test_main_dry_run_and_delete_write_outputs(
    monkeypatch: pytest.MonkeyPatch, gh_output: Path, tmp_path: Path
) -> None:
    monkeypatch.setenv("GH_REPO", "owner/repo")
    summary = tmp_path / "summary"
    monkeypatch.setenv("GITHUB_STEP_SUMMARY", str(summary))
    monkeypatch.setattr(
        mod,
        "list_caches",
        lambda *args, **kwargs: [
            mod.CacheRow("k1", "1 MiB", "main"),
            mod.CacheRow("k2", "2 MiB", "main"),
        ],
    )
    deleted: list[list[str]] = []
    monkeypatch.setattr(
        mod,
        "delete_caches",
        lambda repo, branch, keys: deleted.append(keys) or (len(keys), 0),
    )

    assert mod.main(["--summary"]) == 0
    assert deleted == []
    assert "count=2" in gh_output.read_text()
    assert "Dry run" in summary.read_text()

    assert mod.main(["--delete", "--summary"]) == 0
    assert deleted == [["k1", "k2"]]
    assert "Deletion Results" in summary.read_text()


def test_main_requires_repository_context(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.delenv("GH_REPO", raising=False)
    monkeypatch.delenv("GITHUB_REPOSITORY", raising=False)
    with pytest.raises(SystemExit):
        mod.main([])


def test_prune_policy_respects_watermarks_protection_and_largest_first() -> None:
    under = [_usage("ccache-linux", 3000), _usage("avd-Linux", 1000)]
    victims, total, projected = mod.select_prune_victims(
        under, keep_mb=6500, high_water_mb=9000, protect=mod.DEFAULT_PROTECT
    )
    assert victims == []
    assert total == projected == 4000 * 1024 * 1024

    over = [
        _usage("ccache-linux", 5000),
        _usage("avd-Linux", 1750),
        _usage("codeql-trap", 1370),
        _usage("gradle-wrapper", 400),
    ]
    victims, _, projected = mod.select_prune_victims(
        over, keep_mb=6500, high_water_mb=8000, protect=mod.DEFAULT_PROTECT
    )
    assert [victim.key for victim in victims] == ["avd-Linux", "codeql-trap"]
    assert projected <= 6500 * 1024 * 1024

    protected = [_usage("ccache-linux", 9500), _usage("avd-Linux", 1000)]
    victims, _, projected = mod.select_prune_victims(
        protected, keep_mb=6500, high_water_mb=9000, protect=mod.DEFAULT_PROTECT
    )
    assert [victim.key for victim in victims] == ["avd-Linux"]
    assert projected == 9500 * 1024 * 1024


def test_list_cache_usage_parses_api_payload(monkeypatch: pytest.MonkeyPatch) -> None:
    payload = '[{"id":17,"key":"ccache-x","ref":"refs/heads/master","sizeInBytes":1048576,"lastAccessedAt":"2026-01-01"}]'
    monkeypatch.setattr(subprocess, "run", lambda *args, **kwargs: completed(payload))
    rows = mod.list_caches_usage("owner/repo")
    assert (rows[0].cache_id, rows[0].key, rows[0].size_bytes) == (17, "ccache-x", 1048576)


def test_delete_cache_ids_targets_exact_entries(monkeypatch: pytest.MonkeyPatch) -> None:
    calls: list[list[str]] = []

    def run(command: list[str], **kwargs: Any) -> subprocess.CompletedProcess:
        calls.append(command)
        return completed()

    monkeypatch.setattr(subprocess, "run", run)
    assert mod.delete_cache_ids("owner/repo", [17, 23]) == (2, 0)
    assert [call[3] for call in calls] == ["17", "23"]


def test_prune_main_is_dry_by_default_and_deletes_only_victims(
    monkeypatch: pytest.MonkeyPatch, gh_output: Path
) -> None:
    monkeypatch.setenv("GH_REPO", "owner/repo")
    requested_limits: list[int] = []

    def list_usage(repo: str, limit: int) -> list[Any]:
        del repo
        requested_limits.append(limit)
        return [_usage("ccache-x", 9000), _usage("avd", 2000)]

    monkeypatch.setattr(mod, "list_caches_usage", list_usage)
    deleted: list[list[int]] = []
    monkeypatch.setattr(
        mod,
        "delete_cache_ids",
        lambda repo, cache_ids: deleted.append(cache_ids) or (len(cache_ids), 0),
    )

    assert mod.main(["--prune"]) == 0
    assert deleted == []
    assert "deleted=0" in gh_output.read_text()

    assert mod.main(["--prune", "--delete"]) == 0
    assert deleted == [[1]]
    assert requested_limits == [mod._PRUNE_CACHE_LIMIT, mod._PRUNE_CACHE_LIMIT]
