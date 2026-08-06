# macOS GStreamer SDK discovery — invoked by Orchestrator.cmake.

macro(_qgc_discover_macos_sdk)
    if(NOT DEFINED GStreamer_ROOT_DIR)
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

    if(NOT DEFINED GStreamer_ROOT_DIR OR NOT EXISTS "${GStreamer_ROOT_DIR}")
        gstreamer_resolve_or_download_sdk(
            PLATFORM         macos
            CACHE_SUBDIR     "gstreamer-mac-${GStreamer_FIND_VERSION}"
            FILENAME_PRIMARY "gstreamer.pkg"
            FILENAME_SECONDARY "gstreamer-devel.pkg"
            CACHE_DIR_OUT    _gst_mac_cache_dir
            ARCHIVE_OUT      _gst_mac_pkg
            ARCHIVE2_OUT     _gst_mac_devel_pkg
        )
        set(_gst_mac_expanded "${_gst_mac_cache_dir}/expanded")
        set(_gst_mac_devel_expanded "${_gst_mac_cache_dir}/expanded-devel")
        set(_gst_mac_root "${_gst_mac_cache_dir}/root")
        set(_gst_mac_required_plugin_dir "${_gst_mac_root}/lib/gstreamer-1.0")
        set(_gst_mac_required_include_dir "${_gst_mac_root}/include/gstreamer-1.0")
        set(_gst_mac_required_pc_file "${_gst_mac_root}/lib/pkgconfig/gstreamer-1.0.pc")

        if(EXISTS "${_gst_mac_root}/.merge_complete")
            if(NOT EXISTS "${_gst_mac_required_plugin_dir}"
               OR NOT EXISTS "${_gst_mac_required_include_dir}"
               OR NOT EXISTS "${_gst_mac_required_pc_file}")
                message(STATUS "GStreamer: cached macOS SDK is incomplete; rebuilding cache")
                file(REMOVE_RECURSE "${_gst_mac_root}" "${_gst_mac_expanded}" "${_gst_mac_devel_expanded}")
            endif()
        endif()

        if(NOT EXISTS "${_gst_mac_root}/.merge_complete")
            file(REMOVE_RECURSE "${_gst_mac_root}")
            file(REMOVE_RECURSE "${_gst_mac_expanded}" "${_gst_mac_devel_expanded}")

            _qgc_pkgutil_expand_and_validate("${_gst_mac_pkg}" "${_gst_mac_expanded}" "macOS runtime")
            _qgc_pkgutil_expand_and_validate("${_gst_mac_devel_pkg}" "${_gst_mac_devel_expanded}" "macOS devel")

            file(MAKE_DIRECTORY "${_gst_mac_root}")
            foreach(_expanded_dir IN ITEMS "${_gst_mac_expanded}" "${_gst_mac_devel_expanded}")
                file(GLOB _sub_pkg_dirs "${_expanded_dir}/*.pkg")
                foreach(_pkg_dir IN LISTS _sub_pkg_dirs)
                    if(EXISTS "${_pkg_dir}/Payload")
                        file(GLOB _payload_entries "${_pkg_dir}/Payload/*")
                        foreach(_entry IN LISTS _payload_entries)
                            cmake_path(GET _entry FILENAME _entry_name)
                            if(_entry_name STREQUAL "Headers")
                                continue()
                            endif()
                            cmake_path(IS_PREFIX _gst_mac_root "${_gst_mac_root}/${_entry_name}" NORMALIZE _is_safe)
                            if(NOT _is_safe)
                                message(FATAL_ERROR "GStreamer: Path traversal detected in extracted SDK entry: ${_entry_name}")
                            endif()
                            if(IS_SYMLINK "${_entry}")
                                get_filename_component(_link_target "${_entry}" REALPATH)
                                cmake_path(IS_PREFIX _expanded_dir "${_link_target}" NORMALIZE _link_safe)
                                if(NOT _link_safe)
                                    message(FATAL_ERROR "GStreamer: SDK entry '${_entry_name}' symlinks outside the payload: ${_link_target}")
                                endif()
                            endif()
                            file(COPY "${_entry}" DESTINATION "${_gst_mac_root}")
                        endforeach()
                    endif()
                endforeach()
            endforeach()
            file(TOUCH "${_gst_mac_root}/.merge_complete")
        endif()

        if(NOT EXISTS "${_gst_mac_required_plugin_dir}"
           OR NOT EXISTS "${_gst_mac_required_include_dir}"
           OR NOT EXISTS "${_gst_mac_required_pc_file}")
            file(REMOVE_RECURSE "${_gst_mac_root}" "${_gst_mac_expanded}" "${_gst_mac_devel_expanded}")
            message(FATAL_ERROR "Downloaded macOS GStreamer SDK is incomplete "
                "(required runtime/devel artifacts were not found).\n"
                "Install manually from https://gstreamer.freedesktop.org/download/ or set GStreamer_ROOT_DIR.")
        endif()

        set(GStreamer_ROOT_DIR "${_gst_mac_root}")
        set(GStreamer_USE_FRAMEWORK OFF)
        set(GStreamer_AUTO_DOWNLOADED TRUE)
    endif()

    # User-supplied GStreamer_ROOT_DIR skips detection above; re-derive flat-vs-framework from the layout so a
    # flat (Homebrew/CPM) tree doesn't hit the framework branch and FATAL_ERROR.
    if(GStreamer_USE_FRAMEWORK AND NOT GStreamer_ROOT_DIR MATCHES "\\.framework"
       AND EXISTS "${GStreamer_ROOT_DIR}/lib/gstreamer-1.0")
        message(STATUS "GStreamer: ${GStreamer_ROOT_DIR} is a flat SDK layout; disabling framework mode")
        set(GStreamer_USE_FRAMEWORK OFF)
    endif()

    if(GStreamer_USE_FRAMEWORK)
        if(NOT DEFINED GSTREAMER_FRAMEWORK_PATH)
            cmake_path(CONVERT "${GStreamer_ROOT_DIR}/../.." TO_CMAKE_PATH_LIST _gst_mac_fwpath NORMALIZE)
        else()
            set(_gst_mac_fwpath "${GSTREAMER_FRAMEWORK_PATH}")
        endif()
        gstreamer_create_layout_target(
            SDK_ROOT         "${GStreamer_ROOT_DIR}"
            TYPE             FRAMEWORK
            FRAMEWORK_BUNDLE "${_gst_mac_fwpath}"
        )
    else()
        gstreamer_create_layout_target(
            SDK_ROOT "${GStreamer_ROOT_DIR}"
            TYPE     FLAT
        )
    endif()

    if(GStreamer_USE_FRAMEWORK)
        gstreamer_apply_pkgconfig_env(
            MODE SDK
            PKG_CONFIG_EXE "${GStreamer_ROOT_DIR}/bin/pkg-config"
            LIBDIR "${GSTREAMER_LIB_PATH}/pkgconfig" "${GSTREAMER_PLUGIN_PATH}/pkgconfig"
        )
    else()
        _qgc_find_apple_pkg_config(PKG_CONFIG_EXECUTABLE)
        gstreamer_apply_pkgconfig_env(
            MODE SDK
            LIBDIR "${GSTREAMER_LIB_PATH}/pkgconfig" "${GSTREAMER_PLUGIN_PATH}/pkgconfig"
        )
    endif()
endmacro()

# Apple framework overlay — when GStreamer.framework is in use, link the framework
# directly and add its Headers to the include path. Called from the pkg-config
# branch only (iOS uses xcframework which bypasses this entirely).
# Converted from macro to function in PR5: only mutates GLOBAL imported targets
# and the layout target, so PARENT_SCOPE leakage isn't needed.
function(_qgc_apply_macos_framework_overlay)
    if(NOT GSTREAMER_FRAMEWORK_PATH)
        message(FATAL_ERROR
            "_qgc_apply_macos_framework_overlay requires GSTREAMER_FRAMEWORK_PATH; "
            "ensure gstreamer_create_layout_target(TYPE FRAMEWORK FRAMEWORK_BUNDLE ...) ran first.")
    endif()
    set(CMAKE_FIND_FRAMEWORK ONLY)
    cmake_path(GET GSTREAMER_FRAMEWORK_PATH PARENT_PATH _gst_framework_parent)
    find_library(_gst_framework_bundle GStreamer
        PATHS
            "${_gst_framework_parent}"
            "${GSTREAMER_FRAMEWORK_PATH}"
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
