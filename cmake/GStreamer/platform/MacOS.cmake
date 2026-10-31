# macOS GStreamer: SDK discovery, invoked by Orchestrator.cmake, and the installers gstreamer_install_platform_sdk
# (Install.cmake) runs for a GStreamer.framework or a flat SDK.

include_guard(GLOBAL)

# _qgc_gst_homebrew_prefix(<root> <out_var>)
# Sets <out_var> to the Homebrew prefix that <root> is or holds as opt/<formula> or Cellar/<formula>/<version>, or to ""
# when <root> is not in a Homebrew prefix.
function(_qgc_gst_homebrew_prefix root out_var)
    set(_prefix "${root}")
    if(root MATCHES "^(.+)/(opt/[^/]+|Cellar/[^/]+/[^/]+)/?$")
        set(_prefix "${CMAKE_MATCH_1}")
    endif()
    if(NOT EXISTS "${_prefix}/bin/brew")
        set(_prefix "")
    endif()
    set(${out_var}
        "${_prefix}"
        PARENT_SCOPE
    )
endfunction()

# _qgc_gst_find_homebrew_gstreamer(<out_var>)
# Sets <out_var> to Homebrew's gstreamer: `brew --prefix gstreamer`, else /opt/homebrew/opt/gstreamer or
# /usr/local/opt/gstreamer. Fails configure when none is installed.
function(_qgc_gst_find_homebrew_gstreamer out_var)
    set(_kegs /opt/homebrew/opt/gstreamer /usr/local/opt/gstreamer)
    find_program(_brew brew PATHS /opt/homebrew/bin /usr/local/bin NO_CACHE)
    if(_brew)
        execute_process(
            COMMAND "${_brew}" --prefix gstreamer
            OUTPUT_VARIABLE _brew_keg
            OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET
        )
        list(PREPEND _kegs ${_brew_keg})
    endif()
    foreach(_keg IN LISTS _kegs)
        if(EXISTS "${_keg}/lib/pkgconfig/gstreamer-1.0.pc")
            set(${out_var}
                "${_keg}"
                PARENT_SCOPE
            )
            return()
        endif()
    endforeach()
    message(
        FATAL_ERROR "GStreamer: QGC_GST_USE_HOMEBREW is ON but Homebrew's gstreamer is not in any of: ${_kegs}\n"
                    "Install it with: brew install gstreamer pkgconf"
    )
endfunction()

# _qgc_gst_discover_homebrew_sdk(<prefix>)
# Discovery of the Homebrew GStreamer at GStreamer_ROOT_DIR, in Homebrew <prefix> (the prefix itself means its
# opt/gstreamer), through the prefix's pkg-config: only that one searches the macOS shims (libffi, zlib) GLib requires;
# the dirs given to it find an unlinked gstreamer or glib. Sets the HOMEBREW_PREFIX layout property (Install.cmake).
# cmake-lint: disable=C0103
function(_qgc_gst_discover_homebrew_sdk prefix)
    if(GStreamer_ROOT_DIR STREQUAL prefix)
        set(GStreamer_ROOT_DIR "${prefix}/opt/gstreamer")
    endif()
    if(NOT EXISTS "${prefix}/bin/pkg-config")
        message(
            FATAL_ERROR "GStreamer: Homebrew's GStreamer needs Homebrew's pkg-config. Install it with: "
                        "brew install pkgconf"
        )
    endif()
    message(
        STATUS "GStreamer: Using Homebrew's GStreamer at ${GStreamer_ROOT_DIR} (builds only; installs and "
               "packages need the official SDK)"
    )
    gstreamer_create_layout_target(SDK_ROOT "${GStreamer_ROOT_DIR}")
    gstreamer_apply_pkgconfig_env(
        MODE SYSTEM_AUGMENT
        PKG_CONFIG_EXE "${prefix}/bin/pkg-config"
        LIBDIR "${GSTREAMER_LIB_PATH}/pkgconfig" "${prefix}/lib/pkgconfig" "${prefix}/opt/glib/lib/pkgconfig"
    )
    gstreamer_layout_set(HOMEBREW_PREFIX "${prefix}")
    _qgc_gst_return_discovery()
endfunction()

# _qgc_gst_discover_macos_sdk()
# Uses the SDK under GStreamer_ROOT_DIR; else, with QGC_GST_USE_HOMEBREW, Homebrew's gstreamer; else an earlier
# auto-downloaded SDK, /Library/Frameworks/GStreamer.framework, or a download of the official runtime and devel
# packages. A root in a Homebrew prefix goes to _qgc_gst_discover_homebrew_sdk. A root inside a .framework bundle is
# used as that framework (GStreamer_USE_FRAMEWORK, GSTREAMER_FRAMEWORK_PATH) with the pkg-config it brings; another
# flat SDK uses Homebrew's pkg-config (cached PKG_CONFIG_EXECUTABLE).
# cmake-lint: disable=C0103
function(_qgc_gst_discover_macos_sdk)
    if(QGC_GST_USE_HOMEBREW AND (NOT GStreamer_ROOT_DIR OR GStreamer_AUTO_DOWNLOADED))
        _qgc_gst_find_homebrew_gstreamer(GStreamer_ROOT_DIR)
        set(GStreamer_AUTO_DOWNLOADED FALSE)
    elseif(NOT GStreamer_ROOT_DIR AND EXISTS "/Library/Frameworks/GStreamer.framework")
        set(GStreamer_ROOT_DIR "/Library/Frameworks/GStreamer.framework/Versions/1.0")
    endif()

    if(NOT GStreamer_ROOT_DIR OR NOT EXISTS "${GStreamer_ROOT_DIR}")
        _qgc_gst_space_free_download_cache()
        _qgc_gst_acquire_macos_sdk(GStreamer_ROOT_DIR)
        set(GStreamer_AUTO_DOWNLOADED TRUE)
    endif()

    _qgc_gst_homebrew_prefix("${GStreamer_ROOT_DIR}" _brew_prefix)
    if(_brew_prefix)
        _qgc_gst_discover_homebrew_sdk("${_brew_prefix}")
        _qgc_gst_return_discovery()
    endif()

    set(GStreamer_USE_FRAMEWORK OFF)
    if(GStreamer_ROOT_DIR MATCHES "^(.*\\.framework)(/|$)")
        set(GStreamer_USE_FRAMEWORK ON)
        gstreamer_create_layout_target(SDK_ROOT "${GStreamer_ROOT_DIR}" FRAMEWORK_BUNDLE "${CMAKE_MATCH_1}")
        gstreamer_apply_pkgconfig_env(
            MODE SDK
            PKG_CONFIG_EXE "${GStreamer_ROOT_DIR}/bin/pkg-config"
            LIBDIR "${GSTREAMER_LIB_PATH}/pkgconfig" "${GSTREAMER_PLUGIN_PATH}/pkgconfig"
        )
    else()
        gstreamer_create_layout_target(SDK_ROOT "${GStreamer_ROOT_DIR}")
        _qgc_gst_find_apple_pkg_config(PKG_CONFIG_EXECUTABLE)
        gstreamer_apply_pkgconfig_env(
            MODE SDK LIBDIR "${GSTREAMER_LIB_PATH}/pkgconfig" "${GSTREAMER_PLUGIN_PATH}/pkgconfig"
        )
    endif()
    _qgc_gst_return_discovery()
endfunction()

# _qgc_gst_apply_macos_framework_overlay()
# Links GStreamer::GStreamer to the GStreamer.framework at GSTREAMER_FRAMEWORK_PATH and adds its Headers.
function(_qgc_gst_apply_macos_framework_overlay)
    set(CMAKE_FIND_FRAMEWORK ONLY)
    cmake_path(GET GSTREAMER_FRAMEWORK_PATH PARENT_PATH _gst_framework_parent)
    find_library(
        _gst_framework_bundle GStreamer PATHS "${_gst_framework_parent}" "${GSTREAMER_FRAMEWORK_PATH}"
                                              "/Library/Frameworks" NO_CACHE
    )
    if(NOT _gst_framework_bundle)
        message(FATAL_ERROR "GStreamer: Could not locate GStreamer.framework")
    endif()
    target_link_libraries(GStreamer::GStreamer INTERFACE ${_gst_framework_bundle})
    target_include_directories(GStreamer::GStreamer INTERFACE "${_gst_framework_bundle}/Headers")
    target_compile_definitions(GStreamer::GStreamer INTERFACE QGC_GST_MACOS_FRAMEWORK)
    # Stash the discovered framework bundle path on GStreamer::Layout so the
    # install helpers read it via gstreamer_layout_get instead of via cache pickup.
    gstreamer_layout_set(FRAMEWORK_BUNDLE "${_gst_framework_bundle}")
endfunction()

# Stage GStreamer.framework into the app bundle's Contents/Frameworks/. Used
# when the SDK is consumed via the Cerbero macOS framework (Layout target
# stashed FRAMEWORK_BUNDLE).
function(gstreamer_install_macos_framework_sdk app_name plugin_dest_out)
    gstreamer_layout_get(FRAMEWORK_BUNDLE _gst_framework_bundle)
    if(NOT _gst_framework_bundle)
        message(FATAL_ERROR "gstreamer_install_macos_framework_sdk: FRAMEWORK_BUNDLE not set on GStreamer::Layout")
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
    gstreamer_install_plugins(
        "${_gst_framework_real}/Versions/1.0/lib/gstreamer-1.0" "${_plugin_dest}" "libgst" "dylib"
    )
    set(${plugin_dest_out}
        "${_plugin_dest}"
        PARENT_SCOPE
    )
endfunction()

# Stage flat-layout dylibs/plugins/gio modules + rpath fixups + helpers. Used for a flat (downloaded or user-supplied)
# macOS GStreamer, which has no framework wrapper. gioopenssl verifies against the Keychain, so the SDK's CA bundle
# stays out.
function(gstreamer_install_macos_flat_sdk app_name plugin_dest_out)
    # Relative to the install prefix; install(CODE) prepends $ENV{DESTDIR}${CMAKE_INSTALL_PREFIX} at install time.
    set(_mac_fw_dest "${app_name}.app/Contents/Frameworks")
    set(_mac_lib_dest "${app_name}.app/Contents/lib")
    set(_mac_libexec_dest "${app_name}.app/Contents/libexec/gstreamer-1.0")

    _qgc_gst_begin_runtime_closure()
    set(_plugin_dest "${_mac_lib_dest}/gstreamer-1.0")
    gstreamer_install_plugins("${GSTREAMER_PLUGIN_PATH}" "${_plugin_dest}" "libgst" "dylib")
    gstreamer_install_gio_modules("${GSTREAMER_GIO_MODULE_PATH}" "${_mac_lib_dest}/gio/modules" "dylib")
    # Libexec helpers are installed before the rpath fixup below, which patches the installed copies.
    set(_gst_helpers "")
    foreach(_helper IN ITEMS gst-plugin-scanner gst-ptp-helper)
        if(EXISTS "${GSTREAMER_PLUGIN_SCANNER_PATH}/${_helper}")
            list(APPEND _gst_helpers "${GSTREAMER_PLUGIN_SCANNER_PATH}/${_helper}")
        endif()
    endforeach()
    _qgc_gst_install_files("${_mac_libexec_dest}" PROGRAMS ${_gst_helpers})
    _qgc_gst_install_runtime_closure("${_mac_fw_dest}")

    # Entry "<dir>:<rpath>:<glob>" — libexec helpers are extensionless, so * not *.dylib.
    foreach(
        _rpath_dir IN
        ITEMS "${_plugin_dest}:@loader_path/../../Frameworks:*.dylib"
              "${_mac_lib_dest}/gio/modules:@loader_path/../../../Frameworks:*.dylib"
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
function(gstreamer_install_macos_sdk app_name plugin_dest_out)
    gstreamer_layout_get(FRAMEWORK_BUNDLE _gst_framework_bundle)
    if(_gst_framework_bundle)
        gstreamer_install_macos_framework_sdk("${app_name}" _plugin_dest)
    else()
        gstreamer_install_macos_flat_sdk("${app_name}" _plugin_dest)
    endif()
    _qgc_gst_install_notice("${app_name}.app/Contents/Resources/licenses/gstreamer")
    set(${plugin_dest_out}
        "${_plugin_dest}"
        PARENT_SCOPE
    )
endfunction()
