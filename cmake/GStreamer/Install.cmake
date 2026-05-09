# Platform SDK install helpers — copy GStreamer plugins, GIO modules, helper
# binaries, and CA bundles into the install tree. Called from
# src/VideoManager/VideoReceiver/GStreamer/CMakeLists.txt after the ANDROID/IOS
# early-return.
# Depends on: cmake/GStreamer/Components.cmake (gstreamer_platform_plugin_attrs).

# Shared install-from-glob helper. CALLER must specify GLOB_PATTERN; FILTER_PREFIX
# (optional) restricts results to files matching ${PREFIX}${name}.${EXT} where name
# is in GSTREAMER_PLUGINS. Used by the public install_* wrappers below.
function(_gstreamer_install_glob LABEL)
    cmake_parse_arguments(ARG "" "SOURCE_DIR;DEST_DIR;GLOB_PATTERN;FILTER_PREFIX" "" ${ARGN})
    if(NOT EXISTS "${ARG_SOURCE_DIR}")
        message(WARNING "${LABEL}: SOURCE_DIR does not exist: ${ARG_SOURCE_DIR}")
        return()
    endif()
    file(GLOB _matches "${ARG_SOURCE_DIR}/${ARG_GLOB_PATTERN}")
    if(ARG_FILTER_PREFIX)
        set(_filtered "")
        foreach(_path IN LISTS _matches)
            get_filename_component(_name "${_path}" NAME)
            foreach(_allowed IN LISTS GSTREAMER_PLUGINS)
                if(_name MATCHES "^${ARG_FILTER_PREFIX}${_allowed}([^a-zA-Z0-9]|$)")
                    list(APPEND _filtered "${_path}")
                    break()
                endif()
            endforeach()
        endforeach()
        set(_matches "${_filtered}")
    endif()
    if(_matches)
        install(FILES ${_matches} DESTINATION "${ARG_DEST_DIR}")
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
        FILTER_PREFIX "${ARG_PREFIX}")
endfunction()

function(gstreamer_install_libs)
    cmake_parse_arguments(ARG "" "SOURCE_DIR;DEST_DIR;EXTENSION" "" ${ARGN})

    if(NOT EXISTS "${ARG_SOURCE_DIR}")
        message(WARNING "gstreamer_install_libs: SOURCE_DIR does not exist: ${ARG_SOURCE_DIR}")
        return()
    endif()

    set(_blocked_prefixes
        /usr/lib /usr/local/lib /opt/homebrew/lib /opt/homebrew/opt
        "C:/Windows" "C:/Program Files" "C:/Program Files (x86)"
    )
    foreach(_prefix IN LISTS _blocked_prefixes)
        cmake_path(IS_PREFIX _prefix "${ARG_SOURCE_DIR}" NORMALIZE _is_system)
        if(_is_system)
            message(FATAL_ERROR
                "gstreamer_install_libs: refusing to copy from system/shared prefix '${ARG_SOURCE_DIR}'.\n"
                "This function copies ALL shared libraries unfiltered. Use an auto-downloaded SDK "
                "or set GStreamer_ROOT_DIR to an isolated installation.")
        endif()
    endforeach()

    file(GLOB _all_libs "${ARG_SOURCE_DIR}/*.${ARG_EXTENSION}")
    if(_all_libs)
        install(FILES ${_all_libs} DESTINATION "${ARG_DEST_DIR}")
    endif()
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

    foreach(_rpath_dir IN ITEMS
        "${_mac_lib_dest}/gstreamer-1.0:@loader_path/../../Frameworks"
        "${_mac_lib_dest}/gio/modules:@loader_path/../../../Frameworks"
        "${_mac_libexec_dest}:@loader_path/../../Frameworks"
    )
        string(REPLACE ":" ";" _parts "${_rpath_dir}")
        list(GET _parts 0 _dir)
        list(GET _parts 1 _rpath)
        install(CODE "
            file(GLOB _libs \"${_dir}/*\")
            foreach(_lib \${_libs})
                execute_process(
                    COMMAND install_name_tool -add_rpath ${_rpath} \"\${_lib}\"
                    ERROR_QUIET
                )
            endforeach()
        ")
    endforeach()

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
# verify exist post-install; default is the minimum needed to run any pipeline.
function(gstreamer_install_platform_sdk PROJECT_NAME)
    cmake_parse_arguments(GIPS "" "" "REQUIRED_PLUGINS" ${ARGN})
    if(NOT GIPS_REQUIRED_PLUGINS)
        gstreamer_runtime_required_plugins(GIPS_REQUIRED_PLUGINS)
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
        set(_required_check "")
        foreach(_p IN LISTS GIPS_REQUIRED_PLUGINS)
            list(APPEND _required_check "${_verify_prefix}${_p}.${_verify_ext}")
        endforeach()

        install(CODE "
            set(_missing_plugins)
            foreach(_plugin IN ITEMS ${_required_check})
                if(NOT EXISTS \"\${CMAKE_INSTALL_PREFIX}/${_verify_dest}/\${_plugin}\")
                    list(APPEND _missing_plugins \"\${_plugin}\")
                endif()
            endforeach()
            if(_missing_plugins)
                list(JOIN _missing_plugins \", \" _missing_list)
                message(FATAL_ERROR \"GStreamer install verification: missing required plugins in ${_verify_dest}: \${_missing_list} — built bundle cannot run any pipeline\")
            else()
                message(STATUS \"GStreamer install verification: all required plugins present in ${_verify_dest}\")
            endif()
        ")
    endif()
endfunction()
