"""Build GStreamer from source for QGroundControl; ``setup/build_gstreamer.py`` is the entry script.

Imports only the standard library and ``tools/`` packages, so the script runs with the host
``python3``. ``config`` holds the build settings and Meson options, ``host`` runs commands and
detects the host, ``meson`` builds Linux, macOS and Windows, ``cerbero`` builds Android and iOS,
``cli`` parses arguments, resolves the arch, prefix and CI defaults, and runs the platform's
builder, and ``ci_plan`` (build-gstreamer.yml's first step) validates a dispatch's platform and
arch and resolves its runner's Qt and apt dependencies.
"""
