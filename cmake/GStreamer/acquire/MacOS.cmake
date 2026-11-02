# macOS GStreamer SDK acquisition: the official runtime and devel .pkg files, merged into a flat SDK root.
# Included by Acquire.cmake, whose qgc_gst_acquire_sdk does the download, caching and locking.

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
                            message(
                                FATAL_ERROR "GStreamer: SDK entry '${_entry_name}' symlinks outside the payload: "
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

# _qgc_gst_acquire_macos_sdk(<out_root>)
# Downloads the official macOS runtime and devel packages and merges them into a flat SDK; sets <out_root> to its root.
function(_qgc_gst_acquire_macos_sdk out_root)
    qgc_gst_acquire_sdk(
        PLATFORM macos
        VERSION ${QGC_GST_REQUESTED_VERSION}
        CACHE_SUBDIR "gstreamer-mac-${QGC_GST_REQUESTED_VERSION}"
        ARCHIVES gstreamer.pkg gstreamer-devel.pkg
        CONTENT sdk
        EXTRACT_FUNCTION _qgc_gst_macos_extract
        CONTENT_DIR_OUT _sdk
    )
    set(${out_root}
        "${_sdk}/root"
        PARENT_SCOPE
    )
endfunction()
