target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Core
    ${CMAKE_CURRENT_SOURCE_DIR}/Pipeline
    ${CMAKE_CURRENT_SOURCE_DIR}/FrameDelivery
    ${CMAKE_CURRENT_SOURCE_DIR}/Health
    ${CMAKE_CURRENT_SOURCE_DIR}/Recording
)

if(QGC_ENABLE_GST_VIDEOSTREAMING)
    find_package(QGCGStreamer
        REQUIRED
        COMPONENTS Core Base Video Rtsp App Pbutils
        OPTIONAL_COMPONENTS Allocators
    )
    if(LINUX)
        find_package(OpenGL REQUIRED COMPONENTS EGL)
    endif()

    target_sources(${CMAKE_PROJECT_NAME}
        PRIVATE
            Core/GStreamer.cc
            Core/GStreamer.h
            Core/GStreamerHelpers.cc
            Core/GStreamerHelpers.h
            Core/GStreamerLogging.cc
            Core/GStreamerLogging.h
            Core/GstCallbackDispatch.h
            Core/GstEnvironment.cc
            Core/GstEnvironment.h
            Core/GstObjectPtr.h
            Core/GstPluginRegistry.cc
            Core/GstPluginRegistry.h
            Core/GstVideoReceiver.cc
            Core/GstVideoReceiver.h
            Core/GstVideoReceiverCallbacks.cc
            # --- Pipeline: topology + element construction ---
            Pipeline/GstPipelineController.cc
            Pipeline/GstPipelineController.h
            Pipeline/GstPipelineFactory.cc
            Pipeline/GstPipelineFactory.h
            Pipeline/GstSourceFactory.cc
            Pipeline/GstSourceFactory.h
            Pipeline/GstTeeBranch.h
            Pipeline/GstDecodingBranch.cc
            Pipeline/GstDecodingBranch.h
            Pipeline/GstRecordingBranch.cc
            Pipeline/GstRecordingBranch.h
            # --- FrameDelivery: appsink → QVideoFrame ---
            FrameDelivery/GstAppsinkBridge.cc
            FrameDelivery/GstAppsinkBridge.h
            FrameDelivery/GstCpuVideoBuffer.cc
            FrameDelivery/GstCpuVideoBuffer.h
            FrameDelivery/GstDmaBufRhiVideoBuffer.cc
            FrameDelivery/GstDmaBufRhiVideoBuffer.h
            FrameDelivery/GstDmaBufVideoBuffer.cc
            FrameDelivery/GstDmaBufVideoBuffer.h
            FrameDelivery/GstFrameFormatNegotiator.cc
            FrameDelivery/GstFrameFormatNegotiator.h
            FrameDelivery/GstFrameImportStrategy.cc
            FrameDelivery/GstFrameImportStrategy.h
            FrameDelivery/GstFormatTable.cc
            FrameDelivery/GstFormatTable.h
            FrameDelivery/GstLatencyTracker.cc
            FrameDelivery/GstLatencyTracker.h
            FrameDelivery/GstVideoBufferFactory.cc
            FrameDelivery/GstVideoBufferFactory.h
            FrameDelivery/GstSampleFrameConverter.cc
            FrameDelivery/GstSampleFrameConverter.h
            FrameDelivery/GstCapsToMediaMetaData.cc
            FrameDelivery/GstCapsToMediaMetaData.h
            # --- Health: watchdog + decoder fallback ---
            Health/StreamHealthMonitor.cc
            Health/StreamHealthMonitor.h
            Health/GstDecoderFallbackController.cc
            Health/GstDecoderFallbackController.h
            # --- Recording: GStreamer-native tee recorder ---
            Recording/GstTeeRecorder.cc
            Recording/GstTeeRecorder.h
    )

    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE QGC_GST_STREAMING)

    # DMA-BUF support is auto-detected inside FindQGCGStreamer when the
    # Allocators component is available. It adds QGC_GST_DMABUF + the libdrm
    # include path as INTERFACE properties on GStreamer::GStreamer; consumers
    # pick them up transitively through the link below.

    target_link_libraries(${CMAKE_PROJECT_NAME}
        PRIVATE
            Qt6::Concurrent
            Qt6::MultimediaPrivate
            Qt6::Multimedia
            GStreamer::GStreamer
    )

    if(LINUX)
        target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OpenGL::EGL)
    endif()

endif()

if(NOT QGC_ENABLE_GST_VIDEOSTREAMING)
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
    # GST_REGISTRY_FORK=no — scanner/PTP helpers are not used (in-process scanning)

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
    # GST_REGISTRY_FORK=no — scanner/PTP helpers are not used (in-process scanning)

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

        # GST_REGISTRY_FORK=no — scanner/PTP helpers are not used (in-process scanning)
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
