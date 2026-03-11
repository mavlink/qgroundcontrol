"""Tests for workflow_runs.py."""

from __future__ import annotations

import json

import workflow_runs as mod


def test_parse_json_documents_handles_paginated_stream() -> None:
    payload = (
        json.dumps({"workflow_runs": [{"id": 1, "name": "Linux"}]})
        + "\n"
        + json.dumps({"workflow_runs": [{"id": 2, "name": "Windows"}]})
    )

    docs = mod.parse_json_documents(payload)
    assert len(docs) == 2
    assert docs[0]["workflow_runs"][0]["id"] == 1
    assert docs[1]["workflow_runs"][0]["id"] == 2


def test_list_workflow_runs_delegates_to_shared_helper(monkeypatch) -> None:
    def fake_list(repo: str, head_sha: str) -> list[dict[str, object]]:
        assert repo == "owner/repo"
        assert head_sha == "abc123"
        return [{"id": 1, "name": "Linux"}]

    monkeypatch.setattr(mod, "_list_workflow_runs_for_sha", fake_list)
    runs = mod.list_workflow_runs("owner/repo", "abc123")
    assert runs == [{"id": 1, "name": "Linux"}]
