# Linux GStreamer: SDK discovery, invoked by Orchestrator.cmake, and the installer gstreamer_install_platform_sdk
# (Install.cmake) runs.

include_guard(GLOBAL)

# _qgc_gst_linux_pkg_variable(<out_var> <module> <variable>)
# Sets <out_var> to the pkg-config <variable> of <module>, prefixed with CMAKE_SYSROOT in a cross build: pkg-config
# prefixes the sysroot to flags only.
function(_qgc_gst_linux_pkg_variable out_var module variable)
    pkg_get_variable(_value ${module} ${variable})
    if(_value AND CMAKE_SYSROOT)
        cmake_path(IS_PREFIX CMAKE_SYSROOT "${_value}" NORMALIZE _in_sysroot)
        if(NOT _in_sysroot)
            set(_value "${CMAKE_SYSROOT}${_value}")
        endif()
    endif()
    set(${out_var}
        "${_value}"
        PARENT_SCOPE
    )
endfunction()

# _qgc_gst_discover_linux_sdk()
# Uses the GStreamer pkg-config finds: the system's, or one under GStreamer_ROOT_DIR, whose pkgconfig directories go
# ahead of the system's. Its layout comes from gstreamer-1.0.pc (prefix, libdir, includedir, pluginsdir and
# pluginscannerdir) and gio-2.0.pc (giomoduledir).
# cmake-lint: disable=C0103
function(_qgc_gst_discover_linux_sdk)
    find_package(PkgConfig REQUIRED QUIET)
    if(GStreamer_ROOT_DIR)
        file(GLOB _pc_files "${GStreamer_ROOT_DIR}/lib*/pkgconfig/gstreamer-1.0.pc"
             "${GStreamer_ROOT_DIR}/lib/*/pkgconfig/gstreamer-1.0.pc"
        )
        if(NOT _pc_files)
            message(
                FATAL_ERROR
                    "GStreamer: no gstreamer-1.0.pc in ${GStreamer_ROOT_DIR}/lib*/pkgconfig or "
                    "${GStreamer_ROOT_DIR}/lib/<multiarch>/pkgconfig; point GStreamer_ROOT_DIR at the "
                    "prefix GStreamer is installed in."
            )
        endif()
        list(TRANSFORM _pc_files REPLACE "/gstreamer-1\\.0\\.pc$" "")
        gstreamer_apply_pkgconfig_env(MODE SYSTEM_AUGMENT LIBDIR ${_pc_files})
    endif()

    # <layout key>|<module>|<variable>|REQUIRED or OPTIONAL; an OPTIONAL one left empty takes the SDK-layout default.
    set(_layout "")
    foreach(
        _entry IN
        ITEMS
            "SDK_ROOT|gstreamer-1.0|prefix|REQUIRED"
            "LIB_PATH|gstreamer-1.0|libdir|REQUIRED"
            "INCLUDE_PATH|gstreamer-1.0|includedir|REQUIRED"
            "PLUGIN_PATH|gstreamer-1.0|pluginsdir|REQUIRED"
            "PLUGIN_SCANNER_PATH|gstreamer-1.0|pluginscannerdir|OPTIONAL"
            "GIO_MODULE_PATH|gio-2.0|giomoduledir|OPTIONAL"
    )
        string(REPLACE "|" ";" _entry "${_entry}")
        list(GET _entry 0 _key)
        list(GET _entry 1 _module)
        list(GET _entry 2 _variable)
        list(GET _entry 3 _need)
        _qgc_gst_linux_pkg_variable(_value ${_module} ${_variable})
        if(_value)
            list(APPEND _layout ${_key} "${_value}")
        elseif(_need STREQUAL "REQUIRED")
            message(
                FATAL_ERROR
                    "GStreamer: pkg-config found no ${_module} (its '${_variable}' variable is empty). Install the "
                    "GStreamer development packages (python3 -m tools.setup.install_dependencies) or set "
                    "GStreamer_ROOT_DIR."
            )
        endif()
    endforeach()
    gstreamer_create_layout_target(${_layout})
    _qgc_gst_return_discovery()
endfunction()

# Linux: plugins, the GIO modules for TLS and proxies, the gst-plugin-scanner/gst-ptp-helper binaries and the notice.
# The libraries come from the AppImage's dependency deployment (cmake/install/CreateAppImage.cmake).
function(gstreamer_install_linux_sdk app_name plugin_dest_out)
    set(_plugin_dest "lib/gstreamer-1.0")
    gstreamer_install_plugins("${GSTREAMER_PLUGIN_PATH}" "${_plugin_dest}" "libgst" "so")
    # glib-networking's GnuTLS backend and libproxy proxy resolver, and dconf's GSettings backend, through which
    # libproxy reads GNOME's proxy settings (it reads KDE's itself). glib-networking's GNOME resolver stays out: GIO
    # instantiates every resolver before asking which applies, and it aborts the process where org.gnome.system.proxy's
    # schema isn't installed. The VFS modules (gvfsdbus, remote volume monitor) stay out: the bundle runs GIO with
    # GIO_USE_VFS=local (AppRun, and GStreamerEnvironment.cc on GLib 2.76+) or alongside the system's modules.
    gstreamer_install_gio_modules(
        "${GSTREAMER_GIO_MODULE_PATH}" "lib/gio/modules" "so" giognutls giolibproxy dconfsettings
    )
    # gstreamer-1.0.pc's pluginscannerdir (Debian lib/<multiarch>/gstreamer1.0/gstreamer-1.0, Fedora
    # libexec/gstreamer-1.0, Arch lib/gstreamer-1.0).
    foreach(_helper IN ITEMS gst-plugin-scanner gst-ptp-helper)
        set(_path "${GSTREAMER_PLUGIN_SCANNER_PATH}/${_helper}")
        if(EXISTS "${_path}")
            install(PROGRAMS "${_path}" DESTINATION "lib/gstreamer1.0/gstreamer-1.0")
        elseif(_helper STREQUAL "gst-plugin-scanner")
            message(
                WARNING "gst-plugin-scanner not found in ${GSTREAMER_PLUGIN_SCANNER_PATH}; AppImage video may not "
                        "work"
            )
        endif()
    endforeach()
    _qgc_gst_install_notice(share/licenses/gstreamer)
    set(${plugin_dest_out}
        "${_plugin_dest}"
        PARENT_SCOPE
    )
endfunction()
