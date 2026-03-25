"""Tests for verify_executable.py."""

from __future__ import annotations

import os

from verify_executable import _setup_gstreamer_env


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
