#!/usr/bin/env python3
"""Python dependency-group and uv synchronization contracts."""

from __future__ import annotations

from pathlib import Path  # noqa: TC003

import pytest
from setup import install_python
from setup.install_python import get_packages_for_groups, sync_groups_with_uv


def test_package_groups_resolve_and_deduplicate_expected_tools() -> None:
    expected = {
        "test": {"pytest", "jinja2", "pyyaml"},
        "scripts": {"defusedxml>=0.7.1"},
        "precommit,test": {"pre-commit", "pytest"},
        "all": {"pytest", "pre-commit", "meson", "ninja"},
    }
    for groups, required in expected.items():
        packages = get_packages_for_groups(groups)
        assert required <= set(packages)
        assert len(packages) == len(set(packages))
    with pytest.raises(ValueError, match="Unknown group"):
        get_packages_for_groups("nope")


def test_uv_sync_uses_locked_active_environment(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    (tmp_path / "tools").mkdir()
    (tmp_path / "tools" / "uv.lock").write_text("")
    calls: list[list[str]] = []
    monkeypatch.setattr(install_python, "find_repo_root", lambda: tmp_path)
    monkeypatch.setattr(
        install_python.subprocess, "run", lambda args, **_kwargs: calls.append(args)
    )
    sync_groups_with_uv(tmp_path / ".venv", "scripts,test")

    args = calls[0]
    assert args[:5] == ["uv", "sync", "--project", str(tmp_path / "tools"), "--active"]
    assert "--frozen" in args
    assert args.count("--extra") == 2
