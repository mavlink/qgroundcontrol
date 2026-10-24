if(NOT QGC_ENABLE_GST_VIDEOSTREAMING)
    return()
endif()

# Gl and GlPrototypes (gst-gl) are optional: the GLMemory GPU path and the glupload bin are built only when
# HwBuffers' gst-gl header probe succeeds.
# Linux adds Allocators (gstreamer-allocators-1.0) for the DMABuf header probe in Orchestrator.cmake.
# RtspServer (gstreamer-rtsp-server-1.0) drives the MockLink test video stream server, which is
# Debug-only (see Comms/MockLink/CMakeLists.txt) — so only request it in Debug to keep the
# rtsp-server lib out of release builds. It is also optional: a define is emitted below only
# when the SDK actually ships it.
set(_qgc_gst_components Base Gl GlPrototypes Video App Rtsp)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    list(APPEND _qgc_gst_components RtspServer)
endif()
if(LINUX)
    list(APPEND _qgc_gst_components Allocators)
endif()
# Check (gstreamer-check-1.0) is optional too: the GstHarness tests compile only where the SDK or distribution ships it.
if(QGC_BUILD_TESTING AND NOT (ANDROID OR IOS))
    list(APPEND _qgc_gst_components Check)
endif()
set(QGC_GST_COMPONENTS ${_qgc_gst_components})
include(GStreamer/Orchestrator)

# QGCGStreamer: one static lib for the self-contained HwBuffers + gstqgc leaf layer (no app deps).
# PUBLIC usage reqs flow to the app via the link below; facade sources stay app-side (need app headers).
qt_add_library(QGCGStreamer STATIC)
# Each element and buffer backend keeps file-local GObject property enums and singletons, so they cannot share a
# unity unit.
set_target_properties(
    QGCGStreamer
    PROPERTIES
        UNITY_BUILD OFF
)

target_link_libraries(
    QGCGStreamer
    PUBLIC
        GStreamer::GStreamer
        Qt6::Multimedia
        Qt6::MultimediaPrivate
        QGCLogging
)

target_compile_definitions(QGCGStreamer PUBLIC QGC_GST_STREAMING)

# gstreamer-rtsp-server-1.0 is only requested in Debug (MockLink test server) and may be absent
# on minimal installs. Its GStreamer::RtspServer target is already aggregated into GStreamer::GStreamer
# when found; just flag availability for MockVideoStreamServer.
if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND GStreamer_RtspServer_FOUND)
    target_compile_definitions(QGCGStreamer PUBLIC QGC_GST_RTSP_SERVER)
    if(ANDROID OR IOS)
        # The app links only GStreamer::mobile, so keep the RTSP server functions MockVideoStreamServer calls in it.
        qgc_gst_mobile_keep_symbols(
            gst_rtsp_media_factory_new
            gst_rtsp_media_factory_set_launch
            gst_rtsp_media_factory_set_shared
            gst_rtsp_mount_points_add_factory
            gst_rtsp_server_attach
            gst_rtsp_server_get_bound_port
            gst_rtsp_server_get_mount_points
            gst_rtsp_server_new
            gst_rtsp_server_set_address
            gst_rtsp_server_set_service
        )
    endif()
endif()

if(LINUX AND GStreamer_VERSION VERSION_LESS "1.28.0")
    target_compile_definitions(QGCGStreamer PUBLIC QGC_GST_ENABLE_LEGACY_PARSEBIN_CAPS_FILTER=1)
endif()

# MockVideoStreamServer feeds MockLink's synthetic video stream; MockLink is Debug-only
# (see Comms/MockLink/CMakeLists.txt), so only compile it in Debug to keep it out of release.
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_sources(
        QGCGStreamer
        PRIVATE
            MockVideoStreamServer.cc
            MockVideoStreamServer.h
    )
endif()

target_include_directories(
    QGCGStreamer
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/gstqgc
        ${CMAKE_CURRENT_SOURCE_DIR}/HwBuffers
        ${CMAKE_CURRENT_SOURCE_DIR}/HwBuffers/common
        ${CMAKE_CURRENT_SOURCE_DIR}/HwBuffers/dmabuf
        ${CMAKE_CURRENT_SOURCE_DIR}/HwBuffers/gl
        ${CMAKE_CURRENT_SOURCE_DIR}/HwBuffers/d3d
        ${CMAKE_CURRENT_SOURCE_DIR}/HwBuffers/vulkan
        ${CMAKE_CURRENT_SOURCE_DIR}/HwBuffers/apple
        ${CMAKE_CURRENT_SOURCE_DIR}/HwBuffers/android
        ${CMAKE_CURRENT_SOURCE_DIR}/HwBuffers/cuda
    PRIVATE ${CMAKE_BINARY_DIR}
)

# Qt6::MultimediaQuickPrivate is OPTIONAL at the root find_package; link it when present
# (GStreamer.cc / GstQgcFrameMap.cc need MultimediaQuick) rather than failing configure.
if(TARGET Qt6::MultimediaQuickPrivate)
    target_link_libraries(QGCGStreamer PUBLIC Qt6::MultimediaQuickPrivate)
else()
    message(
        WARNING "QGCGStreamer: Qt6::MultimediaQuickPrivate not found - GStreamer video sink integration may not build"
    )
endif()

add_subdirectory(HwBuffers)
add_subdirectory(gstqgc)
# HwBuffers' gstreamer-*-1.0 probes were the last to need the SDK's pkg-config environment.
qgc_gst_restore_pkgconfig_env()

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCGStreamer)

# GStreamer:: facade: app-facing API + source/receiver glue. App-coupled, so it stays on the app
# target and inherits QGCGStreamer's PUBLIC deps/defs/includes via the link above.
target_sources(
    ${CMAKE_PROJECT_NAME}
    PRIVATE
        BranchContext.h
        GStreamer.cc
        GStreamer.h
        GStreamerEnvironment.cc
        GStreamerEnvironment.h
        GStreamerHelpers.cc
        GStreamerHelpers.h
        GStreamerLogging.cc
        GStreamerLogging.h
        GstScoped.h
        GstSourceFactory.cc
        GstSourceFactory.h
        GstVideoReceiver.cc
        GstVideoReceiver.h
        QGCQVideoSinkController.cc
        QGCQVideoSinkController.h
        StreamWatchdog.cc
        StreamWatchdog.h
        PipelineDotDumper.cc
        PipelineDotDumper.h
        PipelineSession.cc
        PipelineSession.h
        RecordingBranch.cc
        RecordingBranch.h
        RecordingFinalizer.cc
        RecordingFinalizer.h
        DecodingBranch.cc
        DecodingBranch.h
)

# Its file-scope `using namespace GStreamer;` would leak into later sources of a unity unit.
set_source_files_properties(
    GstSourceFactory.cc
    TARGET_DIRECTORY
    ${CMAKE_PROJECT_NAME}
    PROPERTIES
        SKIP_UNITY_BUILD_INCLUSION ON
)

# The iOS SDK ships no CA list / no keychain-via-OpenSSL; bundle a Mozilla NSS extract for
# libgioopenssl (fetched at configure time, IOS.cmake). On the app target for MACOSX_PACKAGE_LOCATION.
if(IOS AND GStreamer_IOS_CA_BUNDLE)
    target_sources(
        ${CMAKE_PROJECT_NAME}
        PRIVATE
            "${GStreamer_IOS_CA_BUNDLE}"
    )
    set_source_files_properties(
        "${GStreamer_IOS_CA_BUNDLE}"
        PROPERTIES
            MACOSX_PACKAGE_LOCATION "ssl/certs"
            HEADER_FILE_ONLY TRUE
    )
endif()
# The GStreamer license notice (Orchestrator.cmake); Android stages it with the APK assets, desktop installs it.
if(IOS)
    gstreamer_layout_get(NOTICE_DIR _qgc_gst_notice_dir)
    set(_qgc_gst_notice_files "${_qgc_gst_notice_dir}/NOTICE.txt" "${_qgc_gst_notice_dir}/LGPL-2.1.txt")
    target_sources(
        ${CMAKE_PROJECT_NAME}
        PRIVATE
            ${_qgc_gst_notice_files}
    )
    set_source_files_properties(
        ${_qgc_gst_notice_files}
        PROPERTIES
            MACOSX_PACKAGE_LOCATION "licenses/gstreamer"
            HEADER_FILE_ONLY TRUE
    )
endif()

# Mobile layouts link the SDK into GStreamer::mobile (libgstreamer_android.so, the iOS framework), which registers
# GSTREAMER_STATIC_PLUGINS in its generated gst_init_static_plugins(), called by GStreamer.cc::_registerPlugins. The
# Android and iOS SDKs register none of their plugins themselves (verified on-device: without it, coreelements, tcp,
# udp, rtp, ... stay unregistered).
if(ANDROID OR IOS)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE GStreamer::mobile)
endif()
if(ANDROID)
    # GStreamerEnvironment.cc extracts the bundled APK assets with the NDK's asset manager.
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE android)
endif()
# The iOS check is wired in cmake/platform/Apple.cmake. The app target is created by the top-level directory.
if(ANDROID)
    cmake_language(
        DEFER
        DIRECTORY
        "${CMAKE_SOURCE_DIR}"
        CALL
        add_custom_command
        TARGET
        ${CMAKE_PROJECT_NAME}
        POST_BUILD
        COMMAND
        "${CMAKE_COMMAND}"
        "-DAPP=$<TARGET_FILE:${CMAKE_PROJECT_NAME}>"
        "-DNM=${CMAKE_NM}"
        -P
        "${CMAKE_SOURCE_DIR}/cmake/GStreamer/SingleCoreCheck.cmake"
        COMMENT
        "Checking that the app links a single GStreamer core"
        VERBATIM
    )
endif()

# Mobile platforms ship their plugins inside GStreamer::mobile; no install steps below.
if(ANDROID OR IOS)
    return()
endif()

# Build-time rpath so the unrelocated binary finds component dylibs in the SDK
# (framework /lib or flat /lib); install-time rpath is set in cmake/install/Install.cmake.
if(MACOS AND GSTREAMER_LIB_PATH)
    set_property(
        TARGET ${CMAKE_PROJECT_NAME}
        APPEND
        PROPERTY BUILD_RPATH "${GSTREAMER_LIB_PATH}"
    )
endif()

# QGC_GST_BUNDLE_PLUGINS=OFF: the install carries no plugins (Install.cmake), so the app keeps the system plugin path.
# An AppImage cannot rely on the host's plugins: its bundled GStreamer core and GLib would load them.
if(LINUX AND NOT QGC_GST_BUNDLE_PLUGINS)
    if(QGC_CREATE_APPIMAGE)
        message(
            FATAL_ERROR "QGC_GST_BUNDLE_PLUGINS=OFF is for distribution packages, whose GStreamer plugins come from "
                        "the system; an AppImage must bundle them. Set QGC_CREATE_APPIMAGE=OFF."
        )
    endif()
    set_property(
        SOURCE GStreamerEnvironment.cc TARGET_DIRECTORY ${CMAKE_PROJECT_NAME}
        APPEND
        PROPERTY COMPILE_DEFINITIONS QGC_GST_SYSTEM_PLUGINS
    )
endif()

# Native packages install only the Runtime component. Keep this assignment
# scoped here so bundled SDK files are included without packaging dependency
# development files declared elsewhere in the source tree.
set(CMAKE_INSTALL_DEFAULT_COMPONENT_NAME Runtime)
gstreamer_install_platform_sdk(${CMAKE_PROJECT_NAME})
