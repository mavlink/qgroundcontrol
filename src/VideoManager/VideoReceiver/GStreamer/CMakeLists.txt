target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(${CMAKE_PROJECT_NAME} PRIVATE VideoItemStub.h)

if(QGC_ENABLE_GST_VIDEOSTREAMING)
    find_package(QGCGStreamer
        REQUIRED
        COMPONENTS Core Base Video Gl GlPrototypes Rtsp
        OPTIONAL_COMPONENTS GlEgl GlWayland GlX11
    )

    if(GStreamer_USE_STATIC_LIBS AND NOT (ANDROID OR IOS))
        foreach(_plugin IN LISTS GSTREAMER_PLUGINS)
            if(GST_PLUGIN_${_plugin}_FOUND)
                target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE GST_PLUGIN_${_plugin}_FOUND)
            endif()
        endforeach()
    endif()

    add_subdirectory(gstqml6gl)

    # TODO: Add Qt6 Direct3D11 plugin support
    # https://gstreamer.freedesktop.org/documentation/qt6d3d11/index.html#qml6d3d11sink-page
endif()

if(TARGET gstqml6gl)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE gstqml6gl)

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

    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE QGC_GST_STREAMING)

    if(LINUX)
        # Fixed "lib" to match AppRun expectations (CMAKE_INSTALL_LIBDIR varies by distro)
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
        # We install to Debian convention (lib/gstreamer1.0/gstreamer-1.0/);
        # runtime env var GST_PLUGIN_SCANNER in GStreamer.cc matches this path.
        find_program(_GST_PLUGIN_SCANNER gst-plugin-scanner
            PATHS
                "${GSTREAMER_LIB_PATH}/gstreamer1.0/gstreamer-1.0"
                "${GStreamer_ROOT_DIR}/libexec/gstreamer-1.0"
                "${GSTREAMER_PLUGIN_PATH}"
            NO_DEFAULT_PATH
        )
        if(_GST_PLUGIN_SCANNER)
            install(PROGRAMS "${_GST_PLUGIN_SCANNER}"
                DESTINATION "lib/gstreamer1.0/gstreamer-1.0")
        else()
            message(WARNING "gst-plugin-scanner not found; AppImage video may not work")
        endif()
        find_program(_GST_PTP_HELPER gst-ptp-helper
            PATHS
                "${GSTREAMER_LIB_PATH}/gstreamer1.0/gstreamer-1.0"
                "${GStreamer_ROOT_DIR}/libexec/gstreamer-1.0"
                "${GSTREAMER_PLUGIN_PATH}"
            NO_DEFAULT_PATH
        )
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
        # Framework install preserves internal structure/symlinks required for codesigning.
        # Plugin dylibs in lib/gstreamer-1.0/ are filtered to the GSTREAMER_PLUGINS
        # allowlist — they are loose files, not part of the framework's sealed structure.
        if(GSTREAMER_FRAMEWORK)
            # Resolve the framework real path before install. Some SDK layouts expose
            # the framework as a symlink; copying through the symlink can leave an
            # unsigned wrapper path inside the app bundle.
            get_filename_component(_gst_framework_real "${GSTREAMER_FRAMEWORK}" REALPATH)
            # Copy only the versioned content — avoids non-standard root-level
            # entries (Commands, Libraries, etc.) that codesign rejects as
            # "unsealed contents". Plugin dylibs are excluded and installed
            # selectively below.
            set(_gst_fw_dest "${CMAKE_INSTALL_PREFIX}/${CMAKE_PROJECT_NAME}.app/Contents/Frameworks/GStreamer.framework")
            # Clean destination so stale files from previous installs don't survive
            install(CODE "file(REMOVE_RECURSE \"${_gst_fw_dest}\")")
            install(
                DIRECTORY "${_gst_framework_real}/Versions/1.0/"
                DESTINATION "${_gst_fw_dest}/Versions/1.0"
                USE_SOURCE_PERMISSIONS
                PATTERN "*.la" EXCLUDE
                PATTERN "*.a" EXCLUDE
                PATTERN "*/bin" EXCLUDE
                PATTERN "*/etc" EXCLUDE
                PATTERN "*/gst-validate-launcher" EXCLUDE
                PATTERN "*/Headers" EXCLUDE
                PATTERN "*/include" EXCLUDE
                PATTERN "*/pkgconfig" EXCLUDE
                PATTERN "*/share" EXCLUDE
                PATTERN "*gstpython*" EXCLUDE
                PATTERN "Commands" EXCLUDE
                REGEX ".*/lib/gstreamer-1.0/libgst.*" EXCLUDE
            )

            # Construct standard framework symlinks
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

            # Install only the plugins QGC needs. Fewer dylibs means fewer
            # codesign calls, faster notarization, and a smaller DMG.
            gstreamer_install_plugins(
                SOURCE_DIR "${_gst_framework_real}/Versions/1.0/lib/gstreamer-1.0"
                DEST_DIR "${_gst_fw_dest}/Versions/1.0/lib/gstreamer-1.0"
                EXTENSION "dylib"
                PREFIX "libgst"
            )
        elseif(GStreamer_AUTO_DOWNLOADED)
            # Auto-downloaded SDK (non-framework): install dylibs into Frameworks
            # and plugins/gio into Contents/lib to keep bare directories out of
            # Frameworks (codesign rejects non-.framework directories there).
            # Helper binaries in libexec get rpaths fixed post-install below.
            set(_mac_fw_dest "${CMAKE_INSTALL_PREFIX}/${CMAKE_PROJECT_NAME}.app/Contents/Frameworks")
            set(_mac_lib_dest "${CMAKE_INSTALL_PREFIX}/${CMAKE_PROJECT_NAME}.app/Contents/lib")
            gstreamer_install_libs(
                SOURCE_DIR "${GSTREAMER_LIB_PATH}"
                DEST_DIR "${_mac_fw_dest}"
                EXTENSION "dylib"
            )
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
            set(_gst_plugins_dest "${_mac_lib_dest}/gstreamer-1.0")
            set(_gst_gio_dest "${_mac_lib_dest}/gio/modules")
            install(CODE "
                file(GLOB _gst_plugin_libs \"${_gst_plugins_dest}/*.dylib\")
                foreach(_lib \${_gst_plugin_libs})
                    execute_process(
                        COMMAND install_name_tool -add_rpath @loader_path/../../Frameworks \"\${_lib}\"
                        ERROR_QUIET
                    )
                endforeach()
            ")
            install(CODE "
                file(GLOB _gst_gio_libs \"${_gst_gio_dest}/*.dylib\")
                foreach(_lib \${_gst_gio_libs})
                    execute_process(
                        COMMAND install_name_tool -add_rpath @loader_path/../../../Frameworks \"\${_lib}\"
                        ERROR_QUIET
                    )
                endforeach()
            ")
            if(EXISTS "${GStreamer_ROOT_DIR}/libexec/gstreamer-1.0")
                set(_gst_libexec_dest "${CMAKE_INSTALL_PREFIX}/${CMAKE_PROJECT_NAME}.app/Contents/libexec/gstreamer-1.0")
                install(
                    DIRECTORY "${GStreamer_ROOT_DIR}/libexec/gstreamer-1.0/"
                    DESTINATION "${_gst_libexec_dest}"
                    FILE_PERMISSIONS
                        OWNER_READ OWNER_WRITE OWNER_EXECUTE
                        GROUP_READ             GROUP_EXECUTE
                        WORLD_READ             WORLD_EXECUTE
                    FILES_MATCHING
                        PATTERN "gst-plugin-scanner"
                        PATTERN "gst-ptp-helper"
                )
                install(CODE "
                    file(GLOB _gst_helpers \"${_gst_libexec_dest}/*\")
                    foreach(_helper \${_gst_helpers})
                        execute_process(
                            COMMAND install_name_tool -add_rpath @loader_path/../../Frameworks \"\${_helper}\"
                            ERROR_QUIET
                        )
                    endforeach()
                ")
            endif()
        else()
            message(WARNING "macOS: Homebrew GStreamer detected. "
                "Distributable builds require the GStreamer.framework SDK from "
                "https://gstreamer.freedesktop.org/download/")
        endif()
    endif()

    if(LINUX OR WIN32)
        set(_gst_verify_ext "so")
        set(_gst_verify_prefix "libgst")
        set(_gst_verify_dest "lib/gstreamer-1.0")
        if(WIN32)
            set(_gst_verify_ext "dll")
            set(_gst_verify_prefix "gst")
            set(_gst_verify_dest "${CMAKE_INSTALL_LIBDIR}/gstreamer-1.0")
        endif()

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

endif()
