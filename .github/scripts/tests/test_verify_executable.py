"""Tests for verify_executable.py."""

from __future__ import annotations

import os
import subprocess

import pytest
from verify_executable import _setup_gstreamer_env, _verify_appimage_update_information


class TestSetupGstreamerEnv:
    """Tests for _setup_gstreamer_env function."""

    def test_no_cache_file(self, tmp_path, monkeypatch):
        """Returns without setting env if CMakeCache.txt is missing."""
        monkeypatch.delenv("GST_PLUGIN_PATH", raising=False)
        _setup_gstreamer_env(tmp_path)
        assert "GST_PLUGIN_PATH" not in os.environ

    def test_no_gstreamer_in_cache(self, tmp_path, monkeypatch):
        """Returns without setting env if no GStreamer_ROOT_DIR in cache."""
        cache = tmp_path / "CMakeCache.txt"
        cache.write_text("CMAKE_BUILD_TYPE:STRING=Release\n")
        monkeypatch.delenv("GST_PLUGIN_PATH", raising=False)
        _setup_gstreamer_env(tmp_path)
        assert "GST_PLUGIN_PATH" not in os.environ

    def test_sets_plugin_path(self, tmp_path, monkeypatch):
        """Sets GST_PLUGIN_PATH when plugin dir exists."""
        gst_root = tmp_path / "gstreamer"
        plugin_dir = gst_root / "lib" / "gstreamer-1.0"
        plugin_dir.mkdir(parents=True)

        cache = tmp_path / "CMakeCache.txt"
        cache.write_text(f"GStreamer_ROOT_DIR:PATH={gst_root}\n")

        monkeypatch.delenv("GST_PLUGIN_PATH", raising=False)
        _setup_gstreamer_env(tmp_path)

        assert os.environ["GST_PLUGIN_PATH"] == str(plugin_dir)
        assert os.environ["GST_PLUGIN_SYSTEM_PATH"] == str(plugin_dir)


def test_verify_appimage_update_information(tmp_path, monkeypatch):
    appimage = tmp_path / "QGroundControl-x86_64.AppImage"
    update_information = (
        "gh-releases-zsync|mavlink|qgroundcontrol|latest|QGroundControl-x86_64.AppImage.zsync"
    )
    (tmp_path / f"{appimage.name}.zsync").write_text("zsync metadata")
    monkeypatch.setattr(
        subprocess,
        "run",
        lambda *args, **kwargs: subprocess.CompletedProcess(
            args[0], 0, stdout=f"{update_information}\n", stderr=""
        ),
    )

    _verify_appimage_update_information(appimage, update_information)


def test_verify_appimage_update_information_rejects_mismatch_or_missing_sidecar(
    tmp_path, monkeypatch
):
    appimage = tmp_path / "QGroundControl-x86_64.AppImage"
    monkeypatch.setattr(
        subprocess,
        "run",
        lambda *args, **kwargs: subprocess.CompletedProcess(
            args[0], 0, stdout="unexpected\n", stderr=""
        ),
    )

    with pytest.raises(RuntimeError, match="Unexpected AppImage update information"):
        _verify_appimage_update_information(appimage, "expected")

    monkeypatch.setattr(
        subprocess,
        "run",
        lambda *args, **kwargs: subprocess.CompletedProcess(
            args[0], 0, stdout="expected\n", stderr=""
        ),
    )
    with pytest.raises(RuntimeError, match="zsync sidecar is missing or empty"):
        _verify_appimage_update_information(appimage, "expected")
