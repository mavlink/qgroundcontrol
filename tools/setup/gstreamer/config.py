"""Build settings, plus the Meson options and plugins the desktop builds enable."""

from __future__ import annotations

import tempfile
from dataclasses import dataclass, field
from pathlib import Path

# Common meson options for all Meson-based platforms
MESON_COMMON = {
    "auto_features": "disabled",
    "introspection": "disabled",
    "tests": "disabled",
    "examples": "disabled",
    "gtk_doc": "disabled",
    "gst-full-libraries": "app,video,audio,gl,codecparsers",
    "gpl": "enabled",
    "libav": "enabled",
    "orc": "enabled",
    "qt6": "enabled",
    "base": "enabled",
    "good": "enabled",
    "bad": "enabled",
    "ugly": "enabled",
}

# Base plugins (shared across Meson platforms)
PLUGINS_BASE = {
    "gst-plugins-base:app": "enabled",
    "gst-plugins-base:gl": "enabled",
    "gst-plugins-base:playback": "enabled",
    "gst-plugins-base:tcp": "enabled",
}

# Good plugins (shared across Meson platforms)
PLUGINS_GOOD = {
    "gst-plugins-good:isomp4": "enabled",
    "gst-plugins-good:matroska": "enabled",
    "gst-plugins-good:qt-method": "auto",
    "gst-plugins-good:qt6": "enabled",
    "gst-plugins-good:rtp": "enabled",
    "gst-plugins-good:rtpmanager": "enabled",
    "gst-plugins-good:rtsp": "enabled",
    "gst-plugins-good:udp": "enabled",
}

# Bad plugins (shared across Meson platforms)
PLUGINS_BAD = {
    "gst-plugins-bad:gl": "enabled",
    "gst-plugins-bad:mpegtsdemux": "enabled",
    "gst-plugins-bad:rtp": "enabled",
    "gst-plugins-bad:sdp": "enabled",
    "gst-plugins-bad:videoparsers": "enabled",
}

# Ugly plugins (shared across Meson platforms)
PLUGINS_UGLY = {
    "gst-plugins-ugly:x264": "enabled",
}

# Platform-specific GL configurations
GL_CONFIG = {
    "linux": {
        "gst-plugins-base:gl_api": "opengl,gles2",
        "gst-plugins-base:gl_platform": "glx,egl",
        "gst-plugins-base:gl_winsys": "x11,egl,wayland",
        "gst-plugins-base:x11": "enabled",
        "gst-plugins-good:qt-egl": "enabled",
        "gst-plugins-good:qt-wayland": "enabled",
        "gst-plugins-good:qt-x11": "enabled",
        "gst-plugins-bad:va": "enabled",
        "gst-plugins-bad:wayland": "enabled",
        "gst-plugins-bad:x11": "enabled",
        "gst-plugins-bad:x265": "enabled",
        "vaapi": "enabled",
    },
    "macos": {
        "gst-plugins-base:gl_api": "opengl,gles2",
        "gst-plugins-base:gl_platform": "cgl",
        "gst-plugins-base:gl_winsys": "cocoa",
        "gst-plugins-bad:applemedia": "enabled",
    },
    "windows": {
        "gst-plugins-base:gl_api": "opengl",
        "gst-plugins-base:gl_platform": "wgl,egl",
        "gst-plugins-base:gl_winsys": "win32,egl",
        "gst-plugins-bad:d3d11": "enabled",
        "gst-plugins-bad:d3d12": "enabled",
        "gst-plugins-bad:mediafoundation": "enabled",
    },
}


@dataclass
class BuildConfig:
    platform: str
    arch: str
    version: str
    build_type: str = "release"
    prefix: Path | None = None
    work_dir: Path = field(default_factory=lambda: Path(tempfile.gettempdir()))
    qt_prefix: Path | None = None
    jobs: int | None = None
    clean: bool = False
    simulator: bool = False  # iOS only

    def __post_init__(self):
        if not self.prefix:
            self.prefix = self.work_dir / f"gst-{self.platform}-{self.arch}"
        self.source_dir = self.work_dir / "gstreamer"
        self.build_dir = self.source_dir / f"builddir-{self.arch}"
