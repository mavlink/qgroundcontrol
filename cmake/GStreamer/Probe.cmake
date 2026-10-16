# Feature-test helpers for GStreamer headers, symbols and optional gst-plugins-bad libraries, used to gate the HwBuffers
# GPU paths. Probing only: callers register sources so their paths resolve against the caller's
# CMAKE_CURRENT_SOURCE_DIR. Results are cached under QGC_GST_HAS_*, PC_GStreamer_*, GST_D3D* and QGC_GST_VULKAN* names,
# which cmake/GStreamer/SdkState.cmake purges whenever the GStreamer SDK changes.

include_guard(GLOBAL)
include("${CMAKE_CURRENT_LIST_DIR}/Platform.cmake")
include(CheckCXXSourceCompiles)
include(CMakePushCheckState)

# qgc_check_gst_header(VAR <out_var> HEADER <header_path> SYMBOL <symbol> [TARGET <gst_target>] [INCLUDES <dirs>])
# Compile-tests that <header> can be included and <symbol> referenced when linked against <gst_target> (defaults to
# GStreamer::GStreamer, falling back to GStreamerMobile on Android where that's the primary target). Result is set in
# OUT_VAR (TRUE/FALSE in PARENT_SCOPE) and cached under that name, so incremental reconfigures skip the test; an SDK
# change purges it.
function(qgc_check_gst_header)
    cmake_parse_arguments(PARSE_ARGV 0 ARG "" "VAR;HEADER;SYMBOL;TARGET" "INCLUDES")
    _qgc_gst_check_args(qgc_check_gst_header VAR HEADER SYMBOL)

    if(NOT ARG_TARGET)
        if(ANDROID AND TARGET GStreamerMobile)
            set(ARG_TARGET GStreamerMobile)
        else()
            set(ARG_TARGET GStreamer::GStreamer)
        endif()
    endif()

    # cmake-lint: disable=C0103
    cmake_push_check_state(RESET)
    set(CMAKE_REQUIRED_LIBRARIES ${ARG_TARGET})
    # INCLUDES: extra dirs for headers that pull SDK deps not on the default path (e.g. gst/cuda/gstcuda.h -> cuda.h).
    set(CMAKE_REQUIRED_INCLUDES ${ARG_INCLUDES})
    set(CMAKE_REQUIRED_QUIET TRUE)
    check_cxx_source_compiles(
        "
        #include <${ARG_HEADER}>
        int main() { (void)${ARG_SYMBOL}; return 0; }
    "
        ${ARG_VAR}
    )
    cmake_pop_check_state()
    set(${ARG_VAR}
        "${${ARG_VAR}}"
        PARENT_SCOPE
    )
endfunction()

# _qgc_probe_gst_lib(<name> HAS_VAR <cache_var> HEADER <header> SYMBOL <symbol>
#                    [PC_MODULE <module>] [LIBNAMES <names>... LIB_VAR <cache_var>] [INCLUDES <dirs>...])
# Probes one optional GStreamer library:
#   1. pkg-config <module> into PC_GStreamer_<name>_*, rejoining Windows paths that pkg-config split at spaces.
#   2. <header>/<symbol> compile test cached as <HAS_VAR>, with <INCLUDES> plus the module's include dirs.
#   3. With LIBNAMES, find_library into the <LIB_VAR> cache entry and link that absolute path: the bare pkg-config name
#      misses the link path for Windows import libs (LNK1181) and for the macOS framework's gstreamer-vulkan-1.0.pc.
#      Without LIBNAMES the module's own library names and dirs are used, and the module itself is required.
# Sets in PARENT_SCOPE: <name>_FOUND, <name>_LIBRARIES, <name>_INCLUDE_DIRS (<INCLUDES> plus the module's),
# <name>_LIBRARY_DIRS and, when not found, <name>_REASON.
function(_qgc_probe_gst_lib name)
    cmake_parse_arguments(PARSE_ARGV 1 ARG "" "HAS_VAR;HEADER;SYMBOL;PC_MODULE;LIB_VAR" "LIBNAMES;INCLUDES")
    _qgc_gst_check_args(
        _qgc_probe_gst_lib
        HAS_VAR
        HEADER
        SYMBOL
        ALLOW_EMPTY
        PC_MODULE
        LIB_VAR
        LIBNAMES
        INCLUDES
    )
    if(ARG_LIBNAMES AND NOT ARG_LIB_VAR)
        message(FATAL_ERROR "_qgc_probe_gst_lib(${name}): LIBNAMES requires LIB_VAR")
    endif()

    # cmake-lint: disable=C0103
    set(${name}_FOUND
        FALSE
        PARENT_SCOPE
    )
    set(_pc PC_GStreamer_${name})
    if(ARG_PC_MODULE AND PkgConfig_FOUND)
        pkg_check_modules(${_pc} QUIET ${ARG_PC_MODULE})
        _gst_recover_split_pkgconfig_paths(${_pc})
        _gst_coalesce_existing_paths(${_pc}_INCLUDE_DIRS)
        _gst_coalesce_existing_paths(${_pc}_LIBRARY_DIRS)
    endif()

    set(_check_args VAR ${ARG_HAS_VAR} HEADER ${ARG_HEADER} SYMBOL ${ARG_SYMBOL})
    if(ARG_INCLUDES OR ${_pc}_INCLUDE_DIRS)
        list(APPEND _check_args INCLUDES ${ARG_INCLUDES} ${${_pc}_INCLUDE_DIRS})
    endif()
    qgc_check_gst_header(${_check_args})
    if(NOT ${ARG_HAS_VAR})
        set(${name}_REASON
            "${ARG_HEADER} (${ARG_SYMBOL}) not usable"
            PARENT_SCOPE
        )
        return()
    endif()

    set(_libraries ${${_pc}_LIBRARIES})
    if(ARG_LIBNAMES)
        if(DEFINED ${ARG_LIB_VAR} AND NOT EXISTS "${${ARG_LIB_VAR}}")
            unset(${ARG_LIB_VAR} CACHE)
        endif()
        # A bundled SDK (gstreamer_apply_pkgconfig_env MODE SDK) must not lose to another GStreamer in
        # CMAKE_PREFIX_PATH, which find_library searches before HINTS.
        set(_prefix_path_args "")
        if(DEFINED PKG_CONFIG_USE_CMAKE_PREFIX_PATH AND NOT PKG_CONFIG_USE_CMAKE_PREFIX_PATH)
            set(_prefix_path_args NO_CMAKE_PATH NO_CMAKE_ENVIRONMENT_PATH)
        endif()
        find_library(
            ${ARG_LIB_VAR}
            NAMES ${ARG_LIBNAMES}
            HINTS ${${_pc}_LIBRARY_DIRS} "${GSTREAMER_LIB_PATH}" ${_prefix_path_args}
        )
        if(NOT ${ARG_LIB_VAR})
            list(GET ARG_LIBNAMES 0 _lib)
            set(${name}_REASON
                "${_lib} not on the link path"
                PARENT_SCOPE
            )
            return()
        endif()
        set(_libraries "${${ARG_LIB_VAR}}")
    elseif(ARG_PC_MODULE AND NOT ${_pc}_FOUND)
        set(${name}_REASON
            "pkg-config module ${ARG_PC_MODULE} not found"
            PARENT_SCOPE
        )
        return()
    endif()

    set(${name}_FOUND
        TRUE
        PARENT_SCOPE
    )
    set(${name}_LIBRARIES
        "${_libraries}"
        PARENT_SCOPE
    )
    # <INCLUDES> too: the headers may need them (gst/cuda/gstcuda.h pulls <cuda.h>), not just the probe.
    set(_include_dirs ${ARG_INCLUDES} ${${_pc}_INCLUDE_DIRS})
    set(${name}_INCLUDE_DIRS
        "${_include_dirs}"
        PARENT_SCOPE
    )
    set(${name}_LIBRARY_DIRS
        "${${_pc}_LIBRARY_DIRS}"
        PARENT_SCOPE
    )
endfunction()
