# Pkg-config target creation — shared by Linux/Windows/macOS/Android
# (everything except iOS xcframework). Pkg-config env management lives in
# cmake/GStreamer/PkgConfig.cmake (gstreamer_apply_pkgconfig_env), target creation
# in cmake/GStreamer/Targets.cmake.

include_guard(GLOBAL)

# _qgc_create_pkgconfig_targets()
# Creates the GStreamer:: targets for the discovered SDK, then applies the macOS framework overlay or builds the Android
# mobile target. Sets GStreamer_VERSION, on static layouts appends --static to PKG_CONFIG_ARGN and, on Android, sets
# GSTREAMER_STATIC_PLUGINS, all in the caller's scope.
# cmakelang 0.6.13 predates return(PROPAGATE).
# cmake-lint: disable=C0103,E1121
function(_qgc_create_pkgconfig_targets)
    # MACOS is set out-of-band by cmake/Toolchain.cmake; the overlay below gates on
    # it, so warn loudly if it's unset rather than silently no-op the overlay.
    if(APPLE
       AND NOT IOS
       AND NOT DEFINED MACOS
    )
        message(WARNING "GStreamer: MACOS is undefined on an Apple host — cmake/Toolchain.cmake "
                        "must run before _qgc_create_pkgconfig_targets or the macOS framework "
                        "overlay will be skipped."
        )
    endif()

    set(_outputs GStreamer_VERSION)
    # Normal variable: the GStreamer subtree's later pkg-config probes (HwBuffers) need the same flags.
    if(GStreamer_USE_STATIC_LIBS AND NOT "--static" IN_LIST PKG_CONFIG_ARGN)
        list(APPEND PKG_CONFIG_ARGN "--static")
        list(APPEND _outputs PKG_CONFIG_ARGN)
    endif()

    set(_qgc_gst_target_args "")
    if(GStreamer_USE_STATIC_LIBS)
        list(APPEND _qgc_gst_target_args STATIC)
    endif()
    if(GStreamer_USE_FRAMEWORK)
        list(APPEND _qgc_gst_target_args FRAMEWORK_PATH "${GSTREAMER_FRAMEWORK_PATH}")
    endif()
    # QGC_GST_REQUESTED_VERSION is the per-platform floor chosen by Orchestrator.cmake — Linux honors the distro
    # minimum (1.20), bundled-SDK platforms enforce ≥1.28 so a downgraded SDK fails configure loudly.
    qgc_gst_create_pkgconfig_targets(
        ROOT
        "${GStreamer_ROOT_DIR}"
        VERSION
        "${QGC_GST_REQUESTED_VERSION}"
        APIS
        ${GSTREAMER_APIS}
        PLUGINS
        ${GSTREAMER_PLUGINS}
        ${_qgc_gst_target_args}
    )

    # Apple framework overlay — defined in platform/MacOS.cmake; iOS uses xcframework.
    if(MACOS
       AND GStreamer_USE_FRAMEWORK
       AND TARGET GStreamer::GStreamer
    )
        _qgc_apply_macos_framework_overlay()
    endif()

    # Android mobile-target setup: bundle plugins + optional fonts/CA assets into the
    # GStreamerMobile target. iOS uses the xcframework path upstream, so this is Android-only.
    if(ANDROID)
        _qgc_create_android_mobile_target()
        list(APPEND _outputs GSTREAMER_STATIC_PLUGINS)
    endif()
    return(PROPAGATE ${_outputs})
endfunction()
