# macOS GStreamer: SDK discovery, invoked by Orchestrator.cmake, and the installer qgc_gst_install_platform_sdk
# (Install.cmake) runs.

include_guard(GLOBAL)

# _qgc_gst_discover_macos_sdk()
# Uses the SDK under GStreamer_ROOT_DIR (the user's, or the one an earlier configure downloaded), else downloads the
# official runtime and devel packages. The SDK is read with a pkg-config of the host (cached PKG_CONFIG_EXECUTABLE).
# cmake-lint: disable=C0103
function(_qgc_gst_discover_macos_sdk)
    if(NOT GStreamer_ROOT_DIR OR NOT EXISTS "${GStreamer_ROOT_DIR}")
        _qgc_gst_space_free_download_cache()
        _qgc_gst_acquire_macos_sdk(GStreamer_ROOT_DIR)
        set(GStreamer_AUTO_DOWNLOADED TRUE)
    endif()

    # Homebrew's libraries and plugins load each other by absolute paths under its prefix, so an app bundle made from
    # them would not carry its own GStreamer.
    if(GStreamer_ROOT_DIR MATCHES "/(Cellar|opt)/gstreamer(/|$)" OR EXISTS "${GStreamer_ROOT_DIR}/bin/brew")
        message(
            FATAL_ERROR "GStreamer: GStreamer_ROOT_DIR '${GStreamer_ROOT_DIR}' is Homebrew's GStreamer, which cannot "
                        "be bundled into the app. Unset GStreamer_ROOT_DIR to download the official SDK."
        )
    endif()

    qgc_gst_resolve_layout(SDK_ROOT "${GStreamer_ROOT_DIR}")
    _qgc_gst_find_apple_pkg_config(PKG_CONFIG_EXECUTABLE)
    qgc_gst_apply_pkgconfig_env(MODE SDK LIBDIR "${GSTREAMER_LIB_PATH}/pkgconfig" "${GSTREAMER_PLUGIN_PATH}/pkgconfig")
    _qgc_gst_return_discovery()
endfunction()

# qgc_gst_install_macos_sdk(<app_name> <plugin_dest_out>)
# Stages the SDK's plugins, GIO modules and helpers and the libraries those load into <app_name>.app, with their rpath
# fixups, then the notice; sets <plugin_dest_out> to the plugin directory, relative to the install prefix. gioopenssl
# verifies against the Keychain, so the SDK's CA bundle stays out.
function(qgc_gst_install_macos_sdk app_name plugin_dest_out)
    qgc_gst_layout_get(LIB_PATH _gst_lib_path)
    qgc_gst_layout_get(PLUGIN_PATH _gst_plugin_path)
    qgc_gst_layout_get(PLUGIN_SCANNER_PATH _gst_scanner_path)
    qgc_gst_layout_get(GIO_MODULE_PATH _gst_gio_module_path)
    # Relative to the install prefix; install(CODE) prepends $ENV{DESTDIR}${CMAKE_INSTALL_PREFIX} at install time.
    set(_mac_fw_dest "${app_name}.app/Contents/Frameworks")
    set(_mac_lib_dest "${app_name}.app/Contents/lib")
    set(_mac_libexec_dest "${app_name}.app/Contents/libexec/gstreamer-1.0")

    _qgc_gst_begin_runtime_closure()
    set(_plugin_dest "${_mac_lib_dest}/gstreamer-1.0")
    qgc_gst_install_plugins("${_gst_plugin_path}" "${_plugin_dest}" "libgst" "dylib")
    # The official SDK's GIO modules are .so files, unlike its plugins.
    qgc_gst_install_gio_modules("${_gst_gio_module_path}" "${_mac_lib_dest}/gio/modules" "so")
    # libproxy, which the giolibproxy module links, loads its backend as @rpath/libproxy/<backend>: from libproxy/
    # beside the bundled libproxy, through the executable's rpath.
    file(GLOB _gst_libproxy_backends "${_gst_lib_path}/libproxy/*.dylib")
    _qgc_gst_install_files("${_mac_fw_dest}/libproxy" ${_gst_libproxy_backends})
    # The plugin scanner is installed before the rpath fixup below, which patches the installed copy.
    set(_gst_helpers "")
    if(EXISTS "${_gst_scanner_path}/gst-plugin-scanner")
        list(APPEND _gst_helpers "${_gst_scanner_path}/gst-plugin-scanner")
    endif()
    _qgc_gst_install_files("${_mac_libexec_dest}" PROGRAMS ${_gst_helpers})
    _qgc_gst_install_runtime_closure("${_mac_fw_dest}")

    # Entry "<dir>:<rpath>:<glob>" — libexec helpers are extensionless, so * not *.dylib.
    foreach(
        _rpath_dir IN
        ITEMS "${_plugin_dest}:@loader_path/../../Frameworks:*.dylib"
              "${_mac_lib_dest}/gio/modules:@loader_path/../../../Frameworks:*.so"
              "${_mac_libexec_dest}:@loader_path/../../Frameworks:*"
    )
        string(REPLACE ":" ";" _parts "${_rpath_dir}")
        list(GET _parts 0 _dir)
        list(GET _parts 1 _rpath)
        list(GET _parts 2 _glob)
        install(
            CODE "
            file(GLOB _libs \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${_dir}/${_glob}\")
            foreach(_lib \${_libs})
                if(IS_DIRECTORY \"\${_lib}\")
                    continue()
                endif()
                execute_process(
                    COMMAND otool -l \"\${_lib}\"
                    OUTPUT_VARIABLE _otool_out
                    RESULT_VARIABLE _otool_rc
                    ERROR_VARIABLE  _otool_err
                )
                if(NOT _otool_rc EQUAL 0)
                    # otool failed: empty output would make the FIND below wrongly
                    # conclude 'rpath absent' and add a duplicate — skip and warn.
                    message(WARNING \"GStreamer: otool -l failed for \${_lib} (rc=\${_otool_rc}): \${_otool_err}; \"
                                    \"skipping rpath fixup\")
                    continue()
                endif()
                string(FIND \"\${_otool_out}\" \"${_rpath}\" _has_rpath)
                if(_has_rpath EQUAL -1)
                    execute_process(
                        COMMAND install_name_tool -add_rpath \"${_rpath}\" \"\${_lib}\"
                        RESULT_VARIABLE _int_rc
                        ERROR_VARIABLE _int_err
                    )
                    if(NOT _int_rc EQUAL 0)
                        message(WARNING \"GStreamer: install_name_tool -add_rpath failed for \${_lib}: \${_int_err}\")
                    endif()
                endif()
            endforeach()
        "
        )
    endforeach()

    _qgc_gst_install_notice("${app_name}.app/Contents/Resources/licenses/gstreamer")
    set(${plugin_dest_out}
        "${_plugin_dest}"
        PARENT_SCOPE
    )
endfunction()
