# Unified pkg-config env management for GStreamer discovery — single mutator
# for PKG_CONFIG_PATH / PKG_CONFIG_LIBDIR / --dont-define-prefix, used by the
# platform discovery functions: platform/Linux.cmake (system-augment) and
# platform/{Windows,MacOS,Android}.cmake (SDK lock-in).
#
# The pkg-config state discovery derives is scoped to the GStreamer subtree, whose
# later probes (HwBuffers' gstreamer-{cuda,vulkan,d3d*}-1.0) need the same SDK view:
# PKG_CONFIG_EXECUTABLE, PKG_CONFIG_USE_CMAKE_PREFIX_PATH and the derived PKG_CONFIG_ARGN flags are normal variables of
# the calling directory, never written to the cache, and the environment is restored
# by qgc_gst_restore_pkgconfig_env once that subtree is configured.

include_guard(GLOBAL)

include("${CMAKE_CURRENT_LIST_DIR}/Platform.cmake")

# _qgc_gst_save_pkgconfig_env()
# Records the PKG_CONFIG_PATH / PKG_CONFIG_LIBDIR environment before discovery first changes it.
function(_qgc_gst_save_pkgconfig_env)
    get_property(_saved GLOBAL PROPERTY _QGC_GST_PKG_CONFIG_ENV_SAVED)
    if(_saved)
        return()
    endif()
    foreach(_name IN ITEMS PKG_CONFIG_PATH PKG_CONFIG_LIBDIR)
        if(DEFINED ENV{${_name}})
            set_property(GLOBAL PROPERTY _QGC_GST_SAVED_${_name} "$ENV{${_name}}")
        else()
            set_property(GLOBAL PROPERTY _QGC_GST_SAVED_${_name} "<unset>")
        endif()
    endforeach()
    set_property(GLOBAL PROPERTY _QGC_GST_PKG_CONFIG_ENV_SAVED TRUE)
endfunction()

# gstreamer_apply_pkgconfig_env(
#     MODE <SDK|SYSTEM_AUGMENT>
#     LIBDIR <dir>...                 # one or more pkgconfig dirs (semicolon list)
#     [PKG_CONFIG_EXE <path>]         # optional explicit pkg-config binary (PKG_CONFIG_EXECUTABLE)
#     [DONT_DEFINE_PREFIX]            # appends --dont-define-prefix to PKG_CONFIG_ARGN
# )
#
# MODE SDK             — lock pkg-config to the SDK: PKG_CONFIG_LIBDIR=<libdirs>,
#                        PKG_CONFIG_PATH cleared, and PKG_CONFIG_USE_CMAKE_PREFIX_PATH OFF so that
#                        FindPkgConfig does not put another GStreamer from CMAKE_PREFIX_PATH (vcpkg,
#                        Homebrew) ahead of the SDK. System .pc files invisible.
# MODE SYSTEM_AUGMENT  — prepend <libdirs> to PKG_CONFIG_PATH so SDK .pc files
#                        win but system glib/gobject .pc files stay discoverable.
#                        Used by Linux distro-installed GStreamer.
# PKG_CONFIG_EXECUTABLE / PKG_CONFIG_ARGN / PKG_CONFIG_USE_CMAKE_PREFIX_PATH are set as normal variables in the caller's
# scope.
# cmakelang reads set(ENV{<name>}) as a variable name.
# cmake-lint: disable=C0103
function(gstreamer_apply_pkgconfig_env)
    cmake_parse_arguments(PARSE_ARGV 0 ARG "DONT_DEFINE_PREFIX" "MODE;PKG_CONFIG_EXE" "LIBDIR")
    _qgc_gst_check_args(gstreamer_apply_pkgconfig_env MODE LIBDIR)

    _qgc_gst_save_pkgconfig_env()
    if(ARG_PKG_CONFIG_EXE)
        set(PKG_CONFIG_EXECUTABLE
            "${ARG_PKG_CONFIG_EXE}"
            PARENT_SCOPE
        )
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
        set(PKG_CONFIG_USE_CMAKE_PREFIX_PATH
            OFF
            PARENT_SCOPE
        )
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
        message(
            FATAL_ERROR "gstreamer_apply_pkgconfig_env: invalid MODE='${ARG_MODE}' (expected SDK or SYSTEM_AUGMENT)"
        )
    endif()

    if(ARG_DONT_DEFINE_PREFIX AND NOT "--dont-define-prefix" IN_LIST PKG_CONFIG_ARGN)
        list(APPEND PKG_CONFIG_ARGN --dont-define-prefix)
        set(PKG_CONFIG_ARGN
            "${PKG_CONFIG_ARGN}"
            PARENT_SCOPE
        )
    endif()
endfunction()

# qgc_gst_restore_pkgconfig_env()
# Restores the PKG_CONFIG_PATH / PKG_CONFIG_LIBDIR environment gstreamer_apply_pkgconfig_env found.
# cmakelang reads set(ENV{<name>}) as a variable name.
# cmake-lint: disable=C0103
function(qgc_gst_restore_pkgconfig_env)
    get_property(_saved GLOBAL PROPERTY _QGC_GST_PKG_CONFIG_ENV_SAVED)
    if(NOT _saved)
        return()
    endif()
    foreach(_name IN ITEMS PKG_CONFIG_PATH PKG_CONFIG_LIBDIR)
        get_property(_value GLOBAL PROPERTY _QGC_GST_SAVED_${_name})
        if(_value STREQUAL "<unset>")
            unset(ENV{${_name}})
        else()
            set(ENV{${_name}} "${_value}")
        endif()
    endforeach()
    set_property(GLOBAL PROPERTY _QGC_GST_PKG_CONFIG_ENV_SAVED FALSE)
endfunction()
