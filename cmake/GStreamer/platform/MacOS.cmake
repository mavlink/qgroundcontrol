# macOS GStreamer SDK discovery — invoked by Orchestrator.cmake.

include_guard(GLOBAL)

# _qgc_gst_homebrew_prefix(<root> <out_var>)
# Sets <out_var> to the Homebrew prefix that <root> is or holds as opt/<formula> or Cellar/<formula>/<version>, or to ""
# when <root> is not in a Homebrew prefix.
function(_qgc_gst_homebrew_prefix root out_var)
    set(_prefix "${root}")
    if(root MATCHES "^(.+)/(opt/[^/]+|Cellar/[^/]+/[^/]+)/?$")
        set(_prefix "${CMAKE_MATCH_1}")
    endif()
    if(NOT EXISTS "${_prefix}/bin/brew")
        set(_prefix "")
    endif()
    set(${out_var}
        "${_prefix}"
        PARENT_SCOPE
    )
endfunction()

# _qgc_gst_find_homebrew_gstreamer(<out_var>)
# Sets <out_var> to Homebrew's gstreamer: `brew --prefix gstreamer`, else /opt/homebrew/opt/gstreamer or
# /usr/local/opt/gstreamer. Fails configure when none is installed.
function(_qgc_gst_find_homebrew_gstreamer out_var)
    set(_kegs /opt/homebrew/opt/gstreamer /usr/local/opt/gstreamer)
    find_program(_brew brew PATHS /opt/homebrew/bin /usr/local/bin NO_CACHE)
    if(_brew)
        execute_process(
            COMMAND "${_brew}" --prefix gstreamer
            OUTPUT_VARIABLE _brew_keg
            OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET
        )
        list(PREPEND _kegs ${_brew_keg})
    endif()
    foreach(_keg IN LISTS _kegs)
        if(EXISTS "${_keg}/lib/pkgconfig/gstreamer-1.0.pc")
            set(${out_var}
                "${_keg}"
                PARENT_SCOPE
            )
            return()
        endif()
    endforeach()
    message(
        FATAL_ERROR "GStreamer: QGC_GST_USE_HOMEBREW is ON but Homebrew's gstreamer is not in any of: ${_kegs}\n"
                    "Install it with: brew install gstreamer pkgconf"
    )
endfunction()

# _qgc_gst_discover_homebrew_sdk(<prefix>)
# Discovery of the Homebrew GStreamer at GStreamer_ROOT_DIR, in Homebrew <prefix> (the prefix itself means its
# opt/gstreamer), through the prefix's pkg-config: only that one searches the macOS shims (libffi, zlib) GLib requires;
# the dirs given to it find an unlinked gstreamer or glib. Sets the HOMEBREW_PREFIX layout property (Install.cmake).
# cmake-lint: disable=C0103
function(_qgc_gst_discover_homebrew_sdk prefix)
    if(GStreamer_ROOT_DIR STREQUAL prefix)
        set(GStreamer_ROOT_DIR "${prefix}/opt/gstreamer")
    endif()
    if(NOT EXISTS "${prefix}/bin/pkg-config")
        message(
            FATAL_ERROR "GStreamer: Homebrew's GStreamer needs Homebrew's pkg-config. Install it with: "
                        "brew install pkgconf"
        )
    endif()
    message(
        STATUS "GStreamer: Using Homebrew's GStreamer at ${GStreamer_ROOT_DIR} (builds only; installs and "
               "packages need the official SDK)"
    )
    gstreamer_create_layout_target(SDK_ROOT "${GStreamer_ROOT_DIR}")
    gstreamer_apply_pkgconfig_env(
        MODE SYSTEM_AUGMENT
        PKG_CONFIG_EXE "${prefix}/bin/pkg-config"
        LIBDIR "${GSTREAMER_LIB_PATH}/pkgconfig" "${prefix}/lib/pkgconfig" "${prefix}/opt/glib/lib/pkgconfig"
    )
    gstreamer_layout_set(HOMEBREW_PREFIX "${prefix}")
    _qgc_gst_return_discovery()
endfunction()

# _qgc_gst_discover_macos_sdk()
# Uses the SDK under GStreamer_ROOT_DIR; else, with QGC_GST_USE_HOMEBREW, Homebrew's gstreamer; else an earlier
# auto-downloaded SDK, /Library/Frameworks/GStreamer.framework, or a download of the official runtime and devel
# packages. A root in a Homebrew prefix goes to _qgc_gst_discover_homebrew_sdk. A root inside a .framework bundle is
# used as that framework (GStreamer_USE_FRAMEWORK, GSTREAMER_FRAMEWORK_PATH) with the pkg-config it brings; another
# flat SDK uses Homebrew's pkg-config (cached PKG_CONFIG_EXECUTABLE).
# cmake-lint: disable=C0103
function(_qgc_gst_discover_macos_sdk)
    if(QGC_GST_USE_HOMEBREW AND (NOT GStreamer_ROOT_DIR OR GStreamer_AUTO_DOWNLOADED))
        _qgc_gst_find_homebrew_gstreamer(GStreamer_ROOT_DIR)
        set(GStreamer_AUTO_DOWNLOADED FALSE)
    elseif(NOT GStreamer_ROOT_DIR AND EXISTS "/Library/Frameworks/GStreamer.framework")
        set(GStreamer_ROOT_DIR "/Library/Frameworks/GStreamer.framework/Versions/1.0")
    endif()

    if(NOT GStreamer_ROOT_DIR OR NOT EXISTS "${GStreamer_ROOT_DIR}")
        _qgc_gst_acquire_macos_sdk(GStreamer_ROOT_DIR)
        set(GStreamer_AUTO_DOWNLOADED TRUE)
    endif()

    _qgc_gst_homebrew_prefix("${GStreamer_ROOT_DIR}" _brew_prefix)
    if(_brew_prefix)
        _qgc_gst_discover_homebrew_sdk("${_brew_prefix}")
        _qgc_gst_return_discovery()
    endif()

    set(GStreamer_USE_FRAMEWORK OFF)
    if(GStreamer_ROOT_DIR MATCHES "^(.*\\.framework)(/|$)")
        set(GStreamer_USE_FRAMEWORK ON)
        gstreamer_create_layout_target(SDK_ROOT "${GStreamer_ROOT_DIR}" FRAMEWORK_BUNDLE "${CMAKE_MATCH_1}")
        gstreamer_apply_pkgconfig_env(
            MODE SDK
            PKG_CONFIG_EXE "${GStreamer_ROOT_DIR}/bin/pkg-config"
            LIBDIR "${GSTREAMER_LIB_PATH}/pkgconfig" "${GSTREAMER_PLUGIN_PATH}/pkgconfig"
        )
    else()
        gstreamer_create_layout_target(SDK_ROOT "${GStreamer_ROOT_DIR}")
        _qgc_gst_find_apple_pkg_config(PKG_CONFIG_EXECUTABLE)
        gstreamer_apply_pkgconfig_env(
            MODE SDK LIBDIR "${GSTREAMER_LIB_PATH}/pkgconfig" "${GSTREAMER_PLUGIN_PATH}/pkgconfig"
        )
    endif()
    _qgc_gst_return_discovery()
endfunction()

# _qgc_gst_apply_macos_framework_overlay()
# Links GStreamer::GStreamer to the GStreamer.framework at GSTREAMER_FRAMEWORK_PATH and adds its Headers.
function(_qgc_gst_apply_macos_framework_overlay)
    set(CMAKE_FIND_FRAMEWORK ONLY)
    cmake_path(GET GSTREAMER_FRAMEWORK_PATH PARENT_PATH _gst_framework_parent)
    find_library(
        _gst_framework_bundle GStreamer PATHS "${_gst_framework_parent}" "${GSTREAMER_FRAMEWORK_PATH}"
                                              "/Library/Frameworks" NO_CACHE
    )
    if(NOT _gst_framework_bundle)
        message(FATAL_ERROR "GStreamer: Could not locate GStreamer.framework")
    endif()
    target_link_libraries(GStreamer::GStreamer INTERFACE ${_gst_framework_bundle})
    target_include_directories(GStreamer::GStreamer INTERFACE "${_gst_framework_bundle}/Headers")
    target_compile_definitions(GStreamer::GStreamer INTERFACE QGC_GST_MACOS_FRAMEWORK)
    # Stash the discovered framework bundle path on GStreamer::Layout so the
    # install helpers read it via gstreamer_layout_get instead of via cache pickup.
    gstreamer_layout_set(FRAMEWORK_BUNDLE "${_gst_framework_bundle}")
endfunction()
