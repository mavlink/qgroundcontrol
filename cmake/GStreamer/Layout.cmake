# The SDK layout every platform's discovery derives from the SDK it resolved, and GStreamer::Layout, a GLOBAL target
# whose GSTREAMER_<KEY> properties carry what directories outside the Orchestrator's read: FRAMEWORK_BUNDLE (the macOS
# GStreamer.framework) and HOMEBREW_PREFIX (a Homebrew GStreamer's prefix), both from platform/MacOS.cmake, and
# NOTICE_DIR (Orchestrator.cmake).

include_guard(GLOBAL)

include("${CMAKE_CURRENT_LIST_DIR}/Platform.cmake")

# qgc_gst_create_layout_target(SDK_ROOT <root> [LIB_PATH <dir>] [PLUGIN_PATH <dir>] [INCLUDE_PATH <dir>]
#                                [PLUGIN_SCANNER_PATH <dir>] [GIO_MODULE_PATH <dir>] [FRAMEWORK_BUNDLE <bundle>])
# Sets, in the caller's scope, GStreamer_ROOT_DIR to the normalized <root> and the SDK layout, each defaulting to the
# SDK directory layout: GSTREAMER_LIB_PATH (<root>/lib), GSTREAMER_PLUGIN_PATH (<lib>/gstreamer-1.0),
# GSTREAMER_INCLUDE_PATH (<root>/include), GSTREAMER_PLUGIN_SCANNER_PATH (<root>/libexec/gstreamer-1.0) and
# GSTREAMER_GIO_MODULE_PATH (<lib>/gio/modules). Fails configure when the root, library, plugin or include directory
# does not exist; with FRAMEWORK_BUNDLE also sets GSTREAMER_FRAMEWORK_PATH and the FRAMEWORK_BUNDLE layout property.
# cmake-lint: disable=C0103,E1121
function(qgc_gst_create_layout_target)
    cmake_parse_arguments(
        PARSE_ARGV 0 ARG ""
        "SDK_ROOT;LIB_PATH;PLUGIN_PATH;INCLUDE_PATH;PLUGIN_SCANNER_PATH;GIO_MODULE_PATH;FRAMEWORK_BUNDLE" ""
    )
    _qgc_gst_check_args(qgc_gst_create_layout_target SDK_ROOT)

    cmake_path(SET _root NORMALIZE "${ARG_SDK_ROOT}")
    set(_lib "${_root}/lib")
    if(ARG_LIB_PATH)
        set(_lib "${ARG_LIB_PATH}")
    endif()
    set(_defaults "PLUGIN_PATH|${_lib}/gstreamer-1.0" "INCLUDE_PATH|${_root}/include"
                  "PLUGIN_SCANNER_PATH|${_root}/libexec/gstreamer-1.0" "GIO_MODULE_PATH|${_lib}/gio/modules"
    )
    foreach(_default IN LISTS _defaults)
        string(REPLACE "|" ";" _default "${_default}")
        list(GET _default 0 _key)
        list(GET _default 1 _value)
        if(NOT ARG_${_key})
            set(ARG_${_key} "${_value}")
        endif()
    endforeach()
    foreach(_path IN ITEMS "${_root}" "${_lib}" "${ARG_PLUGIN_PATH}" "${ARG_INCLUDE_PATH}")
        if(NOT EXISTS "${_path}")
            message(
                FATAL_ERROR "GStreamer: SDK path does not exist: ${_path}\n"
                            "Check the installation or set GStreamer_ROOT_DIR."
            )
        endif()
    endforeach()
    set(GStreamer_ROOT_DIR "${_root}")
    set(GSTREAMER_LIB_PATH "${_lib}")
    foreach(_key IN ITEMS PLUGIN_PATH INCLUDE_PATH PLUGIN_SCANNER_PATH GIO_MODULE_PATH)
        set(GSTREAMER_${_key} "${ARG_${_key}}")
    endforeach()
    set(_outputs GStreamer_ROOT_DIR GSTREAMER_LIB_PATH GSTREAMER_PLUGIN_PATH GSTREAMER_INCLUDE_PATH
                 GSTREAMER_PLUGIN_SCANNER_PATH GSTREAMER_GIO_MODULE_PATH
    )
    if(ARG_FRAMEWORK_BUNDLE)
        set(GSTREAMER_FRAMEWORK_PATH "${ARG_FRAMEWORK_BUNDLE}")
        list(APPEND _outputs GSTREAMER_FRAMEWORK_PATH)
        qgc_gst_layout_set(FRAMEWORK_BUNDLE "${ARG_FRAMEWORK_BUNDLE}")
    endif()
    return(PROPAGATE ${_outputs})
endfunction()

# qgc_gst_layout_get(<key> <out_var>)
# Sets <out_var> to the GSTREAMER_<key> property of GStreamer::Layout, or "" when it or the target does not exist.
function(qgc_gst_layout_get key out_var)
    set(_value "")
    if(TARGET GStreamer::Layout)
        get_target_property(_value GStreamer::Layout "GSTREAMER_${key}")
        if(NOT _value)
            set(_value "")
        endif()
    endif()
    set(${out_var}
        "${_value}"
        PARENT_SCOPE
    )
endfunction()

# qgc_gst_layout_set(<key> <value>)
# Sets the GSTREAMER_<key> property of GStreamer::Layout, creating the target; does nothing in script mode.
function(qgc_gst_layout_set key value)
    if(CMAKE_SCRIPT_MODE_FILE)
        return()
    endif()
    if(NOT TARGET GStreamer::Layout)
        add_library(GStreamer::Layout INTERFACE IMPORTED GLOBAL)
    endif()
    set_target_properties(
        GStreamer::Layout
        PROPERTIES
            "GSTREAMER_${key}" "${value}"
    )
endfunction()
