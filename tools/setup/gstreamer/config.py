"""Build settings, plus the Meson options and plugins the desktop builds enable."""

from __future__ import annotations

import tempfile
from dataclasses import dataclass, field
from pathlib import Path

# Common meson options for all Meson-based platforms. QGC ships only LGPL-compatible GStreamer, so the GPL plugins (and
# gst-plugins-ugly, whose only plugin QGC used was the GPL x264enc) stay off; QGC renders through its own video sink,
# so neither is the Qt6 plugin built.
MESON_COMMON = {
    "auto_features": "disabled",
    "introspection": "disabled",
    "tests": "disabled",
    "examples": "disabled",
    "gtk_doc": "disabled",
    "gst-full-libraries": "app,video,audio,gl,codecparsers",
    "gpl": "disabled",
    "libav": "enabled",
    "orc": "enabled",
    "qt6": "disabled",
    "base": "enabled",
    "good": "enabled",
    "bad": "enabled",
    "ugly": "disabled",
}

# The Meson option that builds each plugin of the allowlist (gstreamer.plugins in .github/build-config.json), which the
# build must ship: auto_features=disabled leaves every other plugin out.
PLUGIN_OPTIONS = {
    "app": "gst-plugins-base:app",
    "opengl": "gst-plugins-base:gl",
    "playback": "gst-plugins-base:playback",
    "tcp": "gst-plugins-base:tcp",
    "typefindfunctions": "gst-plugins-base:typefind",
    "videoconvertscale": "gst-plugins-base:videoconvertscale",
    "isomp4": "gst-plugins-good:isomp4",
    "jpeg": "gst-plugins-good:jpeg",
    "matroska": "gst-plugins-good:matroska",
    "rtp": "gst-plugins-good:rtp",
    "rtpmanager": "gst-plugins-good:rtpmanager",
    "rtsp": "gst-plugins-good:rtsp",
    "udp": "gst-plugins-good:udp",
    "vpx": "gst-plugins-good:vpx",
    "applemedia": "gst-plugins-bad:applemedia",
    "d3d": "gst-plugins-bad:d3dvideosink",
    "d3d11": "gst-plugins-bad:d3d11",
    "d3d12": "gst-plugins-bad:d3d12",
    "jpegformat": "gst-plugins-bad:jpegformat",
    "mpegtsdemux": "gst-plugins-bad:mpegtsdemux",
    "nvcodec": "gst-plugins-bad:nvcodec",
    "openh264": "gst-plugins-bad:openh264",
    "qsv": "gst-plugins-bad:qsv",
    "sdpelem": "gst-plugins-bad:sdp",
    "va": "gst-plugins-bad:va",
    "videoparsersbad": "gst-plugins-bad:videoparsers",
    "vulkan": "gst-plugins-bad:vulkan",
}

# Allowlisted plugins without a plugin option here, and why.
PLUGINS_WITHOUT_OPTION = {
    "coreelements": "part of the GStreamer core",
    "libav": "gst-libav, which MESON_COMMON enables",
    "videoconvert": "folded into videoconvertscale in GStreamer 1.22",
    "videoscale": "folded into videoconvertscale in GStreamer 1.22",
    "dav1d": "gst-plugins-rs, which needs Rust and this build leaves out",
    "androidmedia": "Android, which Cerbero builds",
}

# The gstreamer.plugins key of build-config.json that adds to the common plugins on each Meson platform.
PLUGIN_KEYS = {"linux": "linux", "macos": "apple", "windows": "windows"}

# GStreamer's OpenGL integration in gst-plugins-bad, which its GL-based plugins use.
PLUGINS_BAD_SUPPORT = {"gst-plugins-bad:gl": "enabled"}

# Platform-specific OpenGL and windowing configuration
GL_CONFIG = {
    "linux": {
        "gst-plugins-base:gl_api": "opengl,gles2",
        "gst-plugins-base:gl_platform": "glx,egl",
        "gst-plugins-base:gl_winsys": "x11,egl,wayland",
        "gst-plugins-base:x11": "enabled",
        "gst-plugins-bad:wayland": "enabled",
        "gst-plugins-bad:x11": "enabled",
    },
    "macos": {
        "gst-plugins-base:gl_api": "opengl,gles2",
        "gst-plugins-base:gl_platform": "cgl",
        "gst-plugins-base:gl_winsys": "cocoa",
    },
    "windows": {
        "gst-plugins-base:gl_api": "opengl",
        "gst-plugins-base:gl_platform": "wgl,egl",
        "gst-plugins-base:gl_winsys": "win32,egl",
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


def plugin_options(platform: str, plugins: dict[str, list[str]]) -> dict[str, str]:
    """Return the Meson options that build *platform*'s allowlisted plugins.

    *plugins* is build-config.json's gstreamer.plugins. Raises ValueError for an allowlisted plugin with neither a
    PLUGIN_OPTIONS nor a PLUGINS_WITHOUT_OPTION entry, so a new plugin cannot be left out unnoticed.
    """
    names = [*plugins.get("common", []), *plugins.get(PLUGIN_KEYS[platform], [])]
    unknown = [
        name for name in names if name not in PLUGIN_OPTIONS and name not in PLUGINS_WITHOUT_OPTION
    ]
    if unknown:
        raise ValueError(f"No Meson option for allowlisted GStreamer plugins: {', '.join(unknown)}")
    return {PLUGIN_OPTIONS[name]: "enabled" for name in names if name in PLUGIN_OPTIONS}
