# SDK layout target — captures the on-disk shape of the GStreamer SDK
# (FLAT / FRAMEWORK / XCFRAMEWORK / STATIC_TARBALL) as a single IMPORTED
# INTERFACE target with properties, so consumers can read GSTREAMER_*_DIR
# without depending on PARENT_SCOPE propagation chains.
#
# Properties on GStreamer::Layout (single source of truth):
#   GSTREAMER_LAYOUT_TYPE     - FLAT | FRAMEWORK | XCFRAMEWORK | STATIC_TARBALL
#   GSTREAMER_LIB_DIR         - directory containing libgstreamer-1.0.* and friends
#   GSTREAMER_PLUGIN_DIR      - directory containing libgst<plugin>.* (== LIB_DIR/gstreamer-1.0 for FLAT)
#   GSTREAMER_INCLUDE_DIR     - root of gstreamer-1.0/ + glib-2.0/ headers
#   GSTREAMER_FRAMEWORK_BUNDLE- discovered macOS GStreamer.framework path (set by overlay, optional)
#   GSTREAMER_XCFRAMEWORK_PATH- iOS GStreamer.xcframework path (set when TYPE=XCFRAMEWORK)
#
# Use gstreamer_layout_get(<KEY> <OUT_VAR>) / gstreamer_layout_set(<KEY> <VALUE>)
# to read/write so callers don't replicate the NOTFOUND-handling boilerplate.

include_guard(GLOBAL)

# gstreamer_create_layout_target(SDK_ROOT <root> TYPE <type>
#   [INCLUDE_PATH <path>] [FRAMEWORK_BUNDLE <path>] [XCFRAMEWORK_BUNDLE <path>])
# TYPE: FLAT | FRAMEWORK | XCFRAMEWORK | STATIC_TARBALL
# Sets GSTREAMER_LIB_PATH, GSTREAMER_PLUGIN_PATH, GSTREAMER_INCLUDE_PATH,
# GSTREAMER_FRAMEWORK_PATH (FRAMEWORK only), GSTREAMER_XCFRAMEWORK_PATH (XCFRAMEWORK only)
# in PARENT_SCOPE and creates/updates IMPORTED INTERFACE target GStreamer::Layout.
#
# The FRAMEWORK_BUNDLE / XCFRAMEWORK_BUNDLE keyword names are deliberately
# distinct from their TYPE values — cmake_parse_arguments treats keyword strings
# as reserved at parse time, so reusing "FRAMEWORK" or "XCFRAMEWORK" both as a
# TYPE value and as a sibling keyword would make `TYPE FRAMEWORK FRAMEWORK
# <path>` ambiguous (TYPE's value gets eaten by the keyword recognizer).
function(gstreamer_create_layout_target)
    cmake_parse_arguments(GCLT "" "SDK_ROOT;TYPE;LIB_PATH;INCLUDE_PATH;FRAMEWORK_BUNDLE;XCFRAMEWORK_BUNDLE" "" ${ARGN})

    if(NOT GCLT_SDK_ROOT)
        message(FATAL_ERROR "gstreamer_create_layout_target: SDK_ROOT is required")
    endif()
    if(NOT GCLT_TYPE)
        message(FATAL_ERROR "gstreamer_create_layout_target: TYPE is required")
    endif()

    cmake_path(CONVERT "${GCLT_SDK_ROOT}" TO_CMAKE_PATH_LIST _gclt_root NORMALIZE)
    if(NOT EXISTS "${_gclt_root}")
        message(FATAL_ERROR "GStreamer: SDK not found at '${_gclt_root}' — "
            "check installation or set GStreamer_ROOT_DIR")
    endif()
    set(GStreamer_ROOT_DIR "${_gclt_root}" PARENT_SCOPE)

    if(GCLT_TYPE STREQUAL "XCFRAMEWORK")
        # Slice dir IS the lib root; no lib/ subdir exists.
        set(_gclt_lib     "${_gclt_root}")
        set(_gclt_plugins "${_gclt_root}")
        set(_gclt_include "${GCLT_INCLUDE_PATH}")
        if(NOT _gclt_include)
            set(_gclt_include "${_gclt_root}/Headers")
        endif()
        if(NOT EXISTS "${_gclt_root}")
            message(FATAL_ERROR "GStreamer: xcframework slice dir not found: ${_gclt_root}")
        endif()
        if(GCLT_XCFRAMEWORK_BUNDLE AND NOT EXISTS "${GCLT_XCFRAMEWORK_BUNDLE}")
            message(FATAL_ERROR "GStreamer: xcframework not found at ${GCLT_XCFRAMEWORK_BUNDLE}")
        endif()
        set(GSTREAMER_XCFRAMEWORK_PATH "${GCLT_XCFRAMEWORK_BUNDLE}" PARENT_SCOPE)
    elseif(GCLT_TYPE STREQUAL "FRAMEWORK")
        set(_gclt_lib     "${_gclt_root}/lib")
        set(_gclt_plugins "${_gclt_root}/lib/gstreamer-1.0")
        if(GCLT_INCLUDE_PATH)
            set(_gclt_include "${GCLT_INCLUDE_PATH}")
        else()
            set(_gclt_include "${_gclt_root}/include")
        endif()
        foreach(_p IN ITEMS "${_gclt_root}" "${_gclt_lib}" "${_gclt_include}")
            if(NOT EXISTS "${_p}")
                message(FATAL_ERROR "GStreamer (FRAMEWORK): required path does not exist: ${_p}")
            endif()
        endforeach()
        if(GCLT_FRAMEWORK_BUNDLE)
            set(GSTREAMER_FRAMEWORK_PATH "${GCLT_FRAMEWORK_BUNDLE}" PARENT_SCOPE)
        endif()
    elseif(GCLT_TYPE STREQUAL "FLAT" OR GCLT_TYPE STREQUAL "STATIC_TARBALL")
        if(GCLT_LIB_PATH)
            set(_gclt_lib "${GCLT_LIB_PATH}")
        else()
            set(_gclt_lib "${_gclt_root}/lib")
        endif()
        set(_gclt_plugins "${_gclt_lib}/gstreamer-1.0")
        if(GCLT_INCLUDE_PATH)
            set(_gclt_include "${GCLT_INCLUDE_PATH}")
        else()
            set(_gclt_include "${_gclt_root}/include")
        endif()
        foreach(_p IN ITEMS "${_gclt_root}" "${_gclt_lib}" "${_gclt_include}")
            if(NOT EXISTS "${_p}")
                message(FATAL_ERROR "GStreamer (${GCLT_TYPE}): required path does not exist: ${_p}")
            endif()
        endforeach()
    else()
        message(FATAL_ERROR "gstreamer_create_layout_target: unknown TYPE \'${GCLT_TYPE}\' — must be FLAT, FRAMEWORK, XCFRAMEWORK, or STATIC_TARBALL")
    endif()

    set(GSTREAMER_LIB_PATH     "${_gclt_lib}"     PARENT_SCOPE)
    set(GSTREAMER_PLUGIN_PATH  "${_gclt_plugins}" PARENT_SCOPE)
    set(GSTREAMER_INCLUDE_PATH "${_gclt_include}" PARENT_SCOPE)

    # Target creation is not scriptable (cmake -P) — guard so cmake -P unit tests
    # can exercise scalar-path computation without a full configure context.
    if(NOT CMAKE_SCRIPT_MODE_FILE)
        if(NOT TARGET GStreamer::Layout)
            add_library(GStreamer::Layout INTERFACE IMPORTED GLOBAL)
        endif()
        set_target_properties(GStreamer::Layout PROPERTIES
            GSTREAMER_LAYOUT_TYPE    "${GCLT_TYPE}"
            GSTREAMER_LIB_DIR        "${_gclt_lib}"
            GSTREAMER_PLUGIN_DIR     "${_gclt_plugins}"
            GSTREAMER_INCLUDE_DIR    "${_gclt_include}"
        )
        if(GCLT_TYPE STREQUAL "XCFRAMEWORK" AND GCLT_XCFRAMEWORK_BUNDLE)
            set_target_properties(GStreamer::Layout PROPERTIES
                GSTREAMER_XCFRAMEWORK_PATH "${GCLT_XCFRAMEWORK_BUNDLE}")
        endif()
        if(GCLT_TYPE STREQUAL "FRAMEWORK" AND GCLT_FRAMEWORK_BUNDLE)
            set_target_properties(GStreamer::Layout PROPERTIES
                GSTREAMER_FRAMEWORK_BUNDLE "${GCLT_FRAMEWORK_BUNDLE}")
        endif()
    endif()
endfunction()

# gstreamer_layout_get(<KEY> <OUT_VAR>)
# Read a layout property. Returns empty string when the target doesn't exist
# or the property isn't set, so callers can skip the NOTFOUND-handling dance.
# KEY is the bare name (e.g. FRAMEWORK_BUNDLE); the GSTREAMER_ prefix is added
# automatically.
function(gstreamer_layout_get KEY OUT_VAR)
    if(NOT TARGET GStreamer::Layout)
        set(${OUT_VAR} "" PARENT_SCOPE)
        return()
    endif()
    get_target_property(_val GStreamer::Layout "GSTREAMER_${KEY}")
    if(_val STREQUAL "_val-NOTFOUND" OR NOT _val)
        set(${OUT_VAR} "" PARENT_SCOPE)
    else()
        set(${OUT_VAR} "${_val}" PARENT_SCOPE)
    endif()
endfunction()

# gstreamer_layout_set(<KEY> <VALUE>)
# Write a layout property. KEY is the bare name (e.g. FRAMEWORK_BUNDLE); the
# GSTREAMER_ prefix is added automatically. Skipped silently in cmake -P
# script mode (target creation isn't scriptable).
function(gstreamer_layout_set KEY VALUE)
    if(CMAKE_SCRIPT_MODE_FILE)
        return()
    endif()
    if(NOT TARGET GStreamer::Layout)
        message(FATAL_ERROR "gstreamer_layout_set: GStreamer::Layout target does not exist; call gstreamer_create_layout_target first.")
    endif()
    set_target_properties(GStreamer::Layout PROPERTIES "GSTREAMER_${KEY}" "${VALUE}")
endfunction()
