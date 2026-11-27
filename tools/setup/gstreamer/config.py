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
    "gpl": "disabled",
    "libav": "enabled",
    "orc": "enabled",
    "qt6": "disabled",
    "base": "enabled",
    "good": "enabled",
    "bad": "enabled",
    "ugly": "disabled",
    # gst-plugin-scanner, which QGC installs with the plugins, and gst-inspect-1.0 and gst-launch-1.0.
    "tools": "enabled",
    # gstreamer-rtsp-server-1.0, which a Debug build of QGC links for MockLink's video stream. Where the prefix lacks
    # it, a Linux configure goes without, as the system's is another GStreamer's (cmake/GStreamer/Targets.cmake).
    "rtsp_server": "enabled",
    # gstreamer-check-1.0, which QGC's GstHarness tests link. gst-plugins-good's and gst-libav's docs/meson.build also
    # read a variable that the core's defines only after it, so configure stops without it.
    "gstreamer:check": "enabled",
}

# gst-libav builds FFmpeg from GStreamer's wrap, whose Meson port makes every codec a feature option, so
# auto_features=disabled turns them all off (Meson has no auto_features of a subproject's own). These are the ones
# the libav plugin gets. Its decoders are for the video a stream QGC plays can carry, less MJPEG, VP8 and VP9, which
# the jpeg and vpx plugins decode.
FFMPEG_FEATURES = (
    "h264_decoder",
    "hevc_decoder",
    "mpeg2video_decoder",
    # FFmpeg's MPEG-4 decoder is built on its H.263 decoder, and that one on the H.263 parser. Without the part it
    # is built on, a decoder is left out with no word of it.
    "mpeg4_decoder",
    "h263_decoder",
    "h263_parser",
    # The one encoder: QGC's tests play MPEG-2 video as an MPEG-TS camera sends it, and nothing else encodes it.
    "mpeg2video_encoder",
)

# Where each platform's build takes the libraries GStreamer depends on. macOS and Windows have none to offer, so every
# one GStreamer has a wrap for is built into the prefix. Linux links the system's, which the gstreamer_build packages
# install, and fails on a missing one: forced fallbacks there build a GLib beside the one Qt loads and then stop at
# wayland-protocols, whose wayland subproject has no wrap. Of the wraps Linux builds FFmpeg, without the GPL parts
# Ubuntu's has, and orc, which GStreamer's build takes from its wrap by default.
WRAP_ARGS = {
    "linux": ["--wrap-mode=nofallback", "--force-fallback-for=FFmpeg"],
    "macos": ["--wrap-mode=forcefallback"],
    "windows": ["--wrap-mode=forcefallback"],
}

# The Meson option that builds each plugin the build must have: auto_features=disabled leaves every other plugin out.
# They are the plugins of the allowlist (gstreamer.plugins in .github/build-config.json) and, as the build is one to
# develop against, those the Debug-only MockLink video server and QGC's tests make their streams with
# (gstreamer_plugin_policy.mock_server_debug), which QGC never installs.
PLUGIN_OPTIONS = {
    "coretracers": "gstreamer:coretracers",
    "app": "gst-plugins-base:app",
    "opengl": "gst-plugins-base:gl",
    "playback": "gst-plugins-base:playback",
    "tcp": "gst-plugins-base:tcp",
    "typefindfunctions": "gst-plugins-base:typefind",
    "videoconvertscale": "gst-plugins-base:videoconvertscale",
    "videotestsrc": "gst-plugins-base:videotestsrc",
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
    "mpegtsmux": "gst-plugins-bad:mpegtsmux",
    "nvcodec": "gst-plugins-bad:nvcodec",
    "openh264": "gst-plugins-bad:openh264",
    "qsv": "gst-plugins-bad:qsv",
    "sdpelem": "gst-plugins-bad:sdp",
    "switchbin": "gst-plugins-bad:switchbin",
    "va": "gst-plugins-bad:va",
    "videoparsersbad": "gst-plugins-bad:videoparsers",
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
        # The DRM allocator. gstreamer-allocators-1.0 then requires libdrm, as a distribution's does: QGC's DMABuf
        # path includes libdrm's drm_fourcc.h through it.
        "gst-plugins-base:drm": "enabled",
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
    jobs: int | None = None
    clean: bool = False
    simulator: bool = False  # iOS only

    def __post_init__(self):
        if not self.prefix:
            self.prefix = self.work_dir / f"gst-{self.platform}-{self.arch}"
        self.source_dir = self.work_dir / "gstreamer"
        self.build_dir = self.source_dir / f"builddir-{self.arch}"


def plugin_options(
    platform: str, plugins: dict[str, list[str]], debug_plugins: list[str] | None = None
) -> dict[str, str]:
    """Return the Meson options that build *platform*'s allowlisted plugins and *debug_plugins*.

    *plugins* is build-config.json's gstreamer.plugins and *debug_plugins* its
    gstreamer_plugin_policy.mock_server_debug. Raises ValueError for a plugin with neither a PLUGIN_OPTIONS nor a
    PLUGINS_WITHOUT_OPTION entry, so a new plugin cannot be left out unnoticed.
    """
    names = [
        *plugins.get("common", []),
        *plugins.get(PLUGIN_KEYS[platform], []),
        *(debug_plugins or []),
    ]
    unknown = [
        name for name in names if name not in PLUGIN_OPTIONS and name not in PLUGINS_WITHOUT_OPTION
    ]
    if unknown:
        raise ValueError(f"No Meson option for GStreamer plugins: {', '.join(unknown)}")
    return {PLUGIN_OPTIONS[name]: "enabled" for name in names if name in PLUGIN_OPTIONS}
