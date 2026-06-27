# Pkg-config target creation — shared by Linux/Windows/macOS/Android
# (everything except iOS xcframework). Pkg-config env management lives in
# cmake/GStreamer/PkgConfig.cmake (gstreamer_apply_pkgconfig_env). Macro form
# is required so GStreamer_FOUND / IMPORTED targets / cache vars propagate to
# the caller.

macro(_qgc_create_pkgconfig_targets)

# MACOS is set out-of-band by cmake/Toolchain.cmake; the overlay below gates on
# it, so warn loudly if it's unset rather than silently no-op the overlay.
if(APPLE AND NOT IOS AND NOT DEFINED MACOS)
    message(WARNING
        "GStreamer: MACOS is undefined on an Apple host — cmake/Toolchain.cmake "
        "must run before _qgc_create_pkgconfig_targets or the macOS framework "
        "overlay will be skipped.")
endif()

if(GStreamer_USE_STATIC_LIBS AND NOT "--static" IN_LIST PKG_CONFIG_ARGN)
    list(APPEND PKG_CONFIG_ARGN "--static")
endif()

find_package(PkgConfig REQUIRED QUIET)

list(PREPEND CMAKE_PREFIX_PATH "${GStreamer_ROOT_DIR}")

# FORCE so derived flags from the helpers above (--static, --dont-define-prefix,
# --define-variable=…) propagate to the cache; guards above prevent duplicate appends.
set(PKG_CONFIG_ARGN "${PKG_CONFIG_ARGN}" CACHE STRING "Arguments to supply to pkg-config" FORCE)

# CPM creates a stub gstreamer-config.cmake that shadows our vendored module
if(DEFINED CMAKE_FIND_PACKAGE_REDIRECTS_DIR)
    file(REMOVE
        "${CMAKE_FIND_PACKAGE_REDIRECTS_DIR}/gstreamer-config.cmake"
        "${CMAKE_FIND_PACKAGE_REDIRECTS_DIR}/gstreamer-config-version.cmake"
        "${CMAKE_FIND_PACKAGE_REDIRECTS_DIR}/GStreamerConfig.cmake"
        "${CMAKE_FIND_PACKAGE_REDIRECTS_DIR}/GStreamerConfigVersion.cmake"
    )
endif()
unset(GStreamer_FOUND CACHE)
unset(GStreamer_FOUND)
# Use the per-platform version chosen at the top of this file as the floor — Linux honors the distro
# minimum (1.20), bundled-SDK platforms enforce ≥1.28 so a downgraded SDK fails configure loudly.
find_package(GStreamer ${GStreamer_FIND_VERSION} REQUIRED MODULE)

# Apple framework overlay — defined in platform/MacOS.cmake; iOS uses xcframework.
if(MACOS AND GStreamer_USE_FRAMEWORK AND TARGET GStreamer::GStreamer)
    _qgc_apply_macos_framework_overlay()
endif()

# Android mobile-target setup: bundle plugins + optional fonts/CA assets into the
# GStreamerMobile target. iOS uses the xcframework path upstream, so this is Android-only.
if(ANDROID)
    set(_mobile_components ${GSTREAMER_PLUGINS} mobile)
    if(EXISTS "${GStreamer_ROOT_DIR}/share/gst-android/ndk-build/fontconfig")
        list(APPEND _mobile_components fonts)
    endif()
    if(EXISTS "${GStreamer_ROOT_DIR}/etc/ssl/certs/ca-certificates.crt")
        list(APPEND _mobile_components ca_certificates)
    endif()
    set(GStreamerMobile_FIND_COMPONENTS ${_mobile_components})
    _qgc_create_android_mobile_target()
    set(GStreamerMobile_FOUND TRUE)
endif()

endmacro()
