"""Contracts for executable-test GStreamer environment setup."""

from __future__ import annotations

import os
from pathlib import Path
from typing import TYPE_CHECKING

from verify_executable import _setup_gstreamer_env, resolve_executable

if TYPE_CHECKING:
    import pytest


def test_missing_cache_or_gstreamer_root_is_a_noop(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    monkeypatch.delenv("GST_PLUGIN_PATH", raising=False)
    _setup_gstreamer_env(tmp_path)
    (tmp_path / "CMakeCache.txt").write_text("CMAKE_BUILD_TYPE:STRING=Release\n")
    _setup_gstreamer_env(tmp_path)
    assert "GST_PLUGIN_PATH" not in os.environ


def test_existing_gstreamer_plugins_set_both_search_paths(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    plugins = tmp_path / "gstreamer/lib/gstreamer-1.0"
    plugins.mkdir(parents=True)
    (tmp_path / "CMakeCache.txt").write_text(f"GStreamer_ROOT_DIR:PATH={tmp_path / 'gstreamer'}\n")
    monkeypatch.delenv("GST_PLUGIN_PATH", raising=False)
    _setup_gstreamer_env(tmp_path)
    assert os.environ["GST_PLUGIN_PATH"] == str(plugins)
    assert os.environ["GST_PLUGIN_SYSTEM_PATH"] == str(plugins)


def test_executable_resolution_preserves_paths_and_supports_workdir_basenames(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    workspace = tmp_path / "workspace"
    workdir = tmp_path / "runtime"
    nested = workspace / "build/bin/QGroundControl"
    workdir.mkdir()
    nested.parent.mkdir(parents=True)
    nested.touch()
    (workdir / "Custom-QGroundControl.AppImage").touch()
    monkeypatch.chdir(workspace)

    executable, resolved_workdir = resolve_executable(Path("build/bin/QGroundControl"), workdir)
    assert executable == nested
    assert resolved_workdir == workdir

    executable, resolved_workdir = resolve_executable(
        Path("Custom-QGroundControl.AppImage"), workdir
    )
    assert executable == workdir / "Custom-QGroundControl.AppImage"
    assert resolved_workdir == workdir
