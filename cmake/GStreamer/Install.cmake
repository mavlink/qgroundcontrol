# Platform SDK install helpers — copy GStreamer plugins, GIO modules, helper
# binaries, and CA bundles into the install tree. Called from
# src/VideoManager/VideoReceiver/GStreamer/CMakeLists.txt after the ANDROID/IOS
# early-return.
# Depends on: Components.cmake (gstreamer_platform_plugin_attrs),
#             PluginPolicy.cmake (gstreamer_runtime_required_plugins, gstreamer_plugin_satisfy_sets).

# Shared install-from-glob helper. CALLER must specify GLOB_PATTERN; FILTER_PREFIX
# (optional) restricts results to files matching ${PREFIX}${name}.${EXT} where name
# is in GSTREAMER_PLUGINS. Used by the public install_* wrappers below.
# Invariant: GLOB is evaluated at configure time, so the SDK must already be
# expanded before any install_* wrapper runs; re-running cmake --install without
# re-configure reuses this stale list.

# Pure (script-testable) plugin-name filter: keep only paths whose basename is
# ${PREFIX}<name>(boundary) where <name> is in GSTREAMER_PLUGINS. The trailing
# boundary stops gstvideo matching gstvideoconvert.
function(_gstreamer_filter_plugin_paths PREFIX PATHS OUT_VAR)
    set(_filtered "")
    foreach(_path IN LISTS PATHS)
        get_filename_component(_name "${_path}" NAME)
        foreach(_allowed IN LISTS GSTREAMER_PLUGINS)
            string(REGEX REPLACE "([][.+*?^$()|\\\\])" "\\\\\\1" _allowed_re "${_allowed}")
            if(_name MATCHES "^${PREFIX}${_allowed_re}([^a-zA-Z0-9]|$)")
                list(APPEND _filtered "${_path}")
                break()
            endif()
        endforeach()
    endforeach()
    set(${OUT_VAR} "${_filtered}" PARENT_SCOPE)
endfunction()

function(_gstreamer_install_glob LABEL)
    cmake_parse_arguments(ARG "REQUIRE_NONEMPTY" "SOURCE_DIR;DEST_DIR;GLOB_PATTERN;FILTER_PREFIX" "" ${ARGN})
    if(NOT EXISTS "${ARG_SOURCE_DIR}")
        message(WARNING "${LABEL}: SOURCE_DIR does not exist: ${ARG_SOURCE_DIR}")
        return()
    endif()
    file(GLOB _matches "${ARG_SOURCE_DIR}/${ARG_GLOB_PATTERN}")
    if(ARG_FILTER_PREFIX)
        _gstreamer_filter_plugin_paths("${ARG_FILTER_PREFIX}" "${_matches}" _matches)
    endif()
    if(_matches)
        install(FILES ${_matches} DESTINATION "${ARG_DEST_DIR}")
    elseif(ARG_REQUIRE_NONEMPTY)
        # Empty here means a missing/unexpanded SDK — fail loudly rather than ship
        # a video build with zero plugins.
        message(FATAL_ERROR
            "${LABEL}: glob '${ARG_GLOB_PATTERN}' in '${ARG_SOURCE_DIR}' matched no files. "
            "The GStreamer SDK must be fully expanded at configure time before install. "
            "Re-run cmake configure against a complete SDK.")
    endif()
endfunction()

function(gstreamer_install_gio_modules)
    cmake_parse_arguments(ARG "" "SOURCE_DIR;DEST_DIR;EXTENSION" "" ${ARGN})
    _gstreamer_install_glob("gstreamer_install_gio_modules"
        SOURCE_DIR "${ARG_SOURCE_DIR}" DEST_DIR "${ARG_DEST_DIR}"
        GLOB_PATTERN "*.${ARG_EXTENSION}")
endfunction()

function(gstreamer_install_plugins)
    cmake_parse_arguments(ARG "" "SOURCE_DIR;DEST_DIR;EXTENSION;PREFIX" "" ${ARGN})
    _gstreamer_install_glob("gstreamer_install_plugins"
        SOURCE_DIR "${ARG_SOURCE_DIR}" DEST_DIR "${ARG_DEST_DIR}"
        GLOB_PATTERN "${ARG_PREFIX}*.${ARG_EXTENSION}"
        FILTER_PREFIX "${ARG_PREFIX}"
        REQUIRE_NONEMPTY)
endfunction()

function(gstreamer_install_libs)
    cmake_parse_arguments(ARG "" "SOURCE_DIR;DEST_DIR;EXTENSION" "" ${ARGN})

    if(NOT EXISTS "${ARG_SOURCE_DIR}")
        message(WARNING "gstreamer_install_libs: SOURCE_DIR does not exist: ${ARG_SOURCE_DIR}")
        return()
    endif()

    _gstreamer_is_blocked_lib_copy_source("${ARG_SOURCE_DIR}" _blocked_lib_source)
    if(_blocked_lib_source)
        message(FATAL_ERROR
            "gstreamer_install_libs: refusing to copy from system/shared prefix '${ARG_SOURCE_DIR}'.\n"
            "This function copies ALL shared libraries unfiltered. Use an auto-downloaded SDK "
            "or set GStreamer_ROOT_DIR to an isolated installation.")
    endif()

    # Deliberately copies every lib in the isolated SDK dir — shared transitive deps are
    # not enumerable from the plugin set, and the system-prefix guard above bounds the blast.
    # Note: on Windows this also ships unused codec DLLs from the SDK bin/ dir (bloat,
    # attack surface) — acceptable tradeoff for not maintaining a transitive-dep allowlist.
    file(GLOB _all_libs "${ARG_SOURCE_DIR}/*.${ARG_EXTENSION}")
    if(_all_libs)
        install(FILES ${_all_libs} DESTINATION "${ARG_DEST_DIR}")
    else()
        # An isolated SDK lib/bin dir with zero matching libraries means the SDK
        # is missing or not yet expanded — fail rather than ship a libless bundle.
        message(FATAL_ERROR
            "gstreamer_install_libs: no *.${ARG_EXTENSION} found in '${ARG_SOURCE_DIR}'. "
            "The GStreamer SDK must be fully expanded at configure time before install.")
    endif()
endfunction()

function(_gstreamer_windows_runtime_dependency_dirs ROOT_DIR OUT_VAR)
    set(_runtime_dirs)

    # giolibproxy.dll loads proxy-1.dll, which loads pxbackend-1.0.dll from
    # lib/libproxy in the official Windows SDK. Install those backend DLLs next
    # to the executable because startup only prepends the app bin dir to PATH.
    foreach(_relative_dir IN ITEMS "lib/libproxy")
        set(_candidate_dir "${ROOT_DIR}/${_relative_dir}")
        file(GLOB _candidate_dlls "${_candidate_dir}/*.dll")
        if(_candidate_dlls)
            list(APPEND _runtime_dirs "${_candidate_dir}")
        endif()
    endforeach()

    set(${OUT_VAR} "${_runtime_dirs}" PARENT_SCOPE)
endfunction()

function(_gstreamer_is_blocked_lib_copy_source SOURCE_DIR OUT_VAR)
    set(_blocked_prefixes
        /usr/lib /usr/local/lib /opt/homebrew/lib /opt/homebrew/opt)
    set(_allowed_prefixes)
    set(_is_windows_guard ${WIN32})
    if(QGC_GSTREAMER_TEST_WIN32)
        set(_is_windows_guard TRUE)
    endif()

    if(_is_windows_guard)
        list(APPEND _blocked_prefixes
            "C:/Windows" "C:/Program Files" "C:/Program Files (x86)")
        # The official GStreamer MSVC SDK installs here by default; it is an SDK
        # root, not an arbitrary shared system DLL directory.
        list(APPEND _allowed_prefixes
            "C:/Program Files/gstreamer" "C:/Program Files (x86)/gstreamer")
    endif()

    # Normalize case + separators so the guard isn't bypassed by C:\Windows or c:/windows.
    cmake_path(SET _src_norm NORMALIZE "${SOURCE_DIR}")
    if(_is_windows_guard)
        string(TOLOWER "${_src_norm}" _src_norm)
    endif()

    foreach(_prefix IN LISTS _allowed_prefixes)
        if(_is_windows_guard)
            string(TOLOWER "${_prefix}" _prefix)
            string(FIND "${_src_norm}/" "${_prefix}/" _is_allowed_pos)
            if(_is_allowed_pos EQUAL 0)
                set(${OUT_VAR} FALSE PARENT_SCOPE)
                return()
            endif()
            continue()
        endif()
        cmake_path(IS_PREFIX _prefix "${_src_norm}" NORMALIZE _is_allowed)
        if(_is_allowed)
            set(${OUT_VAR} FALSE PARENT_SCOPE)
            return()
        endif()
    endforeach()

    foreach(_prefix IN LISTS _blocked_prefixes)
        if(_is_windows_guard)
            string(TOLOWER "${_prefix}" _prefix)
            string(FIND "${_src_norm}/" "${_prefix}/" _is_system_pos)
            if(_is_system_pos EQUAL 0)
                set(${OUT_VAR} TRUE PARENT_SCOPE)
                return()
            endif()
            continue()
        endif()
        cmake_path(IS_PREFIX _prefix "${_src_norm}" NORMALIZE _is_system)
        if(_is_system)
            set(${OUT_VAR} TRUE PARENT_SCOPE)
            return()
        endif()
    endforeach()
    set(${OUT_VAR} FALSE PARENT_SCOPE)
endfunction()

# ─────────────────────────────────────────────────────────────────────────────
# Per-platform install helpers. Each function uses variables set by
# cmake/GStreamer/Orchestrator.cmake (GSTREAMER_*_PATH, GStreamer_ROOT_DIR,
# GStreamer_AUTO_DOWNLOADED, etc.).
# ─────────────────────────────────────────────────────────────────────────────

function(gstreamer_install_linux_sdk)
    gstreamer_install_plugins(
        SOURCE_DIR "${GSTREAMER_PLUGIN_PATH}"
        DEST_DIR "lib/gstreamer-1.0"
        EXTENSION "so"
        PREFIX "libgst"
    )
    gstreamer_install_gio_modules(
        SOURCE_DIR "${GSTREAMER_LIB_PATH}/gio/modules"
        DEST_DIR "lib/gio/modules"
        EXTENSION "so"
    )
    # Helper binary path varies by distro: Debian lib/<multiarch>/gstreamer1.0/,
    # Fedora libexec/gstreamer-1.0/, Arch lib/gstreamer-1.0/.
    set(_gst_helper_search_paths
        "${GSTREAMER_LIB_PATH}/gstreamer1.0/gstreamer-1.0"
        "${GStreamer_ROOT_DIR}/libexec/gstreamer-1.0"
        "${GSTREAMER_PLUGIN_PATH}"
    )
    foreach(_helper IN ITEMS gst-plugin-scanner gst-ptp-helper)
        string(MAKE_C_IDENTIFIER "_GST_${_helper}_PROG" _helper_var)
        find_program(${_helper_var} ${_helper}
            PATHS ${_gst_helper_search_paths} NO_DEFAULT_PATH)
        if(${_helper_var})
            install(PROGRAMS "${${_helper_var}}"
                DESTINATION "lib/gstreamer1.0/gstreamer-1.0")
        elseif(_helper STREQUAL "gst-plugin-scanner")
            message(WARNING "gst-plugin-scanner not found; AppImage video may not work")
        endif()
    endforeach()
endfunction()

function(gstreamer_install_windows_sdk)
    gstreamer_install_libs(
        SOURCE_DIR "${GStreamer_ROOT_DIR}/bin"
        DEST_DIR "${CMAKE_INSTALL_BINDIR}"
        EXTENSION "dll"
    )
    _gstreamer_windows_runtime_dependency_dirs("${GStreamer_ROOT_DIR}" _gst_win_runtime_dependency_dirs)
    foreach(_dependency_dir IN LISTS _gst_win_runtime_dependency_dirs)
        gstreamer_install_libs(
            SOURCE_DIR "${_dependency_dir}"
            DEST_DIR "${CMAKE_INSTALL_BINDIR}"
            EXTENSION "dll"
        )
    endforeach()
    gstreamer_install_gio_modules(
        SOURCE_DIR "${GSTREAMER_LIB_PATH}/gio/modules"
        DEST_DIR "${CMAKE_INSTALL_LIBDIR}/gio/modules"
        EXTENSION "dll"
    )
    gstreamer_install_plugins(
        SOURCE_DIR "${GSTREAMER_PLUGIN_PATH}"
        DEST_DIR "${CMAKE_INSTALL_LIBDIR}/gstreamer-1.0"
        EXTENSION "dll"
        PREFIX "gst"
    )
    install(
        DIRECTORY "${GStreamer_ROOT_DIR}/libexec/gstreamer-1.0/"
        DESTINATION "${CMAKE_INSTALL_LIBEXECDIR}/gstreamer-1.0"
        FILE_PERMISSIONS
            OWNER_READ OWNER_WRITE OWNER_EXECUTE
            GROUP_READ             GROUP_EXECUTE
            WORLD_READ             WORLD_EXECUTE
        FILES_MATCHING
            PATTERN "*.exe"
    )
    # CA bundle for gioopenssl.dll — OpenSSL's compiled-in default path is
    # relative to the SDK root; ship the bundle so HTTPS/RTSPS sources can
    # verify certs after install (paired with SSL_CERT_FILE wiring at startup).
    if(EXISTS "${GStreamer_ROOT_DIR}/etc/ssl/certs/ca-certificates.crt")
        install(
            FILES "${GStreamer_ROOT_DIR}/etc/ssl/certs/ca-certificates.crt"
            DESTINATION etc/ssl/certs
        )
    endif()
endfunction()

# Stage GStreamer.framework into the app bundle's Contents/Frameworks/. Used
# when the SDK is consumed via the Cerbero macOS framework (Layout target
# stashed FRAMEWORK_BUNDLE).
function(gstreamer_install_macos_framework_sdk PROJECT_NAME)
    gstreamer_layout_get(FRAMEWORK_BUNDLE _gst_framework_bundle)
    if(NOT _gst_framework_bundle)
        message(FATAL_ERROR "gstreamer_install_macos_framework_sdk: FRAMEWORK_BUNDLE not set on GStreamer::Layout")
    endif()
    get_filename_component(_gst_framework_real "${_gst_framework_bundle}" REALPATH)
    set(_gst_fw_dest "${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app/Contents/Frameworks/GStreamer.framework")

    if(NOT _gst_fw_dest MATCHES "\\.app/Contents/Frameworks/GStreamer\\.framework$")
        message(FATAL_ERROR "gstreamer_install_macos_framework_sdk: refusing REMOVE_RECURSE on unexpected dest '${_gst_fw_dest}'")
    endif()
    install(CODE "file(REMOVE_RECURSE \"${_gst_fw_dest}\")")
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

    install(CODE "
        set(_fw \"${_gst_fw_dest}\")
        if(NOT EXISTS \"\${_fw}/Versions/1.0/GStreamer\")
            message(FATAL_ERROR \"GStreamer framework: Versions/1.0/GStreamer not found — SDK layout may have changed\")
        endif()
        execute_process(COMMAND \${CMAKE_COMMAND} -E create_symlink 1.0 \"\${_fw}/Versions/Current\")
        execute_process(COMMAND \${CMAKE_COMMAND} -E create_symlink Versions/Current/GStreamer \"\${_fw}/GStreamer\")
        if(IS_DIRECTORY \"\${_fw}/Versions/1.0/Resources\")
            execute_process(COMMAND \${CMAKE_COMMAND} -E create_symlink Versions/Current/Resources \"\${_fw}/Resources\")
        endif()
    ")

    gstreamer_install_plugins(
        SOURCE_DIR "${_gst_framework_real}/Versions/1.0/lib/gstreamer-1.0"
        DEST_DIR "${_gst_fw_dest}/Versions/1.0/lib/gstreamer-1.0"
        EXTENSION "dylib"
        PREFIX "libgst"
    )
endfunction()

# Stage flat-layout dylibs/plugins/gio modules + rpath fixups + helpers + CA
# bundle. Used for Homebrew/CPM-downloaded macOS GStreamer (no framework wrapper).
function(gstreamer_install_macos_flat_sdk PROJECT_NAME)
    set(_mac_fw_dest "${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app/Contents/Frameworks")
    set(_mac_lib_dest "${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app/Contents/lib")
    set(_mac_libexec_dest "${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app/Contents/libexec/gstreamer-1.0")

    if(GStreamer_AUTO_DOWNLOADED)
        gstreamer_install_libs(
            SOURCE_DIR "${GSTREAMER_LIB_PATH}"
            DEST_DIR "${_mac_fw_dest}"
            EXTENSION "dylib"
        )
    else()
        file(GLOB _gst_libs
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
            message(FATAL_ERROR
                "gstreamer_install_macos_flat_sdk: no GStreamer/GLib dylibs found in "
                "'${GSTREAMER_LIB_PATH}'. The SDK lib dir must be populated at configure "
                "time before install.")
        endif()
    endif()
    gstreamer_install_plugins(
        SOURCE_DIR "${GSTREAMER_PLUGIN_PATH}"
        DEST_DIR "${_mac_lib_dest}/gstreamer-1.0"
        EXTENSION "dylib"
        PREFIX "libgst"
    )
    gstreamer_install_gio_modules(
        SOURCE_DIR "${GSTREAMER_LIB_PATH}/gio/modules"
        DEST_DIR "${_mac_lib_dest}/gio/modules"
        EXTENSION "dylib"
    )

    # Stage libexec helpers BEFORE the rpath fixup — install() runs in declaration
    # order, so staging later would leave nothing for the fixup to patch.
    if(EXISTS "${GStreamer_ROOT_DIR}/libexec/gstreamer-1.0")
        install(
            DIRECTORY "${GStreamer_ROOT_DIR}/libexec/gstreamer-1.0/"
            DESTINATION "${_mac_libexec_dest}"
            FILE_PERMISSIONS
                OWNER_READ OWNER_WRITE OWNER_EXECUTE
                GROUP_READ             GROUP_EXECUTE
                WORLD_READ             WORLD_EXECUTE
            FILES_MATCHING
                PATTERN "gst-plugin-scanner"
                PATTERN "gst-ptp-helper"
        )
    endif()

    # Entry "<dir>:<rpath>:<glob>" — libexec helpers are extensionless, so * not *.dylib.
    foreach(_rpath_dir IN ITEMS
        "${_mac_lib_dest}/gstreamer-1.0:@loader_path/../../Frameworks:*.dylib"
        "${_mac_lib_dest}/gio/modules:@loader_path/../../../Frameworks:*.dylib"
        "${_mac_libexec_dest}:@loader_path/../../Frameworks:*"
    )
        string(REPLACE ":" ";" _parts "${_rpath_dir}")
        list(GET _parts 0 _dir)
        list(GET _parts 1 _rpath)
        list(GET _parts 2 _glob)
        install(CODE "
            file(GLOB _libs \"${_dir}/${_glob}\")
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
                    message(WARNING \"GStreamer: otool -l failed for \${_lib} (rc=\${_otool_rc}): \${_otool_err}; skipping rpath fixup\")
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
        ")
    endforeach()

    # CA bundle for libgioopenssl.so — Cerbero's compiled-in OpenSSL trust
    # path doesn't exist on a user's machine; ship the bundle so the
    # SSL_CERT_FILE wiring at startup can point gioopenssl at it.
    if(EXISTS "${GStreamer_ROOT_DIR}/etc/ssl/certs/ca-certificates.crt")
        install(
            FILES "${GStreamer_ROOT_DIR}/etc/ssl/certs/ca-certificates.crt"
            DESTINATION "${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app/Contents/Resources/etc/ssl/certs"
        )
    endif()
endfunction()

# Dispatch to the framework or flat installer based on whether the Layout
# target stashed FRAMEWORK_BUNDLE (set when GStreamer.framework is in use).
function(gstreamer_install_macos_sdk PROJECT_NAME)
    gstreamer_layout_get(FRAMEWORK_BUNDLE _gst_framework_bundle)
    if(_gst_framework_bundle)
        gstreamer_install_macos_framework_sdk("${PROJECT_NAME}")
    else()
        gstreamer_install_macos_flat_sdk("${PROJECT_NAME}")
    endif()
endfunction()

# Top-level dispatcher — call this from CMakeLists.txt after the ANDROID/IOS guard.
# PROJECT_NAME is CMAKE_PROJECT_NAME in the caller; passed explicitly to avoid
# macro-scope shadowing issues inside functions.
# gstreamer_install_platform_sdk(PROJECT_NAME [REQUIRED_PLUGINS <names…>])
# REQUIRED_PLUGINS: list of plugin basenames (no platform prefix or extension) to
# verify exist post-install; default is the minimum needed to run any pipeline,
# extended with every configured plugin present in the SDK at configure time.
function(gstreamer_install_platform_sdk PROJECT_NAME)
    cmake_parse_arguments(GIPS "" "" "REQUIRED_PLUGINS" ${ARGN})
    if(NOT GIPS_REQUIRED_PLUGINS)
        gstreamer_runtime_required_plugins(GIPS_REQUIRED_PLUGINS)
    endif()

    # Recover the SDK dirs from the GLOBAL Layout target when the Orchestrator's
    # PARENT_SCOPE propagation didn't reach this scope — otherwise the per-platform
    # installers below silently glob empty dirs. Locals here inherit into them.
    if(NOT GSTREAMER_LIB_PATH)
        gstreamer_layout_get(LIB_DIR GSTREAMER_LIB_PATH)
    endif()
    if(NOT GSTREAMER_PLUGIN_PATH)
        gstreamer_layout_get(PLUGIN_DIR GSTREAMER_PLUGIN_PATH)
    endif()
    if(NOT GSTREAMER_INCLUDE_PATH)
        gstreamer_layout_get(INCLUDE_DIR GSTREAMER_INCLUDE_PATH)
    endif()

    # Extend verification beyond the structural floor to every configured plugin
    # present in the SDK at configure time, so an install-filter drop fails loudly.
    if(GSTREAMER_PLUGINS)
        gstreamer_scan_plugin_basenames(_gips_sdk_plugins "${GSTREAMER_PLUGIN_PATH}")
        foreach(_p IN LISTS GSTREAMER_PLUGINS)
            if(_p IN_LIST _gips_sdk_plugins)
                list(APPEND GIPS_REQUIRED_PLUGINS "${_p}")
            endif()
        endforeach()
        list(REMOVE_DUPLICATES GIPS_REQUIRED_PLUGINS)
    endif()

    if(LINUX)
        gstreamer_install_linux_sdk()
    elseif(WIN32)
        gstreamer_install_windows_sdk()
    elseif(MACOS)
        gstreamer_install_macos_sdk("${PROJECT_NAME}")
    endif()

    # Post-install plugin verification (all desktop platforms).
    gstreamer_platform_plugin_attrs(_verify_ext _verify_prefix _verify_glob)
    if(WIN32)
        set(_verify_dest "${CMAKE_INSTALL_LIBDIR}/gstreamer-1.0")
    elseif(MACOS)
        gstreamer_layout_get(FRAMEWORK_BUNDLE _verify_fw)
        if(_verify_fw)
            set(_verify_dest "${PROJECT_NAME}.app/Contents/Frameworks/GStreamer.framework/Versions/1.0/lib/gstreamer-1.0")
        else()
            set(_verify_dest "${PROJECT_NAME}.app/Contents/lib/gstreamer-1.0")
        endif()
    elseif(LINUX)
        set(_verify_dest "lib/gstreamer-1.0")
    endif()

    if(_verify_dest)
        # Build an OR-of-AND existence test per required plugin so an alternate
        # set (videoconvert+videoscale on 1.20) satisfies the fused name
        # (videoconvertscale on 1.22+) instead of tripping the verifier.
        set(_verify_blocks "")
        foreach(_p IN LISTS GIPS_REQUIRED_PLUGINS)
            gstreamer_plugin_satisfy_sets(PLUGIN "${_p}" OUT_VAR _sets)
            set(_checks "")
            set(_alt_labels "")
            foreach(_set IN LISTS _sets)
                string(REPLACE "+" ";" _members "${_set}")
                set(_conds "")
                foreach(_m IN LISTS _members)
                    # Honor DESTDIR: CPack stages into $ENV{DESTDIR}, so this check must
                    # prepend it like install(FILES) does, or DEB/RPM/pacman packaging false-fails.
                    list(APPEND _conds "EXISTS \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${_verify_dest}/${_verify_prefix}${_m}.${_verify_ext}\"")
                endforeach()
                string(JOIN " AND " _and_cond ${_conds})
                string(APPEND _checks "                if(${_and_cond})\n                    set(_ok TRUE)\n                endif()\n")
                list(APPEND _alt_labels "${_set}")
            endforeach()
            string(JOIN " or " _plugin_label ${_alt_labels})
            string(APPEND _verify_blocks "            set(_ok FALSE)\n${_checks}            if(NOT _ok)\n                list(APPEND _missing_plugins \"${_plugin_label}\")\n            endif()\n")
        endforeach()

        install(CODE "
            set(_missing_plugins)
${_verify_blocks}
            if(_missing_plugins)
                list(JOIN _missing_plugins \", \" _missing_list)
                message(FATAL_ERROR \"GStreamer install verification: missing required plugins in ${_verify_dest}: \${_missing_list} — bundle is missing configured video plugins\")
            else()
                message(STATUS \"GStreamer install verification: all required plugins present in ${_verify_dest}\")
            endif()
        ")
    endif()
endfunction()
