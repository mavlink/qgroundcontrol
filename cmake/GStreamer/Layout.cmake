# The SDK layout every platform's discovery derives from the SDK it resolved, and GStreamer::Layout, a GLOBAL target
# whose GSTREAMER_<KEY> properties carry what directories outside the Orchestrator's read: FRAMEWORK_BUNDLE (the macOS
# GStreamer.framework) and HOMEBREW_PREFIX (a Homebrew GStreamer's prefix), both from platform/MacOS.cmake, and
# NOTICE_DIR (Orchestrator.cmake).

include_guard(GLOBAL)

include("${CMAKE_CURRENT_LIST_DIR}/Platform.cmake")

# gstreamer_create_layout_target(SDK_ROOT <root> [LIB_PATH <dir>] [FRAMEWORK_BUNDLE <bundle>])
# Sets, in the caller's scope, GStreamer_ROOT_DIR to the normalized <root>, GSTREAMER_LIB_PATH (default <root>/lib),
# GSTREAMER_PLUGIN_PATH (<lib>/gstreamer-1.0) and GSTREAMER_INCLUDE_PATH (<root>/include), failing configure when one
# does not exist; with FRAMEWORK_BUNDLE also GSTREAMER_FRAMEWORK_PATH and the FRAMEWORK_BUNDLE layout property.
function(gstreamer_create_layout_target)
    cmake_parse_arguments(PARSE_ARGV 0 ARG "" "SDK_ROOT;LIB_PATH;FRAMEWORK_BUNDLE" "")
    _qgc_gst_check_args(gstreamer_create_layout_target SDK_ROOT)

    cmake_path(SET _root NORMALIZE "${ARG_SDK_ROOT}")
    set(_lib "${_root}/lib")
    if(ARG_LIB_PATH)
        set(_lib "${ARG_LIB_PATH}")
    endif()
    foreach(_path IN ITEMS "${_root}" "${_lib}" "${_lib}/gstreamer-1.0" "${_root}/include")
        if(NOT EXISTS "${_path}")
            message(FATAL_ERROR "GStreamer: SDK path does not exist: ${_path}\n"
                                "Check the installation or set GStreamer_ROOT_DIR."
            )
        endif()
    endforeach()
    set(GStreamer_ROOT_DIR
        "${_root}"
        PARENT_SCOPE
    )
    set(GSTREAMER_LIB_PATH
        "${_lib}"
        PARENT_SCOPE
    )
    set(GSTREAMER_PLUGIN_PATH
        "${_lib}/gstreamer-1.0"
        PARENT_SCOPE
    )
    set(GSTREAMER_INCLUDE_PATH
        "${_root}/include"
        PARENT_SCOPE
    )
    if(ARG_FRAMEWORK_BUNDLE)
        set(GSTREAMER_FRAMEWORK_PATH
            "${ARG_FRAMEWORK_BUNDLE}"
            PARENT_SCOPE
        )
        gstreamer_layout_set(FRAMEWORK_BUNDLE "${ARG_FRAMEWORK_BUNDLE}")
    endif()
endfunction()

# gstreamer_layout_get(<key> <out_var>)
# Sets <out_var> to the GSTREAMER_<key> property of GStreamer::Layout, or "" when it or the target does not exist.
function(gstreamer_layout_get key out_var)
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

# gstreamer_layout_set(<key> <value>)
# Sets the GSTREAMER_<key> property of GStreamer::Layout, creating the target; does nothing in script mode.
function(gstreamer_layout_set key value)
    if(CMAKE_SCRIPT_MODE_FILE)
        return()
    endif()
    if(NOT TARGET GStreamer::Layout)
        add_library(GStreamer::Layout INTERFACE IMPORTED GLOBAL)
    endif()
    set_target_properties(GStreamer::Layout PROPERTIES "GSTREAMER_${key}" "${value}")
endfunction()
