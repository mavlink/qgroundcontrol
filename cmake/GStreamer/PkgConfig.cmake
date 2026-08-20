# Unified pkg-config env management for GStreamer discovery — single mutator
# for PKG_CONFIG_PATH / PKG_CONFIG_LIBDIR / PKG_CONFIG_DONT_DEFINE_PREFIX,
# used by FindGStreamer.cmake (SDK fallback), platform/Linux.cmake
# (system-augment), and platform/PkgConfigTargets.cmake (SDK lock-in for
# Windows/macOS/Android).

include_guard(GLOBAL)

# gstreamer_apply_pkgconfig_env(
#     MODE <SDK|SYSTEM_AUGMENT>
#     LIBDIR <dir>...                 # one or more pkgconfig dirs (semicolon list)
#     [PKG_CONFIG_EXE <path>]         # optional explicit pkg-config binary
#     [DONT_DEFINE_PREFIX]            # appends --dont-define-prefix to PKG_CONFIG_ARGN
# )
#
# MODE SDK             — lock pkg-config to the SDK: PKG_CONFIG_LIBDIR=<libdirs>,
#                        PKG_CONFIG_PATH cleared. System .pc files invisible.
# MODE SYSTEM_AUGMENT  — prepend <libdirs> to PKG_CONFIG_PATH so SDK .pc files
#                        win but system glib/gobject .pc files stay discoverable.
#                        Used by Linux distro-installed GStreamer.
function(gstreamer_apply_pkgconfig_env)
    cmake_parse_arguments(PARSE_ARGV 0 ARG "DONT_DEFINE_PREFIX" "MODE;PKG_CONFIG_EXE" "LIBDIR")
    if(ARG_KEYWORDS_MISSING_VALUES)
        message(FATAL_ERROR
            "gstreamer_apply_pkgconfig_env: missing values for: ${ARG_KEYWORDS_MISSING_VALUES}")
    endif()
    if(ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR
            "gstreamer_apply_pkgconfig_env: unknown arguments: ${ARG_UNPARSED_ARGUMENTS}")
    endif()
    if(NOT ARG_MODE)
        message(FATAL_ERROR "gstreamer_apply_pkgconfig_env: MODE is required (SDK or SYSTEM_AUGMENT)")
    endif()
    if(NOT ARG_LIBDIR)
        message(FATAL_ERROR "gstreamer_apply_pkgconfig_env: LIBDIR is required")
    endif()

    if(ARG_PKG_CONFIG_EXE)
        set(ENV{PKG_CONFIG} "${ARG_PKG_CONFIG_EXE}")
        set(PKG_CONFIG_EXECUTABLE "${ARG_PKG_CONFIG_EXE}" CACHE FILEPATH "pkg-config executable" FORCE)
    endif()

    if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
        set(_sep ";")
    else()
        set(_sep ":")
    endif()
    string(REPLACE ";" "${_sep}" _libdir_str "${ARG_LIBDIR}")

    if(ARG_MODE STREQUAL "SDK")
        set(ENV{PKG_CONFIG_PATH} "")
        set(ENV{PKG_CONFIG_LIBDIR} "${_libdir_str}")
    elseif(ARG_MODE STREQUAL "SYSTEM_AUGMENT")
        set(_pkg_config_paths ${ARG_LIBDIR})
        if(DEFINED ENV{PKG_CONFIG_PATH} AND NOT "$ENV{PKG_CONFIG_PATH}" STREQUAL "")
            string(REPLACE "${_sep}" ";" _existing_pkg_config_paths "$ENV{PKG_CONFIG_PATH}")
            list(APPEND _pkg_config_paths ${_existing_pkg_config_paths})
        endif()
        list(REMOVE_DUPLICATES _pkg_config_paths)
        string(REPLACE ";" "${_sep}" _pkg_config_path "${_pkg_config_paths}")
        set(ENV{PKG_CONFIG_PATH} "${_pkg_config_path}")
    else()
        message(FATAL_ERROR "gstreamer_apply_pkgconfig_env: invalid MODE='${ARG_MODE}' (expected SDK or SYSTEM_AUGMENT)")
    endif()

    if(ARG_DONT_DEFINE_PREFIX AND NOT "--dont-define-prefix" IN_LIST PKG_CONFIG_ARGN)
        list(APPEND PKG_CONFIG_ARGN --dont-define-prefix)
        set(PKG_CONFIG_ARGN "${PKG_CONFIG_ARGN}" PARENT_SCOPE)
    endif()
endfunction()
