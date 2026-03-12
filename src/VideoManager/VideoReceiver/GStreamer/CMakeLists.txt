target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(${CMAKE_PROJECT_NAME} PRIVATE VideoItemStub.h)

if(QGC_ENABLE_GST_VIDEOSTREAMING)
    find_package(QGCGStreamer
        REQUIRED
        COMPONENTS Core Base Video Gl GlPrototypes Rtsp
        OPTIONAL_COMPONENTS GlEgl GlWayland GlX11
    )

    target_sources(${CMAKE_PROJECT_NAME}
        PRIVATE
            GStreamer.cc
            GStreamer.h
            GStreamerHelpers.cc
            GStreamerHelpers.h
            GStreamerLogging.cc
            GStreamerLogging.h
            GstVideoReceiver.cc
            GstVideoReceiver.h
    )

    add_subdirectory(gstqgc)
endif()

if(NOT QGC_ENABLE_GST_VIDEOSTREAMING OR NOT TARGET gstqml6gl)
    return()
endif()

if(ANDROID OR IOS)
    return()
endif()

include(GStreamerHelpers)

if(MACOS AND GSTREAMER_FRAMEWORK)
    set_property(TARGET ${CMAKE_PROJECT_NAME} APPEND PROPERTY
        BUILD_RPATH "${GSTREAMER_LIB_PATH}")
endif()

if(LINUX)
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
    find_program(_GST_PLUGIN_SCANNER gst-plugin-scanner
        PATHS ${_gst_helper_search_paths} NO_DEFAULT_PATH)
    find_program(_GST_PTP_HELPER gst-ptp-helper
        PATHS ${_gst_helper_search_paths} NO_DEFAULT_PATH)
    if(_GST_PLUGIN_SCANNER)
        install(PROGRAMS "${_GST_PLUGIN_SCANNER}"
            DESTINATION "lib/gstreamer1.0/gstreamer-1.0")
    else()
        message(WARNING "gst-plugin-scanner not found; AppImage video may not work")
    endif()
    if(_GST_PTP_HELPER)
        install(PROGRAMS "${_GST_PTP_HELPER}"
            DESTINATION "lib/gstreamer1.0/gstreamer-1.0")
    endif()

elseif(WIN32)
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

elseif(MACOS)
    if(GSTREAMER_FRAMEWORK)
        get_filename_component(_gst_framework_real "${GSTREAMER_FRAMEWORK}" REALPATH)
        set(_gst_fw_dest "${CMAKE_INSTALL_PREFIX}/${CMAKE_PROJECT_NAME}.app/Contents/Frameworks/GStreamer.framework")

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

    else()
        # Non-framework layout: downloaded SDK, Homebrew, or other flat prefix
        set(_mac_fw_dest "${CMAKE_INSTALL_PREFIX}/${CMAKE_PROJECT_NAME}.app/Contents/Frameworks")
        set(_mac_lib_dest "${CMAKE_INSTALL_PREFIX}/${CMAKE_PROJECT_NAME}.app/Contents/lib")
        set(_mac_libexec_dest "${CMAKE_INSTALL_PREFIX}/${CMAKE_PROJECT_NAME}.app/Contents/libexec/gstreamer-1.0")

        # gstreamer_install_libs copies ALL dylibs from a directory unfiltered,
        # which is only safe for isolated SDK prefixes. For shared prefixes
        # (Homebrew), install only the GStreamer-related libraries.
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

        # Fix rpaths so dylibs find GStreamer libs in Frameworks/
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
    endif()
endif()

if(WIN32)
    set(_gst_verify_ext "dll")
    set(_gst_verify_prefix "gst")
    set(_gst_verify_dest "${CMAKE_INSTALL_LIBDIR}/gstreamer-1.0")
elseif(MACOS)
    set(_gst_verify_ext "dylib")
    set(_gst_verify_prefix "libgst")
    if(GSTREAMER_FRAMEWORK)
        set(_gst_verify_dest "${CMAKE_PROJECT_NAME}.app/Contents/Frameworks/GStreamer.framework/Versions/1.0/lib/gstreamer-1.0")
    else()
        set(_gst_verify_dest "${CMAKE_PROJECT_NAME}.app/Contents/lib/gstreamer-1.0")
    endif()
elseif(LINUX)
    set(_gst_verify_ext "so")
    set(_gst_verify_prefix "libgst")
    set(_gst_verify_dest "lib/gstreamer-1.0")
endif()

if(_gst_verify_dest)
    set(_gst_required_plugins coreelements playback rtsp rtp rtpmanager tcp udp)
    set(_gst_required_check "")
    foreach(_p IN LISTS _gst_required_plugins)
        list(APPEND _gst_required_check "${_gst_verify_prefix}${_p}.${_gst_verify_ext}")
    endforeach()

    install(CODE "
        set(_missing_plugins)
        foreach(_plugin IN ITEMS ${_gst_required_check})
            if(NOT EXISTS \"\${CMAKE_INSTALL_PREFIX}/${_gst_verify_dest}/\${_plugin}\")
                list(APPEND _missing_plugins \"\${_plugin}\")
            endif()
        endforeach()
        if(_missing_plugins)
            list(JOIN _missing_plugins \", \" _missing_list)
            message(WARNING \"GStreamer install verification: missing plugins in ${_gst_verify_dest}: \${_missing_list}\")
        else()
            message(STATUS \"GStreamer install verification: all required plugins present in ${_gst_verify_dest}\")
        endif()
    ")
endif()
