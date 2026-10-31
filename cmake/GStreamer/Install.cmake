# Platform SDK install helpers — copy GStreamer plugins, GIO modules and helper
# binaries into the install tree. Called from
# src/VideoManager/VideoReceiver/GStreamer/CMakeLists.txt after the ANDROID/IOS
# early-return. Windows and a flat macOS SDK install only the SDK libraries that the installed plugins, GIO modules,
# helpers and QGC itself load (_qgc_gst_install_runtime_closure), and every installer fails an install that would ship a
# GPL library or plugin (_qgc_gst_install_gpl_check).
# Depends on: Components.cmake (gstreamer_platform_plugin_attrs), Platform.cmake (qgc_gst_platform),
#             PluginPolicy.cmake (GSTREAMER_BUNDLE_REQUIRED_PLUGINS, GSTREAMER_GPL_DENYLIST; included again by the
#             install-time checks).

include_guard(GLOBAL)

include("${CMAKE_CURRENT_LIST_DIR}/Platform.cmake")

# _qgc_gst_filter_plugin_paths(<prefix> <paths> <out_var> [<name>...])
# Pure (script-testable) name filter: keep only paths whose basename is
# <prefix><name>(boundary) where <name> is one of the <name>s, by default GSTREAMER_PLUGINS. The trailing
# boundary stops gstvideo matching gstvideoconvert.
function(_qgc_gst_filter_plugin_paths prefix paths out_var)
    set(_names ${ARGN})
    if(NOT _names)
        set(_names ${GSTREAMER_PLUGINS})
    endif()
    set(_filtered "")
    foreach(_path IN LISTS paths)
        get_filename_component(_name "${_path}" NAME)
        foreach(_allowed IN LISTS _names)
            string(REGEX REPLACE "([][.+*?^$()|\\\\])" "\\\\\\1" _allowed_re "${_allowed}")
            if(_name MATCHES "^${prefix}${_allowed_re}([^a-zA-Z0-9]|$)")
                list(APPEND _filtered "${_path}")
                break()
            endif()
        endforeach()
    endforeach()
    set(${out_var}
        "${_filtered}"
        PARENT_SCOPE
    )
endfunction()

# gstreamer_install_plugins(<source_dir> <dest_dir> <prefix> <extension>)
# Installs the GSTREAMER_PLUGINS found as <prefix><name>*.<extension> in <source_dir>. Fails when none match: the glob
# runs at configure time, so the SDK must be complete then. Records the files in QGC_GST_SHIPPED_PLUGINS, which the
# CI boot test of a build tree loads instead of the whole SDK, under that public QGC_ name.
# cmake-lint: disable=C0103
function(gstreamer_install_plugins source_dir dest_dir prefix extension)
    file(GLOB _plugins "${source_dir}/${prefix}*.${extension}")
    _qgc_gst_filter_plugin_paths("${prefix}" "${_plugins}" _plugins)
    if(NOT _plugins)
        message(
            FATAL_ERROR "gstreamer_install_plugins: no requested plugin matches ${source_dir}/${prefix}*.${extension}. "
                        "Re-run cmake configure against a complete SDK."
        )
    endif()
    _qgc_gst_install_files("${dest_dir}" ${_plugins})
    set(QGC_GST_SHIPPED_PLUGINS
        "${_plugins}"
        CACHE INTERNAL "GStreamer plugin files the package ships"
    )
endfunction()

# gstreamer_install_gio_modules(<source_dir> <dest_dir> <extension> [<module>...])
# Installs every *.<extension> GIO module in <source_dir>, or only the lib<module>.<extension> ones given.
function(gstreamer_install_gio_modules source_dir dest_dir extension)
    if(NOT EXISTS "${source_dir}")
        message(WARNING "gstreamer_install_gio_modules: ${source_dir} does not exist")
        return()
    endif()
    file(GLOB _modules "${source_dir}/*.${extension}")
    if(ARGN)
        _qgc_gst_filter_plugin_paths(lib "${_modules}" _modules ${ARGN})
    endif()
    _qgc_gst_install_files("${dest_dir}" ${_modules})
endfunction()

# _qgc_gst_install_files(<dest_dir> [PROGRAMS] <file>...)
# Installs the <file>s (executables with PROGRAMS) into <dest_dir> and records <dest_dir> for
# _qgc_gst_install_gpl_check. While _QGC_GST_RUNTIME_CLOSURE is set (_qgc_gst_install_runtime_closure), the files also
# seed the runtime dependency set the closure installs: each becomes an imported module or executable installed with
# install(IMPORTED_RUNTIME_ARTIFACTS).
function(_qgc_gst_install_files dest_dir)
    cmake_parse_arguments(PARSE_ARGV 1 ARG "PROGRAMS" "" "")
    set(_files ${ARG_UNPARSED_ARGUMENTS})
    if(NOT _files)
        return()
    endif()
    set_property(GLOBAL APPEND PROPERTY _QGC_GST_INSTALL_DIRS "${dest_dir}")
    if(NOT _QGC_GST_RUNTIME_CLOSURE)
        if(ARG_PROGRAMS)
            install(PROGRAMS ${_files} DESTINATION "${dest_dir}")
        else()
            install(FILES ${_files} DESTINATION "${dest_dir}")
        endif()
        return()
    endif()
    foreach(_file IN LISTS _files)
        get_property(_count GLOBAL PROPERTY _QGC_GST_RUNTIME_FILE_COUNT)
        math(EXPR _count "${_count} + 1")
        set_property(GLOBAL PROPERTY _QGC_GST_RUNTIME_FILE_COUNT ${_count})
        set(_target GStreamer::runtime_file_${_count})
        if(ARG_PROGRAMS)
            add_executable(${_target} IMPORTED)
        else()
            add_library(${_target} MODULE IMPORTED)
        endif()
        set_target_properties(
            ${_target}
            PROPERTIES
                IMPORTED_LOCATION "${_file}"
        )
        install(
            IMPORTED_RUNTIME_ARTIFACTS
            ${_target}
            RUNTIME_DEPENDENCY_SET
            qgc_gstreamer
            RUNTIME
            DESTINATION
            "${dest_dir}"
            LIBRARY
            DESTINATION
            "${dest_dir}"
        )
    endforeach()
endfunction()

# _qgc_gst_linked_sdk_libraries(<out_var>)
# Sets <out_var> to the SDK's shared libraries QGC links through GStreamer::GStreamer: the .dylib files on macOS, and on
# Windows the <name>[-<n>].dll in <root>/bin of each <name>.lib import library.
function(_qgc_gst_linked_sdk_libraries out_var)
    _qgc_gst_link_closure(_items GStreamer::GStreamer)
    set(_libraries "")
    foreach(_item IN LISTS _items)
        cmake_path(IS_PREFIX GStreamer_ROOT_DIR "${_item}" NORMALIZE _in_sdk)
        if(NOT IS_ABSOLUTE "${_item}" OR NOT _in_sdk)
            continue()
        endif()
        if(_item MATCHES "\\.dylib$")
            list(APPEND _libraries "${_item}")
        elseif(_item MATCHES "\\.lib$")
            cmake_path(GET _item STEM _stem)
            file(GLOB _dlls "${GStreamer_ROOT_DIR}/bin/${_stem}*.dll")
            string(REGEX REPLACE "([][.+*?^$()|\\\\])" "\\\\\\1" _stem_re "${_stem}")
            list(FILTER _dlls INCLUDE REGEX "/${_stem_re}(-[0-9]+)?\\.dll$")
            list(APPEND _libraries ${_dlls})
        endif()
    endforeach()
    list(REMOVE_DUPLICATES _libraries)
    set(${out_var}
        "${_libraries}"
        PARENT_SCOPE
    )
endfunction()

# _qgc_gst_begin_runtime_closure()
# Makes the files the caller installs from here on with _qgc_gst_install_files seed the runtime dependency set
# qgc_gstreamer that _qgc_gst_install_runtime_closure installs.
# cmake-lint: disable=C0103
macro(_qgc_gst_begin_runtime_closure)
    set(_QGC_GST_RUNTIME_CLOSURE ON)
endmacro()

# _qgc_gst_install_runtime_closure(<library_dest>)
# Installs into <library_dest> the libraries of the SDK at GStreamer_ROOT_DIR that the seeds installed since
# _qgc_gst_begin_runtime_closure() and the SDK libraries QGC links load, directly or not, instead of every library the
# SDK ships (which include unused codecs, some GPL), and ends the closure in the caller. Libraries loaded with dlopen,
# such as libproxy's backends, must be installed as seeds themselves.
function(_qgc_gst_install_runtime_closure library_dest)
    _qgc_gst_linked_sdk_libraries(_qgc_gst_linked)
    _qgc_gst_install_files("${library_dest}" ${_qgc_gst_linked})
    set_property(GLOBAL APPEND PROPERTY _QGC_GST_INSTALL_DIRS "${library_dest}")
    set(_QGC_GST_RUNTIME_CLOSURE
        OFF
        PARENT_SCOPE
    )
    string(REGEX REPLACE "([][.+*?^$()|\\\\])" "\\\\\\1" _qgc_gst_root_re "${GStreamer_ROOT_DIR}")
    if(WIN32)
        # DLL lookups also see the lowercased path.
        string(TOLOWER "${_qgc_gst_root_re}" _qgc_gst_root_lower_re)
        set(_qgc_gst_include_res "^${_qgc_gst_root_re}/" "^${_qgc_gst_root_lower_re}/")
    else()
        set(_qgc_gst_include_res "^${_qgc_gst_root_re}/")
    endif()
    # The system's libraries resolve outside the SDK and are excluded; API sets, the MSVC runtime (installed by
    # InstallRequiredSystemLibraries) and macOS system libraries in the dyld cache may not resolve at all.
    # libproxy keeps the backend library its own library links (pxbackend) in lib/libproxy.
    install(
        RUNTIME_DEPENDENCY_SET
        qgc_gstreamer
        DIRECTORIES
        "${GStreamer_ROOT_DIR}/bin"
        "${GSTREAMER_LIB_PATH}"
        "${GSTREAMER_LIB_PATH}/libproxy"
        PRE_EXCLUDE_REGEXES
        "^api-ms-"
        "^ext-ms-"
        "^(vcruntime|msvcp|concrt|ucrtbase)"
        "^/usr/lib/"
        "^/System/"
        POST_INCLUDE_REGEXES
        ${_qgc_gst_include_res}
        POST_EXCLUDE_REGEXES
        ".*"
        RUNTIME
        DESTINATION
        "${library_dest}"
        LIBRARY
        DESTINATION
        "${library_dest}"
        FRAMEWORK
        DESTINATION
        "${library_dest}"
    )
endfunction()

# _qgc_gst_install_gpl_check()
# Fails the install when a GSTREAMER_GPL_DENYLIST library or plugin landed in a directory the GStreamer installers
# installed into.
function(_qgc_gst_install_gpl_check)
    get_property(_dirs GLOBAL PROPERTY _QGC_GST_INSTALL_DIRS)
    list(REMOVE_DUPLICATES _dirs)
    set(_check_code
        [=[
cmake_policy(PUSH)
cmake_policy(VERSION 3.25)
include("@_policy_file@")
set(_gpl_files "")
foreach(_dir IN ITEMS @_dirs@)
    file(GLOB_RECURSE _files "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/${_dir}/*")
    gstreamer_gpl_files(_found ${_files})
    list(APPEND _gpl_files ${_found})
endforeach()
if(_gpl_files)
    list(REMOVE_DUPLICATES _gpl_files)
    list(JOIN _gpl_files "
    " _gpl_list)
    message(FATAL_ERROR "GStreamer install check: the install ships GPL-licensed GStreamer files, which QGC's LGPL "
                        "notice does not cover:
    ${_gpl_list}
Remove them from the plugin allowlist "
                        "(.github/build-config.json) or find what loads them.")
endif()
cmake_policy(POP)
]=]
    )
    set(_policy_file "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/PluginPolicy.cmake")
    string(CONFIGURE "${_check_code}" _check_code @ONLY)
    install(CODE "${_check_code}")
endfunction()

# Per-platform install helpers, named gstreamer_install_<platform>_sdk (Platform.cmake) and called as
# <helper>(<app_name> <plugin_dest_out>): each sets <plugin_dest_out> to the install-prefix-relative directory it
# installs the plugins to. They use variables set by cmake/GStreamer/Orchestrator.cmake (GSTREAMER_*_PATH,
# GStreamer_ROOT_DIR, GStreamer_AUTO_DOWNLOADED, etc.).

# Installs the LGPL text and notice the Orchestrator wrote (GStreamer::Layout NOTICE_DIR) into <dest>.
function(_qgc_gst_install_notice dest)
    gstreamer_layout_get(NOTICE_DIR _notice_dir)
    if(_notice_dir)
        install(FILES "${_notice_dir}/NOTICE.txt" "${_notice_dir}/LGPL-2.1.txt" DESTINATION "${dest}")
    endif()
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

# Windows: the plugins, GIO modules, libproxy backends and libexec helpers, the SDK DLLs they and QGC load, and the
# notice. gioopenssl verifies against the Windows certificate stores, so the SDK's CA bundle stays out.
function(gstreamer_install_windows_sdk app_name plugin_dest_out)
    set(_plugin_dest "${CMAKE_INSTALL_LIBDIR}/gstreamer-1.0")
    _qgc_gst_begin_runtime_closure()
    gstreamer_install_plugins("${GSTREAMER_PLUGIN_PATH}" "${_plugin_dest}" "gst" "dll")
    gstreamer_install_gio_modules("${GSTREAMER_GIO_MODULE_PATH}" "${CMAKE_INSTALL_LIBDIR}/gio/modules" "dll")
    # giolibproxy.dll loads proxy-1.dll, which loads its backends from lib/libproxy; startup only puts the app's bin
    # directory on PATH, so they go there too.
    file(GLOB _gst_libproxy_backends "${GStreamer_ROOT_DIR}/lib/libproxy/*.dll")
    _qgc_gst_install_files("${CMAKE_INSTALL_BINDIR}" ${_gst_libproxy_backends})
    file(GLOB _gst_helpers "${GSTREAMER_PLUGIN_SCANNER_PATH}/*.exe")
    _qgc_gst_install_files("${CMAKE_INSTALL_LIBEXECDIR}/gstreamer-1.0" PROGRAMS ${_gst_helpers})
    _qgc_gst_install_runtime_closure("${CMAKE_INSTALL_BINDIR}")
    _qgc_gst_install_notice(share/licenses/gstreamer)
    set(${plugin_dest_out}
        "${_plugin_dest}"
        PARENT_SCOPE
    )
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

# Top-level dispatcher — call this from CMakeLists.txt after the ANDROID/IOS guard (mobile platforms link their plugins
# statically and have no install helper). Runs the platform's gstreamer_install_<platform>_sdk.
# <app_name> is CMAKE_PROJECT_NAME in the caller.
# gstreamer_install_platform_sdk(<app_name> [REQUIRED_PLUGINS <names…>])
# REQUIRED_PLUGINS: list of plugin basenames (no platform prefix or extension) to
# verify exist post-install; default is the minimum needed to run any pipeline,
# extended with every configured plugin present in the SDK at configure time.
function(gstreamer_install_platform_sdk app_name)
    if(NOT app_name)
        message(FATAL_ERROR "gstreamer_install_platform_sdk: app_name is required")
    endif()
    cmake_parse_arguments(PARSE_ARGV 1 ARG "" "" "REQUIRED_PLUGINS")
    _qgc_gst_check_args(gstreamer_install_platform_sdk)
    qgc_gst_platform(_gips_platform)
    # QGC_GST_BUNDLE_PLUGINS=OFF (unset means ON): a Linux distribution package depends on the system's plugins, which
    # the app then uses as they are (GStreamerEnvironment.cc, QGC_GST_SYSTEM_PLUGINS).
    if(_gips_platform STREQUAL "linux"
       AND DEFINED QGC_GST_BUNDLE_PLUGINS
       AND NOT QGC_GST_BUNDLE_PLUGINS
    )
        message(STATUS "GStreamer: QGC_GST_BUNDLE_PLUGINS is OFF; the install uses the system's plugins")
        return()
    endif()
    # Homebrew's libraries, plugins and plugin scanner load each other by absolute paths under its prefix (no @rpath),
    # so copies bundled into the app would not be the ones loaded.
    gstreamer_layout_get(HOMEBREW_PREFIX _gips_homebrew_prefix)
    if(_gips_homebrew_prefix)
        set(_gips_message "GStreamer: this build uses Homebrew's GStreamer (${_gips_homebrew_prefix}), which cannot be")
        string(
            APPEND _gips_message " bundled. Configure with the official SDK (QGC_GST_USE_HOMEBREW=OFF and "
            "GStreamer_ROOT_DIR unset) to install or package."
        )
        install(CODE "message(FATAL_ERROR \"${_gips_message}\")")
        return()
    endif()
    if(NOT ARG_REQUIRED_PLUGINS)
        list(APPEND ARG_REQUIRED_PLUGINS ${GSTREAMER_BUNDLE_REQUIRED_PLUGINS})
    endif()

    # Extend verification beyond the structural floor to every configured plugin
    # present in the SDK at configure time, so an install-filter drop fails loudly.
    if(GSTREAMER_PLUGINS)
        gstreamer_scan_plugin_basenames(_gips_sdk_plugins "${GSTREAMER_PLUGIN_PATH}")
        foreach(_p IN LISTS GSTREAMER_PLUGINS)
            if(_p IN_LIST _gips_sdk_plugins)
                list(APPEND ARG_REQUIRED_PLUGINS "${_p}")
            endif()
        endforeach()
        list(REMOVE_DUPLICATES ARG_REQUIRED_PLUGINS)
    endif()

    cmake_language(CALL gstreamer_install_${_gips_platform}_sdk "${app_name}" _verify_dest)
    _qgc_gst_install_gpl_check()

    # Post-install plugin verification, against the plugin directory the installer reported.
    # Checked at install time against $ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}, where CPack stages packages. Install
    # scripts run with every policy unset, which PluginPolicy.cmake (IN_LIST) cannot use.
    gstreamer_platform_plugin_attrs(_verify_ext _verify_prefix)
    set(_verify_code
        [=[
cmake_policy(PUSH)
cmake_policy(VERSION 3.25)
include("@_policy_file@")
set(_verify_dir "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/@_verify_dest@")
file(GLOB _installed_plugins RELATIVE "${_verify_dir}" "${_verify_dir}/@_verify_prefix@*.@_verify_ext@")
list(TRANSFORM _installed_plugins REPLACE "^@_verify_prefix@(.*)\\.@_verify_ext@$" "\\1")
gstreamer_missing_required(REQUIRED @ARG_REQUIRED_PLUGINS@ AVAILABLE ${_installed_plugins} OUT_VAR _missing_plugins)
if(_missing_plugins)
    list(JOIN _missing_plugins ", " _missing_list)
    message(FATAL_ERROR "GStreamer install verification: missing required plugins in @_verify_dest@: "
                        "${_missing_list} — bundle is missing configured video plugins")
endif()
message(STATUS "GStreamer install verification: all required plugins present in @_verify_dest@")
cmake_policy(POP)
]=]
    )
    set(_policy_file "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/PluginPolicy.cmake")
    string(CONFIGURE "${_verify_code}" _verify_code @ONLY)
    install(CODE "${_verify_code}")
endfunction()
