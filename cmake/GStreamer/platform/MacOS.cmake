# macOS GStreamer SDK discovery — invoked by Orchestrator.cmake.

include_guard(GLOBAL)

# _qgc_gst_macos_extract(<staging> <runtime_pkg;devel_pkg>)
# qgc_gst_acquire_sdk extractor: expands both .pkg files into <staging> and merges their payloads
# into the flat SDK root <staging>/root.
function(_qgc_gst_macos_extract staging archives)
    list(GET archives 0 _runtime_pkg)
    list(GET archives 1 _devel_pkg)
    set(_root "${staging}/root")
    file(MAKE_DIRECTORY "${_root}")
    _qgc_gst_pkgutil_expand_and_validate("${_runtime_pkg}" "${staging}/expanded" "macOS runtime")
    _qgc_gst_pkgutil_expand_and_validate("${_devel_pkg}" "${staging}/expanded-devel" "macOS devel")

    foreach(_expanded_dir IN ITEMS "${staging}/expanded" "${staging}/expanded-devel")
        file(GLOB _sub_pkg_dirs "${_expanded_dir}/*.pkg")
        foreach(_pkg_dir IN LISTS _sub_pkg_dirs)
            if(EXISTS "${_pkg_dir}/Payload")
                file(GLOB _payload_entries "${_pkg_dir}/Payload/*")
                foreach(_entry IN LISTS _payload_entries)
                    cmake_path(GET _entry FILENAME _entry_name)
                    if(_entry_name STREQUAL "Headers")
                        continue()
                    endif()
                    cmake_path(IS_PREFIX _root "${_root}/${_entry_name}" NORMALIZE _is_safe)
                    if(NOT _is_safe)
                        message(FATAL_ERROR "GStreamer: Path traversal detected in extracted SDK entry: ${_entry_name}")
                    endif()
                    if(IS_SYMLINK "${_entry}")
                        get_filename_component(_link_target "${_entry}" REALPATH)
                        cmake_path(IS_PREFIX _expanded_dir "${_link_target}" NORMALIZE _link_safe)
                        if(NOT _link_safe)
                            message(FATAL_ERROR "GStreamer: SDK entry '${_entry_name}' symlinks outside the payload: "
                                                "${_link_target}"
                            )
                        endif()
                    endif()
                    file(COPY "${_entry}" DESTINATION "${_root}")
                endforeach()
            endif()
        endforeach()
    endforeach()

    foreach(_required IN ITEMS lib/gstreamer-1.0 include/gstreamer-1.0 lib/pkgconfig/gstreamer-1.0.pc)
        if(NOT EXISTS "${_root}/${_required}")
            message(
                FATAL_ERROR
                    "Downloaded macOS GStreamer SDK is incomplete (${_required} not found).\n"
                    "Install manually from https://gstreamer.freedesktop.org/download/ or set GStreamer_ROOT_DIR."
            )
        endif()
    endforeach()
endfunction()

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
    message(FATAL_ERROR "GStreamer: QGC_GST_USE_HOMEBREW is ON but Homebrew's gstreamer is not in any of: ${_kegs}\n"
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
        message(FATAL_ERROR "GStreamer: Homebrew's GStreamer needs Homebrew's pkg-config. Install it with: "
                            "brew install pkgconf"
        )
    endif()
    message(STATUS "GStreamer: Using Homebrew's GStreamer at ${GStreamer_ROOT_DIR} (builds only; installs and "
                   "packages need the official SDK)"
    )
    set(GStreamer_USE_FRAMEWORK OFF)
    gstreamer_create_layout_target(SDK_ROOT "${GStreamer_ROOT_DIR}")
    gstreamer_apply_pkgconfig_env(
        MODE
        SYSTEM_AUGMENT
        PKG_CONFIG_EXE
        "${prefix}/bin/pkg-config"
        LIBDIR
        "${GSTREAMER_LIB_PATH}/pkgconfig"
        "${prefix}/lib/pkgconfig"
        "${prefix}/opt/glib/lib/pkgconfig"
    )
    gstreamer_layout_set(HOMEBREW_PREFIX "${prefix}")
    _qgc_gst_return_discovery()
endfunction()

# _qgc_gst_discover_macos_sdk()
# Uses the SDK under GStreamer_ROOT_DIR; else, with QGC_GST_USE_HOMEBREW, Homebrew's gstreamer; else an earlier
# auto-downloaded SDK, /Library/Frameworks/GStreamer.framework, or a download of the official runtime and devel
# packages. A root in a Homebrew prefix goes to _qgc_gst_discover_homebrew_sdk. A framework brings its own pkg-config
# and sets GSTREAMER_FRAMEWORK_PATH; another flat SDK uses Homebrew's pkg-config (cached PKG_CONFIG_EXECUTABLE).
# cmake-lint: disable=C0103
function(_qgc_gst_discover_macos_sdk)
    if(QGC_GST_USE_HOMEBREW AND (NOT GStreamer_ROOT_DIR OR GStreamer_AUTO_DOWNLOADED))
        _qgc_gst_find_homebrew_gstreamer(GStreamer_ROOT_DIR)
        set(GStreamer_AUTO_DOWNLOADED FALSE)
    elseif(NOT GStreamer_ROOT_DIR AND EXISTS "/Library/Frameworks/GStreamer.framework")
        set(GStreamer_ROOT_DIR "/Library/Frameworks/GStreamer.framework/Versions/1.0")
    endif()

    if(NOT GStreamer_ROOT_DIR OR NOT EXISTS "${GStreamer_ROOT_DIR}")
        qgc_gst_acquire_sdk(
            PLATFORM
            macos
            VERSION
            ${QGC_GST_REQUESTED_VERSION}
            CACHE_SUBDIR
            "gstreamer-mac-${QGC_GST_REQUESTED_VERSION}"
            ARCHIVES
            gstreamer.pkg
            gstreamer-devel.pkg
            CONTENT
            sdk
            EXTRACT_FUNCTION
            _qgc_gst_macos_extract
            CONTENT_DIR_OUT
            _gst_mac_sdk
        )
        set(GStreamer_ROOT_DIR "${_gst_mac_sdk}/root")
        set(GStreamer_USE_FRAMEWORK OFF)
        set(GStreamer_AUTO_DOWNLOADED TRUE)
    endif()

    _qgc_gst_homebrew_prefix("${GStreamer_ROOT_DIR}" _brew_prefix)
    if(_brew_prefix)
        _qgc_gst_discover_homebrew_sdk("${_brew_prefix}")
        _qgc_gst_return_discovery()
    endif()

    # User-supplied GStreamer_ROOT_DIR skips detection above; re-derive flat-vs-framework from the layout so a
    # flat (CPM) tree doesn't hit the framework branch and FATAL_ERROR.
    if(GStreamer_USE_FRAMEWORK
       AND NOT GStreamer_ROOT_DIR MATCHES "\\.framework"
       AND EXISTS "${GStreamer_ROOT_DIR}/lib/gstreamer-1.0"
    )
        message(STATUS "GStreamer: ${GStreamer_ROOT_DIR} is a flat SDK layout; disabling framework mode")
        set(GStreamer_USE_FRAMEWORK OFF)
    endif()

    if(GStreamer_USE_FRAMEWORK)
        if(NOT DEFINED GSTREAMER_FRAMEWORK_PATH)
            cmake_path(SET _gst_mac_fwpath NORMALIZE "${GStreamer_ROOT_DIR}/../..")
        else()
            set(_gst_mac_fwpath "${GSTREAMER_FRAMEWORK_PATH}")
        endif()
        gstreamer_create_layout_target(SDK_ROOT "${GStreamer_ROOT_DIR}" FRAMEWORK_BUNDLE "${_gst_mac_fwpath}")
    else()
        gstreamer_create_layout_target(SDK_ROOT "${GStreamer_ROOT_DIR}")
    endif()

    if(GStreamer_USE_FRAMEWORK)
        gstreamer_apply_pkgconfig_env(
            MODE
            SDK
            PKG_CONFIG_EXE
            "${GStreamer_ROOT_DIR}/bin/pkg-config"
            LIBDIR
            "${GSTREAMER_LIB_PATH}/pkgconfig"
            "${GSTREAMER_PLUGIN_PATH}/pkgconfig"
        )
    else()
        _qgc_gst_find_apple_pkg_config(PKG_CONFIG_EXECUTABLE)
        gstreamer_apply_pkgconfig_env(MODE SDK LIBDIR "${GSTREAMER_LIB_PATH}/pkgconfig"
                                      "${GSTREAMER_PLUGIN_PATH}/pkgconfig"
        )
    endif()
    _qgc_gst_return_discovery()
endfunction()

# Apple framework overlay — when GStreamer.framework is in use, link the framework
# directly and add its Headers to the include path. Called from the pkg-config
# branch only (iOS uses its merged archive, which bypasses this entirely).
function(_qgc_gst_apply_macos_framework_overlay)
    if(NOT GSTREAMER_FRAMEWORK_PATH)
        message(FATAL_ERROR "_qgc_gst_apply_macos_framework_overlay requires GSTREAMER_FRAMEWORK_PATH; "
                            "ensure gstreamer_create_layout_target(TYPE FRAMEWORK FRAMEWORK_BUNDLE ...) ran first."
        )
    endif()
    set(CMAKE_FIND_FRAMEWORK ONLY)
    cmake_path(GET GSTREAMER_FRAMEWORK_PATH PARENT_PATH _gst_framework_parent)
    find_library(_gst_framework_bundle GStreamer PATHS "${_gst_framework_parent}" "${GSTREAMER_FRAMEWORK_PATH}"
                                                       "/Library/Frameworks"
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
