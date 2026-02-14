# ============================================================================
# GStreamer Video Receiver Backend
# GStreamer-based video streaming and decoding
# ============================================================================

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(${CMAKE_PROJECT_NAME} PRIVATE VideoItemStub.h)

# ============================================================================
# GStreamer Detection and Configuration
# ============================================================================

if(QGC_ENABLE_GST_VIDEOSTREAMING)
    if(NOT MACOS)
        # NOTE: Using FindGStreamer.cmake is currently bypassed on macOS
        # Using hardwired framework path as a workaround until FindGStreamer works on macOS
        find_package(GStreamer
            REQUIRED
            COMPONENTS Core Base Video Gl GlPrototypes Rtsp
            OPTIONAL_COMPONENTS GlEgl GlWayland GlX11
        )
    endif()

    # Build GStreamer Qt6 QML GL plugin
    add_subdirectory(gstqml6gl)

    # TODO: Add Qt6 Direct3D11 plugin support
    # https://gstreamer.freedesktop.org/documentation/qt6d3d11/index.html#qml6d3d11sink-page
endif()

# ============================================================================
# GStreamer Video Receiver Sources
# ============================================================================

if(TARGET gstqml6gl)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE gstqml6gl)

    target_sources(${CMAKE_PROJECT_NAME}
        PRIVATE
            GStreamer.cc
            GStreamer.h
            GStreamerHelpers.cc
            GStreamerHelpers.h
            GstVideoReceiver.cc
            GstVideoReceiver.h
    )

    # Build custom GStreamer QGC plugin
    add_subdirectory(gstqgc)

    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE QGC_GST_STREAMING)

    # ============================================================================
    # GStreamer App Support (for WebSocket video via appsrc)
    # Uses pkg_check_modules directly instead of FindGStreamer.cmake component
    # to avoid IMPORTED target directory-scope visibility issues with autogen.
    # ============================================================================

    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(GSTREAMER_APP QUIET IMPORTED_TARGET gstreamer-app-1.0)
    endif()
    if(TARGET PkgConfig::GSTREAMER_APP)
        set_target_properties(PkgConfig::GSTREAMER_APP PROPERTIES IMPORTED_GLOBAL TRUE)
        target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE PkgConfig::GSTREAMER_APP)
        target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE QGC_GST_APP_AVAILABLE)

        target_sources(${CMAKE_PROJECT_NAME}
            PRIVATE
                QGCWebSocketVideoSource.cc
                QGCWebSocketVideoSource.h
        )

        find_package(Qt6 ${QGC_QT_MINIMUM_VERSION}...${QGC_QT_MAXIMUM_VERSION}
            COMPONENTS WebSockets
        )
        if(TARGET Qt6::WebSockets)
            target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::WebSockets)
        else()
            message(WARNING "Qt6::WebSockets not found - WebSocket video streaming will not be available")
        endif()
    endif()

    # ============================================================================
    # Platform-Specific GStreamer Installation
    # ============================================================================

    if(LINUX)
        # ----------------------------------------------------------------------------
        # Linux: Install GStreamer Plugins and Libraries
        # ----------------------------------------------------------------------------
        install(
            DIRECTORY
                "${GSTREAMER_PLUGIN_PATH}"
                "${GSTREAMER_LIB_PATH}/gio"
            DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            FILES_MATCHING
                PATTERN "*.so"
                PATTERN "*.so.*"
            PATTERN "*/include" EXCLUDE
        )
        install(
            DIRECTORY "${GSTREAMER_LIB_PATH}/gstreamer1.0"
            DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            FILE_PERMISSIONS
                OWNER_READ OWNER_WRITE OWNER_EXECUTE
                GROUP_READ             GROUP_EXECUTE
                WORLD_READ             WORLD_EXECUTE
            FILES_MATCHING
                PATTERN "gst-*"
        )
    elseif(WIN32)
        # ----------------------------------------------------------------------------
        # Windows: Install GStreamer DLLs and Plugins
        # ----------------------------------------------------------------------------
        install(
            DIRECTORY "${GStreamer_ROOT_DIR}/bin/"
            DESTINATION "${CMAKE_INSTALL_BINDIR}"
            FILES_MATCHING
                PATTERN "*.dll"
        )
        install(
            DIRECTORY "${GSTREAMER_LIB_PATH}/gio/modules/"
            DESTINATION "${CMAKE_INSTALL_LIBDIR}/gio/modules"
            FILES_MATCHING
                PATTERN "*.dll"
        )
        install(
            DIRECTORY "${GSTREAMER_PLUGIN_PATH}/"
            DESTINATION "${CMAKE_INSTALL_LIBDIR}/gstreamer-1.0"
            FILES_MATCHING
                PATTERN "*.dll"
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
        # ----------------------------------------------------------------------------
        # macOS: Install GStreamer Framework
        # ----------------------------------------------------------------------------
        if(GSTREAMER_FRAMEWORK)
            install(
                DIRECTORY "${GSTREAMER_FRAMEWORK}"
                DESTINATION "${CMAKE_INSTALL_PREFIX}/${CMAKE_PROJECT_NAME}.app/Contents/Frameworks"
                PATTERN "*.la" EXCLUDE
                PATTERN "*.a" EXCLUDE
                PATTERN "*/bin" EXCLUDE
                PATTERN "*/etc" EXCLUDE
                PATTERN "*/gst-validate-launcher" EXCLUDE
                PATTERN "*/Headers" EXCLUDE
                PATTERN "*/include" EXCLUDE
                PATTERN "*/pkgconfig" EXCLUDE
                PATTERN "*/share" EXCLUDE
            )
        endif()
    endif()
endif()
