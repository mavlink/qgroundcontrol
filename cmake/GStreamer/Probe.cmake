# Feature-test helpers for GStreamer headers/symbols — wraps
# check_cxx_source_compiles for callers that need to gate features on a header
# or symbol being present (HwBuffers, Orchestrator, Linux DMABuf probe).
#
# Also hosts gst-plugins-bad D3D11/D3D12 SDK shim resolution
# (`_qgc_probe_gst_d3d_path`) — pkg-config supplies include/lib dirs, then
# find_library resolves the absolute import lib (the bare pkg-config name doesn't
# reliably land on the MSVC link path). Probing only: source registration is done
# by the caller (HwBuffers) so the d3d/ source paths resolve against HwBuffers'
# CMAKE_CURRENT_SOURCE_DIR, not this module's.

include_guard(GLOBAL)
include(CheckCXXSourceCompiles)
include(CMakePushCheckState)

# qgc_check_gst_header(VAR <out_var> HEADER <header_path> SYMBOL <symbol> [TARGET <gst_target>] [INCLUDES <dirs>])
# Compile-tests that <header> can be included and <symbol> referenced when
# linked against <gst_target> (defaults to GStreamer::GStreamer, falling back
# to GStreamerMobile on Android where that's the primary target).
# Result is set in OUT_VAR (TRUE/FALSE in PARENT_SCOPE) and cached so the test
# isn't re-run on incremental reconfigures (check_cxx_source_compiles caches by
# VAR name). The cache is sticky: a result from a stale toolchain/SDK survives
# until the CMake cache is cleared — clear it after changing the GStreamer SDK.
function(qgc_check_gst_header)
    cmake_parse_arguments(ARG "" "VAR;HEADER;SYMBOL;TARGET" "INCLUDES" ${ARGN})
    foreach(_req IN ITEMS VAR HEADER SYMBOL)
        if(NOT ARG_${_req})
            message(FATAL_ERROR "qgc_check_gst_header: ${_req} is required")
        endif()
    endforeach()

    if(NOT ARG_TARGET)
        if(ANDROID AND TARGET GStreamerMobile)
            set(ARG_TARGET GStreamerMobile)
        else()
            set(ARG_TARGET GStreamer::GStreamer)
        endif()
    endif()

    cmake_push_check_state(RESET)
    set(CMAKE_REQUIRED_LIBRARIES ${ARG_TARGET})
    # INCLUDES: extra dirs for headers that pull SDK deps not on the default path (e.g. gst/cuda/gstcuda.h -> cuda.h).
    set(CMAKE_REQUIRED_INCLUDES ${ARG_INCLUDES})
    set(CMAKE_REQUIRED_QUIET TRUE)
    check_cxx_source_compiles("
        #include <${ARG_HEADER}>
        int main() { (void)${ARG_SYMBOL}; return 0; }
    " ${ARG_VAR})
    cmake_pop_check_state()
    set(${ARG_VAR} "${${ARG_VAR}}" PARENT_SCOPE)
endfunction()

# _qgc_probe_gst_d3d_path(<11|12> <out_prefix>)
# gst_is_d3d{N}_memory exports live in gstd3d{N}-1.0.lib (gst-plugins-bad shared
# helper); pkg-config supplies the search dirs, find_library resolves the absolute
# import lib used for linking. Probing only — on success sets in PARENT_SCOPE:
#   <out_prefix>_FOUND TRUE, <out_prefix>_LIBS, <out_prefix>_INCLUDE_DIRS,
#   <out_prefix>_LIBRARY_DIRS.
# The caller registers the sources (their d3d/ paths must resolve against the
# caller's CMAKE_CURRENT_SOURCE_DIR, not this module's).
function(_qgc_probe_gst_d3d_path VERSION OUT)
    set(${OUT}_FOUND FALSE PARENT_SCOPE)
    set(_pc_var "PC_GStreamer_D3D${VERSION}")
    qgc_check_gst_header(
        VAR    QGC_GST_HAS_D3D${VERSION}
        HEADER gst/d3d${VERSION}/gstd3d${VERSION}.h
        SYMBOL gst_is_d3d${VERSION}_memory)
    set(QGC_GST_HAS_D3D${VERSION} "${QGC_GST_HAS_D3D${VERSION}}" PARENT_SCOPE)
    if(NOT QGC_GST_HAS_D3D${VERSION} OR NOT TARGET Qt6::MultimediaPrivate OR NOT TARGET Qt6::GuiPrivate)
        return()
    endif()

    if(PkgConfig_FOUND)
        pkg_check_modules(${_pc_var} QUIET gstreamer-d3d${VERSION}-1.0)
        _gst_recover_split_pkgconfig_paths(${_pc_var}
            INCLUDE_DIRS CFLAGS_OTHER
            LIBRARY_DIRS LDFLAGS_OTHER
        )
        _gst_coalesce_existing_paths(${_pc_var}_INCLUDE_DIRS)
        _gst_coalesce_existing_paths(${_pc_var}_LIBRARY_DIRS)
    endif()

    # Always resolve the absolute import-lib path and link THAT, not the bare
    # pkg-config name: on Windows the gstreamer-d3d${VERSION}-1.0.pc libdir doesn't put
    # gstd3d${VERSION}-1.0.lib on the MSVC link path, so the bare `gstd3d${VERSION}-1.0`
    # fails at link (LNK1181). find_library locates the lib via the .pc's dirs or
    # GSTREAMER_LIB_PATH; linking the absolute path sidesteps the -L mismatch.
    if(DEFINED GST_D3D${VERSION}_LIB AND
       (GST_D3D${VERSION}_LIB MATCHES "NOTFOUND$" OR NOT EXISTS "${GST_D3D${VERSION}_LIB}"))
        unset(GST_D3D${VERSION}_LIB CACHE)
    endif()
    find_library(GST_D3D${VERSION}_LIB NAMES gstd3d${VERSION}-1.0 gstd3d${VERSION}
        HINTS ${${_pc_var}_LIBRARY_DIRS} "${GSTREAMER_LIB_PATH}")
    if(NOT GST_D3D${VERSION}_LIB)
        message(STATUS "QGCGStreamer: gstd3d${VERSION}-1.0 not on link path - D3D${VERSION} GPU path disabled")
        return()
    endif()
    set(${_pc_var}_LIBRARIES "${GST_D3D${VERSION}_LIB}")

    set(${OUT}_FOUND TRUE PARENT_SCOPE)
    set(${OUT}_LIBS "d3d${VERSION};${${_pc_var}_LIBRARIES}" PARENT_SCOPE)
    set(${OUT}_INCLUDE_DIRS "${${_pc_var}_INCLUDE_DIRS}" PARENT_SCOPE)
    set(${OUT}_LIBRARY_DIRS "${${_pc_var}_LIBRARY_DIRS}" PARENT_SCOPE)
    set(${OUT}_PC_LIBRARIES "${${_pc_var}_LIBRARIES}" PARENT_SCOPE)
endfunction()
