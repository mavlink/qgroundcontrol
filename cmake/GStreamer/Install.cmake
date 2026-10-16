# Platform SDK install helpers — copy GStreamer plugins, GIO modules, helper
# binaries, and CA bundles into the install tree. Called from
# src/VideoManager/VideoReceiver/GStreamer/CMakeLists.txt after the ANDROID/IOS
# early-return.
# Depends on: Components.cmake (gstreamer_platform_plugin_attrs), Platform.cmake (qgc_gst_platform),
#             PluginPolicy.cmake (GSTREAMER_RUNTIME_REQUIRED_PLUGINS; included again by the install-time verifier).

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
# runs at configure time, so the SDK must be complete then.
function(gstreamer_install_plugins source_dir dest_dir prefix extension)
    file(GLOB _plugins "${source_dir}/${prefix}*.${extension}")
    _qgc_gst_filter_plugin_paths("${prefix}" "${_plugins}" _plugins)
    if(NOT _plugins)
        message(
            FATAL_ERROR "gstreamer_install_plugins: no requested plugin matches ${source_dir}/${prefix}*.${extension}. "
                        "Re-run cmake configure against a complete SDK."
        )
    endif()
    install(FILES ${_plugins} DESTINATION "${dest_dir}")
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
    if(_modules)
        install(FILES ${_modules} DESTINATION "${dest_dir}")
    endif()
endfunction()

# gstreamer_install_libs(<source_dir> <dest_dir> <extension>)
# Installs every *.<extension> library of an isolated SDK directory; refuses system prefixes.
function(gstreamer_install_libs source_dir dest_dir extension)
    _qgc_gst_is_blocked_lib_copy_source("${source_dir}" _blocked_lib_source)
    if(_blocked_lib_source)
        message(FATAL_ERROR "gstreamer_install_libs: refusing to copy from system/shared prefix '${source_dir}'.\n"
                            "This function copies ALL shared libraries unfiltered. Use an auto-downloaded SDK "
                            "or set GStreamer_ROOT_DIR to an isolated installation."
        )
    endif()
    # Deliberately copies every lib in the isolated SDK dir: shared transitive deps are not enumerable from the plugin
    # set, and the system-prefix guard above bounds the blast. On Windows this also ships unused codec DLLs.
    file(GLOB _libs "${source_dir}/*.${extension}")
    if(NOT _libs)
        message(FATAL_ERROR "gstreamer_install_libs: no *.${extension} found in '${source_dir}'. "
                            "The GStreamer SDK must be fully expanded at configure time before install."
        )
    endif()
    install(FILES ${_libs} DESTINATION "${dest_dir}")
endfunction()

# Sets <out_var> TRUE when <source_dir> is a system or shared prefix that gstreamer_install_libs must not copy from.
function(_qgc_gst_is_blocked_lib_copy_source source_dir out_var)
    set(_blocked /usr/lib /usr/local/lib /opt/homebrew/lib /opt/homebrew/opt)
    set(_allowed "")
    cmake_path(SET _source NORMALIZE "${source_dir}/")
    if(WIN32)
        list(APPEND _blocked "C:/Windows" "C:/Program Files" "C:/Program Files (x86)")
        # The official MSVC SDK installs here by default; it is an SDK root, not a shared system DLL directory.
        set(_allowed "C:/Program Files/gstreamer" "C:/Program Files (x86)/gstreamer")
        # Case-insensitive, so neither C:\Windows nor c:/windows bypasses the guard.
        string(TOLOWER "${_source}" _source)
        string(TOLOWER "${_blocked}" _blocked)
        string(TOLOWER "${_allowed}" _allowed)
    endif()
    set(_result FALSE)
    foreach(_prefix IN LISTS _blocked)
        string(FIND "${_source}" "${_prefix}/" _position)
        if(_position EQUAL 0)
            set(_result TRUE)
        endif()
    endforeach()
    foreach(_prefix IN LISTS _allowed)
        string(FIND "${_source}" "${_prefix}/" _position)
        if(_position EQUAL 0)
            set(_result FALSE)
        endif()
    endforeach()
    set(${out_var}
        "${_result}"
        PARENT_SCOPE
    )
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
function(gstreamer_install_linux_sdk app_name plugin_dest_out)
    set(_plugin_dest "lib/gstreamer-1.0")
    gstreamer_install_plugins("${GSTREAMER_PLUGIN_PATH}" "${_plugin_dest}" "libgst" "so")
    # glib-networking's GnuTLS backend and proxy resolvers (libproxy; GNOME's reads its settings through dconf). The
    # VFS modules (gvfsdbus, remote volume monitor) stay out: the bundle runs GIO with GIO_USE_VFS=local (AppRun, and
    # GStreamerEnvironment.cc on GLib 2.76+) or alongside the system's modules.
    gstreamer_install_gio_modules(
        "${GSTREAMER_LIB_PATH}/gio/modules"
        "lib/gio/modules"
        "so"
        giognutls
        giolibproxy
        giognomeproxy
        dconfsettings
    )
    # Helper binary path varies by distro: Debian lib/<multiarch>/gstreamer1.0/,
    # Fedora libexec/gstreamer-1.0/, Arch lib/gstreamer-1.0/.
    set(_gst_helper_search_paths "${GSTREAMER_LIB_PATH}/gstreamer1.0/gstreamer-1.0"
                                 "${GStreamer_ROOT_DIR}/libexec/gstreamer-1.0" "${GSTREAMER_PLUGIN_PATH}"
    )
    foreach(_helper IN ITEMS gst-plugin-scanner gst-ptp-helper)
        string(MAKE_C_IDENTIFIER "_GST_${_helper}_PROG" _helper_var)
        find_program(
            ${_helper_var} ${_helper}
            PATHS ${_gst_helper_search_paths}
            NO_DEFAULT_PATH
        )
        if(${_helper_var})
            install(PROGRAMS "${${_helper_var}}" DESTINATION "lib/gstreamer1.0/gstreamer-1.0")
        elseif(_helper STREQUAL "gst-plugin-scanner")
            message(WARNING "gst-plugin-scanner not found; AppImage video may not work")
        endif()
    endforeach()
    _qgc_gst_install_notice(share/licenses/gstreamer)
    set(${plugin_dest_out}
        "${_plugin_dest}"
        PARENT_SCOPE
    )
endfunction()

# Windows: the SDK and libproxy DLLs, GIO modules, plugins, libexec helpers, the CA bundle and the notice.
function(gstreamer_install_windows_sdk app_name plugin_dest_out)
    set(_plugin_dest "${CMAKE_INSTALL_LIBDIR}/gstreamer-1.0")
    gstreamer_install_libs("${GStreamer_ROOT_DIR}/bin" "${CMAKE_INSTALL_BINDIR}" "dll")
    # giolibproxy.dll loads proxy-1.dll, which loads its backends from lib/libproxy; startup only puts the app's bin
    # directory on PATH, so they go there too.
    file(GLOB _gst_libproxy_backends "${GStreamer_ROOT_DIR}/lib/libproxy/*.dll")
    if(_gst_libproxy_backends)
        install(FILES ${_gst_libproxy_backends} DESTINATION "${CMAKE_INSTALL_BINDIR}")
    endif()
    gstreamer_install_gio_modules("${GSTREAMER_LIB_PATH}/gio/modules" "${CMAKE_INSTALL_LIBDIR}/gio/modules" "dll")
    gstreamer_install_plugins("${GSTREAMER_PLUGIN_PATH}" "${_plugin_dest}" "gst" "dll")
    install(
        DIRECTORY "${GStreamer_ROOT_DIR}/libexec/gstreamer-1.0/"
        DESTINATION "${CMAKE_INSTALL_LIBEXECDIR}/gstreamer-1.0"
        FILE_PERMISSIONS OWNER_READ
                         OWNER_WRITE
                         OWNER_EXECUTE
                         GROUP_READ
                         GROUP_EXECUTE
                         WORLD_READ
                         WORLD_EXECUTE
        FILES_MATCHING
        PATTERN "*.exe"
    )
    # CA bundle for gioopenssl.dll — OpenSSL's compiled-in default path is
    # relative to the SDK root; ship the bundle so HTTPS/RTSPS sources can
    # verify certs after install (paired with SSL_CERT_FILE wiring at startup).
    if(EXISTS "${GStreamer_ROOT_DIR}/etc/ssl/certs/ca-certificates.crt")
        install(FILES "${GStreamer_ROOT_DIR}/etc/ssl/certs/ca-certificates.crt" DESTINATION etc/ssl/certs)
    endif()
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
        PATTERN "Commands" EXCLUDE
        REGEX ".*/lib/gstreamer-1.0/libgst.*" EXCLUDE
    )

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
    gstreamer_install_plugins("${_gst_framework_real}/Versions/1.0/lib/gstreamer-1.0" "${_plugin_dest}" "libgst"
                              "dylib"
    )
    set(${plugin_dest_out}
        "${_plugin_dest}"
        PARENT_SCOPE
    )
endfunction()

# Stage flat-layout dylibs/plugins/gio modules + rpath fixups + helpers + CA
# bundle. Used for a flat (downloaded or user-supplied) macOS GStreamer, which has no framework wrapper.
function(gstreamer_install_macos_flat_sdk app_name plugin_dest_out)
    # Relative to the install prefix; install(CODE) prepends $ENV{DESTDIR}${CMAKE_INSTALL_PREFIX} at install time.
    set(_mac_fw_dest "${app_name}.app/Contents/Frameworks")
    set(_mac_lib_dest "${app_name}.app/Contents/lib")
    set(_mac_libexec_dest "${app_name}.app/Contents/libexec/gstreamer-1.0")

    if(GStreamer_AUTO_DOWNLOADED)
        gstreamer_install_libs("${GSTREAMER_LIB_PATH}" "${_mac_fw_dest}" "dylib")
    else()
        file(
            GLOB
            _gst_libs
            "${GSTREAMER_LIB_PATH}/libgst*.dylib"
            "${GSTREAMER_LIB_PATH}/libglib*.dylib"
            "${GSTREAMER_LIB_PATH}/libgobject*.dylib"
            "${GSTREAMER_LIB_PATH}/libgmodule*.dylib"
            "${GSTREAMER_LIB_PATH}/libgthread*.dylib"
            "${GSTREAMER_LIB_PATH}/libgio*.dylib"
            "${GSTREAMER_LIB_PATH}/libintl*.dylib"
            "${GSTREAMER_LIB_PATH}/liborc*.dylib"
            "${GSTREAMER_LIB_PATH}/libffi*.dylib"
            "${GSTREAMER_LIB_PATH}/libpcre2*.dylib"
            "${GSTREAMER_LIB_PATH}/libgraphene*.dylib"
            "${GSTREAMER_LIB_PATH}/libssl*.dylib"
            "${GSTREAMER_LIB_PATH}/libcrypto*.dylib"
        )
        if(_gst_libs)
            install(FILES ${_gst_libs} DESTINATION "${_mac_fw_dest}")
        else()
            message(FATAL_ERROR "gstreamer_install_macos_flat_sdk: no GStreamer/GLib dylibs found in "
                                "'${GSTREAMER_LIB_PATH}'. The SDK lib dir must be populated at configure "
                                "time before install."
            )
        endif()
    endif()
    set(_plugin_dest "${_mac_lib_dest}/gstreamer-1.0")
    gstreamer_install_plugins("${GSTREAMER_PLUGIN_PATH}" "${_plugin_dest}" "libgst" "dylib")
    gstreamer_install_gio_modules("${GSTREAMER_LIB_PATH}/gio/modules" "${_mac_lib_dest}/gio/modules" "dylib")

    # Stage libexec helpers BEFORE the rpath fixup — install() runs in declaration
    # order, so staging later would leave nothing for the fixup to patch.
    if(EXISTS "${GStreamer_ROOT_DIR}/libexec/gstreamer-1.0")
        install(
            DIRECTORY "${GStreamer_ROOT_DIR}/libexec/gstreamer-1.0/"
            DESTINATION "${_mac_libexec_dest}"
            FILE_PERMISSIONS OWNER_READ
                             OWNER_WRITE
                             OWNER_EXECUTE
                             GROUP_READ
                             GROUP_EXECUTE
                             WORLD_READ
                             WORLD_EXECUTE
            FILES_MATCHING
            PATTERN "gst-plugin-scanner"
            PATTERN "gst-ptp-helper"
        )
    endif()

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

    # CA bundle for libgioopenssl.so — Cerbero's compiled-in OpenSSL trust
    # path doesn't exist on a user's machine; ship the bundle so the
    # SSL_CERT_FILE wiring at startup can point gioopenssl at it.
    if(EXISTS "${GStreamer_ROOT_DIR}/etc/ssl/certs/ca-certificates.crt")
        install(FILES "${GStreamer_ROOT_DIR}/etc/ssl/certs/ca-certificates.crt"
                DESTINATION "${app_name}.app/Contents/Resources/etc/ssl/certs"
        )
    endif()
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
        string(APPEND _gips_message " bundled. Configure with the official SDK (QGC_GST_USE_HOMEBREW=OFF and "
               "GStreamer_ROOT_DIR unset) to install or package."
        )
        install(CODE "message(FATAL_ERROR \"${_gips_message}\")")
        return()
    endif()
    if(NOT ARG_REQUIRED_PLUGINS)
        list(APPEND ARG_REQUIRED_PLUGINS ${GSTREAMER_RUNTIME_REQUIRED_PLUGINS})
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
