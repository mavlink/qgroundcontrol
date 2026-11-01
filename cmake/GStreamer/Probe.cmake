# Feature-test helpers for GStreamer headers, symbols and optional gst-plugins-bad libraries, used to gate the HwBuffers
# GPU paths. Probing only: callers register sources so their paths resolve against the caller's
# CMAKE_CURRENT_SOURCE_DIR. Results are cached under QGC_GST_HAS_* (compile tests) and PC_GStreamer_* (pkg-config)
# names, which cmake/GStreamer/SdkState.cmake purges whenever the GStreamer SDK changes.

include_guard(GLOBAL)

include("${CMAKE_CURRENT_LIST_DIR}/Layout.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/Platform.cmake")
include(CheckCXXSourceCompiles)
include(CMakePushCheckState)

# qgc_gst_check_header(VAR <out_var> HEADER <header_path> SYMBOL <symbol> [TARGET <gst_target>] [INCLUDES <dirs>])
# Compile-tests that <header> can be included and <symbol> referenced when linked against <gst_target> (defaults to
# GStreamer::GStreamer, or on Android GStreamer::Static, the libraries GStreamer::mobile is built from). Result is set
# in OUT_VAR (TRUE/FALSE in PARENT_SCOPE) and cached under that name, which must start with QGC_GST_HAS_, so incremental
# reconfigures skip the test; an SDK change purges it.
function(qgc_gst_check_header)
    cmake_parse_arguments(PARSE_ARGV 0 ARG "" "VAR;HEADER;SYMBOL;TARGET" "INCLUDES")
    _qgc_gst_check_args(qgc_gst_check_header VAR HEADER SYMBOL)

    if(NOT ARG_TARGET)
        if(ANDROID)
            set(ARG_TARGET GStreamer::Static)
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

# qgc_gst_probe_library(<name> HAS_VAR <cache_var> HEADER <header> SYMBOL <symbol>
#                    [PC_MODULE <module>] [LIBNAMES <names>... LIB_VAR <var>] [INCLUDES <dirs>...])
# Probes one optional GStreamer library:
#   1. pkg-config <module> into PkgConfig::PC_GStreamer_<name>.
#   2. <header>/<symbol> compile test cached as <HAS_VAR>, with <INCLUDES> plus the module's include dirs.
#   3. With LIBNAMES, find_library (uncached, set in <LIB_VAR>) in the module's library dirs and the SDK's and link only
#      that library: the macOS framework's gstreamer-vulkan-1.0.pc names a library the framework does not ship, which
#      leaves the probe unusable instead of failing the link. Without LIBNAMES the module's imported target is linked,
#      and the module itself is required.
# Sets in PARENT_SCOPE: <name>_FOUND, <name>_LIBRARIES, <name>_INCLUDE_DIRS (<INCLUDES> plus the module's),
# <name>_LIBRARY_DIRS and, when not found, <name>_REASON.
function(qgc_gst_probe_library name)
    qgc_gst_layout_get(LIB_PATH _gst_lib_path)
    cmake_parse_arguments(PARSE_ARGV 1 ARG "" "HAS_VAR;HEADER;SYMBOL;PC_MODULE;LIB_VAR" "LIBNAMES;INCLUDES")
    _qgc_gst_check_args(qgc_gst_probe_library HAS_VAR HEADER SYMBOL ALLOW_EMPTY PC_MODULE LIB_VAR LIBNAMES INCLUDES)
    if(ARG_LIBNAMES AND NOT ARG_LIB_VAR)
        message(FATAL_ERROR "qgc_gst_probe_library(${name}): LIBNAMES requires LIB_VAR")
    endif()

    # cmake-lint: disable=C0103
    set(${name}_FOUND
        FALSE
        PARENT_SCOPE
    )
    set(_pc PC_GStreamer_${name})
    if(ARG_PC_MODULE AND PkgConfig_FOUND)
        pkg_check_modules(${_pc} QUIET IMPORTED_TARGET GLOBAL ${ARG_PC_MODULE})
    endif()

    set(_check_args VAR ${ARG_HAS_VAR} HEADER ${ARG_HEADER} SYMBOL ${ARG_SYMBOL})
    if(ARG_INCLUDES OR ${_pc}_INCLUDE_DIRS)
        list(APPEND _check_args INCLUDES ${ARG_INCLUDES} ${${_pc}_INCLUDE_DIRS})
    endif()
    qgc_gst_check_header(${_check_args})
    if(NOT ${ARG_HAS_VAR})
        set(${name}_REASON
            "${ARG_HEADER} (${ARG_SYMBOL}) not usable"
            PARENT_SCOPE
        )
        return()
    endif()

    set(_libraries "")
    if(TARGET PkgConfig::${_pc})
        set(_libraries PkgConfig::${_pc})
    endif()
    if(ARG_LIBNAMES)
        # A bundled SDK (qgc_gst_apply_pkgconfig_env MODE SDK) must not lose to another GStreamer in
        # CMAKE_PREFIX_PATH, which find_library searches before HINTS.
        set(_prefix_path_args "")
        if(DEFINED PKG_CONFIG_USE_CMAKE_PREFIX_PATH AND NOT PKG_CONFIG_USE_CMAKE_PREFIX_PATH)
            set(_prefix_path_args NO_CMAKE_PATH NO_CMAKE_ENVIRONMENT_PATH)
        endif()
        # find_library skips the search while a cache entry (from configures before NO_CACHE) holds the name.
        unset(${ARG_LIB_VAR} CACHE)
        find_library(
            ${ARG_LIB_VAR}
            NAMES ${ARG_LIBNAMES}
            HINTS ${${_pc}_LIBRARY_DIRS} "${_gst_lib_path}" ${_prefix_path_args} NO_CACHE
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
