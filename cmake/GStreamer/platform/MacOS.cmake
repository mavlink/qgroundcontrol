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

# _qgc_gst_discover_macos_sdk()
# Uses the SDK under GStreamer_ROOT_DIR, else /Library/Frameworks/GStreamer.framework or Homebrew's gstreamer, else
# downloads the official runtime and devel packages. Points pkg-config at the SDK. Sets GStreamer_ROOT_DIR,
# GStreamer_USE_FRAMEWORK, GStreamer_AUTO_DOWNLOADED, GSTREAMER_{LIB,PLUGIN,INCLUDE,FRAMEWORK}_PATH and, for the
# framework, the SDK's PKG_CONFIG_EXECUTABLE; a flat SDK uses Homebrew's pkg-config (cached PKG_CONFIG_EXECUTABLE).
# cmakelang 0.6.13 predates return(PROPAGATE).
# cmake-lint: disable=C0103,E1121
function(_qgc_gst_discover_macos_sdk)
    if(NOT GStreamer_ROOT_DIR)
        if(EXISTS "/Library/Frameworks/GStreamer.framework")
            set(GStreamer_ROOT_DIR "/Library/Frameworks/GStreamer.framework/Versions/1.0")
        else()
            foreach(_brew_prefix IN ITEMS "/opt/homebrew/opt/gstreamer" "/usr/local/opt/gstreamer")
                if(EXISTS "${_brew_prefix}")
                    set(GStreamer_ROOT_DIR "${_brew_prefix}")
                    set(GStreamer_USE_FRAMEWORK OFF)
                    message(STATUS "GStreamer: Using Homebrew at ${GStreamer_ROOT_DIR}")
                    break()
                endif()
            endforeach()
        endif()
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

    # User-supplied GStreamer_ROOT_DIR skips detection above; re-derive flat-vs-framework from the layout so a
    # flat (Homebrew/CPM) tree doesn't hit the framework branch and FATAL_ERROR.
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
        gstreamer_create_layout_target(SDK_ROOT "${GStreamer_ROOT_DIR}" TYPE FRAMEWORK FRAMEWORK_BUNDLE
                                       "${_gst_mac_fwpath}"
        )
    else()
        gstreamer_create_layout_target(SDK_ROOT "${GStreamer_ROOT_DIR}" TYPE FLAT)
    endif()

    set(_outputs
        GStreamer_ROOT_DIR
        GStreamer_USE_FRAMEWORK
        GStreamer_AUTO_DOWNLOADED
        GSTREAMER_LIB_PATH
        GSTREAMER_PLUGIN_PATH
        GSTREAMER_INCLUDE_PATH
        GSTREAMER_FRAMEWORK_PATH
    )
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
        list(APPEND _outputs PKG_CONFIG_EXECUTABLE)
    else()
        _qgc_gst_find_apple_pkg_config(PKG_CONFIG_EXECUTABLE)
        gstreamer_apply_pkgconfig_env(MODE SDK LIBDIR "${GSTREAMER_LIB_PATH}/pkgconfig"
                                      "${GSTREAMER_PLUGIN_PATH}/pkgconfig"
        )
    endif()
    return(PROPAGATE ${_outputs})
endfunction()

# Apple framework overlay — when GStreamer.framework is in use, link the framework
# directly and add its Headers to the include path. Called from the pkg-config
# branch only (iOS uses xcframework which bypasses this entirely).
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
