# macOS GStreamer: SDK discovery, invoked by Orchestrator.cmake, and the installers qgc_gst_install_platform_sdk
# (Install.cmake) runs for a GStreamer.framework or a flat SDK.

include_guard(GLOBAL)

# _qgc_gst_discover_macos_sdk()
# Uses the SDK under GStreamer_ROOT_DIR; else an earlier auto-downloaded SDK, /Library/Frameworks/GStreamer.framework,
# or a download of the official runtime and devel packages. A root inside a .framework bundle is used as that framework
# (GStreamer_USE_FRAMEWORK, GSTREAMER_FRAMEWORK_PATH) with the pkg-config it brings; another flat SDK uses Homebrew's
# pkg-config (cached PKG_CONFIG_EXECUTABLE).
# cmake-lint: disable=C0103
function(_qgc_gst_discover_macos_sdk)
    if(NOT GStreamer_ROOT_DIR AND EXISTS "/Library/Frameworks/GStreamer.framework")
        set(GStreamer_ROOT_DIR "/Library/Frameworks/GStreamer.framework/Versions/1.0")
    endif()

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

    set(GStreamer_USE_FRAMEWORK OFF)
    if(GStreamer_ROOT_DIR MATCHES "^(.*\\.framework)(/|$)")
        set(GStreamer_USE_FRAMEWORK ON)
        qgc_gst_resolve_layout(SDK_ROOT "${GStreamer_ROOT_DIR}" FRAMEWORK_BUNDLE "${CMAKE_MATCH_1}")
        qgc_gst_apply_pkgconfig_env(
            MODE SDK
            PKG_CONFIG_EXE "${GStreamer_ROOT_DIR}/bin/pkg-config"
            LIBDIR "${GSTREAMER_LIB_PATH}/pkgconfig" "${GSTREAMER_PLUGIN_PATH}/pkgconfig"
        )
    else()
        qgc_gst_resolve_layout(SDK_ROOT "${GStreamer_ROOT_DIR}")
        _qgc_gst_find_apple_pkg_config(PKG_CONFIG_EXECUTABLE)
        qgc_gst_apply_pkgconfig_env(
            MODE SDK LIBDIR "${GSTREAMER_LIB_PATH}/pkgconfig" "${GSTREAMER_PLUGIN_PATH}/pkgconfig"
        )
    endif()
    _qgc_gst_return_discovery()
endfunction()

# _qgc_gst_apply_macos_framework_overlay()
# Links GStreamer::GStreamer to the GStreamer.framework at GSTREAMER_FRAMEWORK_PATH and adds its Headers.
function(_qgc_gst_apply_macos_framework_overlay)
    qgc_gst_layout_get(FRAMEWORK_PATH _gst_framework_path)
    set(CMAKE_FIND_FRAMEWORK ONLY)
    cmake_path(GET _gst_framework_path PARENT_PATH _gst_framework_parent)
    find_library(
        _gst_framework_bundle GStreamer PATHS "${_gst_framework_parent}" "${_gst_framework_path}" "/Library/Frameworks"
                                              NO_CACHE
    )
    if(NOT _gst_framework_bundle)
        message(FATAL_ERROR "GStreamer: Could not locate GStreamer.framework")
    endif()
    target_link_libraries(GStreamer::GStreamer INTERFACE ${_gst_framework_bundle})
    target_include_directories(GStreamer::GStreamer INTERFACE "${_gst_framework_bundle}/Headers")
    target_compile_definitions(GStreamer::GStreamer INTERFACE QGC_GST_MACOS_FRAMEWORK)
    # Stash the discovered framework bundle path in the layout so the
    # install helpers read it via qgc_gst_layout_get instead of via cache pickup.
    qgc_gst_layout_set(FRAMEWORK_BUNDLE "${_gst_framework_bundle}")
endfunction()

# Stage GStreamer.framework into the app bundle's Contents/Frameworks/. Used
# when the SDK is consumed via the Cerbero macOS framework (Layout target
# stashed FRAMEWORK_BUNDLE).
function(qgc_gst_install_macos_framework_sdk app_name plugin_dest_out)
    qgc_gst_layout_get(FRAMEWORK_BUNDLE _gst_framework_bundle)
    if(NOT _gst_framework_bundle)
        message(FATAL_ERROR "qgc_gst_install_macos_framework_sdk: FRAMEWORK_BUNDLE not set in the GStreamer layout")
    endif()
    get_filename_component(_gst_framework_real "${_gst_framework_bundle}" REALPATH)
    # Relative to the install prefix; install(CODE) prepends $ENV{DESTDIR}${CMAKE_INSTALL_PREFIX} at install time.
    set(_gst_fw_dest "${app_name}.app/Contents/Frameworks/GStreamer.framework")
    # The framework's GPL libraries serve only GPL plugins, which the plugin allowlist leaves out.
    list(JOIN GSTREAMER_GPL_DENYLIST "|" _gst_gpl_names)

    install(CODE "file(REMOVE_RECURSE \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${_gst_fw_dest}\")")
    install(
        DIRECTORY "${_gst_framework_real}/Versions/1.0/"
        DESTINATION "${_gst_fw_dest}/Versions/1.0"
        USE_SOURCE_PERMISSIONS
        PATTERN "*.la" EXCLUDE
        PATTERN "*.a" EXCLUDE
        PATTERN "*/bin" EXCLUDE
        PATTERN "*/gst-validate-launcher" EXCLUDE
        PATTERN "*/Headers" EXCLUDE
        PATTERN "*/include" EXCLUDE
        PATTERN "*/pkgconfig" EXCLUDE
        PATTERN "*/share/aclocal" EXCLUDE
        PATTERN "*/share/bash-completion" EXCLUDE
        PATTERN "*/share/gdb" EXCLUDE
        PATTERN "*/share/gst-android" EXCLUDE
        PATTERN "*/share/gtk-doc" EXCLUDE
        PATTERN "*/share/installed-tests" EXCLUDE
        PATTERN "*/share/locale" EXCLUDE
        PATTERN "*/share/man" EXCLUDE
        PATTERN "*gstpython*" EXCLUDE
        PATTERN "*/lib/python3*" EXCLUDE
        PATTERN "Commands" EXCLUDE
        REGEX ".*/lib/gstreamer-1.0/libgst.*" EXCLUDE
        REGEX "/(lib)?(${_gst_gpl_names})([-_.][^/]*)?$" EXCLUDE
    )
    set_property(GLOBAL APPEND PROPERTY _QGC_GST_INSTALL_DIRS "${_gst_fw_dest}")

    install(
        CODE "
        set(_fw \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${_gst_fw_dest}\")
        if(NOT EXISTS \"\${_fw}/Versions/1.0/GStreamer\")
            message(FATAL_ERROR \"GStreamer framework: Versions/1.0/GStreamer not found — SDK layout may have changed\")
        endif()
        file(CREATE_LINK 1.0 \"\${_fw}/Versions/Current\" SYMBOLIC)
        file(CREATE_LINK Versions/Current/GStreamer \"\${_fw}/GStreamer\" SYMBOLIC)
        if(IS_DIRECTORY \"\${_fw}/Versions/1.0/Resources\")
            file(CREATE_LINK Versions/Current/Resources \"\${_fw}/Resources\" SYMBOLIC)
        endif()
    "
    )

    set(_plugin_dest "${_gst_fw_dest}/Versions/1.0/lib/gstreamer-1.0")
    qgc_gst_install_plugins("${_gst_framework_real}/Versions/1.0/lib/gstreamer-1.0" "${_plugin_dest}" "libgst" "dylib")
    set(${plugin_dest_out}
        "${_plugin_dest}"
        PARENT_SCOPE
    )
endfunction()

# Stage flat-layout dylibs/plugins/gio modules + rpath fixups + helpers. Used for a flat (downloaded or user-supplied)
# macOS GStreamer, which has no framework wrapper. gioopenssl verifies against the Keychain, so the SDK's CA bundle
# stays out.
function(qgc_gst_install_macos_flat_sdk app_name plugin_dest_out)
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

    set(${plugin_dest_out}
        "${_plugin_dest}"
        PARENT_SCOPE
    )
endfunction()

# Dispatch to the framework or flat installer based on whether the Layout
# target stashed FRAMEWORK_BUNDLE (set when GStreamer.framework is in use), then install the notice.
function(qgc_gst_install_macos_sdk app_name plugin_dest_out)
    qgc_gst_layout_get(FRAMEWORK_BUNDLE _gst_framework_bundle)
    if(_gst_framework_bundle)
        qgc_gst_install_macos_framework_sdk("${app_name}" _plugin_dest)
    else()
        qgc_gst_install_macos_flat_sdk("${app_name}" _plugin_dest)
    endif()
    _qgc_gst_install_notice("${app_name}.app/Contents/Resources/licenses/gstreamer")
    set(${plugin_dest_out}
        "${_plugin_dest}"
        PARENT_SCOPE
    )
endfunction()
