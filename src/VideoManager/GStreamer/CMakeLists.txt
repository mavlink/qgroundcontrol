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
    # The render target and the RHI capture follow Qt Quick's window (QQuickWindow).
    PRIVATE Qt6::Quick
)

target_compile_definitions(QGCGStreamer PUBLIC QGC_GST_STREAMING)

# gstreamer-rtsp-server-1.0 is only requested in Debug (MockLink test server) and may be absent
# on minimal installs. Its GStreamer::RtspServer target is already aggregated into GStreamer::GStreamer
# when found; just flag availability for MockLink's MockVideoStreamServer.
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

target_include_directories(
    QGCGStreamer
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/Runtime
        ${CMAKE_CURRENT_SOURCE_DIR}/Pipeline
        ${CMAKE_CURRENT_SOURCE_DIR}/Sink
        ${CMAKE_CURRENT_SOURCE_DIR}/Sink/gstqgc
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

add_subdirectory(HwBuffers)
add_subdirectory(Sink/gstqgc)
# HwBuffers' gstreamer-*-1.0 probes were the last to need the SDK's pkg-config environment.
qgc_gst_restore_pkgconfig_env()

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCGStreamer)

# GStreamer:: facade: app-facing API + source/receiver glue. App-coupled, so it stays on the app
# target and inherits QGCGStreamer's PUBLIC deps/defs/includes via the link above.
target_sources(
    ${CMAKE_PROJECT_NAME}
    PRIVATE
        # Runtime: initialization, environment, logging, helpers, the boot self-test and the VideoBackend.
        Runtime/GStreamer.cc
        Runtime/GStreamer.h
        Runtime/GStreamerDecoderRankPlan.cc
        Runtime/GStreamerDecoderRankPlan.h
        Runtime/GStreamerDecoderRanks.cc
        Runtime/GStreamerDecoderRanks.h
        Runtime/GStreamerEnvironment.cc
        Runtime/GStreamerEnvironment.h
        Runtime/GStreamerHelpers.cc
        Runtime/GStreamerHelpers.h
        Runtime/GStreamerLogging.cc
        Runtime/GStreamerLogging.h
        Runtime/GStreamerSelfTest.cc
        Runtime/GStreamerSelfTest.h
        Runtime/GStreamerSelfTestMedia.cc
        Runtime/GStreamerSelfTestMedia.h
        Runtime/GStreamerUdpSrcWorkaround.cc
        Runtime/GStreamerUdpSrcWorkaround.h
        Runtime/GStreamerVideoBackend.cc
        Runtime/GStreamerVideoBackend.h
        Runtime/GstScoped.h
        Runtime/PipelineDotDumper.cc
        Runtime/PipelineDotDumper.h
        # Pipeline: the receiver, its worker, sessions and branches, and the source bins.
        Pipeline/BranchContext.h
        Pipeline/DecodingBranch.cc
        Pipeline/DecodingBranch.h
        Pipeline/GstH26xParameterSetGate.cc
        Pipeline/GstH26xParameterSetGate.h
        Pipeline/GstReceiverRequest.h
        Pipeline/GstReceiverWorker.cc
        Pipeline/GstReceiverWorker.h
        Pipeline/GstRtspSource.cc
        Pipeline/GstRtspSource.h
        Pipeline/GstSourceFactory.cc
        Pipeline/GstSourceFactory.h
        Pipeline/GstSourceUri.cc
        Pipeline/GstSourceUri.h
        Pipeline/GstVideoReceiver.cc
        Pipeline/GstVideoReceiver.h
        Pipeline/PipelineSession.cc
        Pipeline/PipelineSession.h
        Pipeline/RecordingBranch.cc
        Pipeline/RecordingBranch.h
        Pipeline/RecordingFinalizer.cc
        Pipeline/RecordingFinalizer.h
        Pipeline/StreamWatchdog.cc
        Pipeline/StreamWatchdog.h
        Pipeline/TeeBranchHead.cc
        Pipeline/TeeBranchHead.h
        # Sink: the Qt side of qgcqvideosink (the gstqgc plugin is in QGCGStreamer).
        Sink/QGCQVideoSinkController.cc
        Sink/QGCQVideoSinkController.h
)

# The samples GStreamer::selfTest() decodes and records, the 160x120 H.264 and 320x240 H.265 streams of Runtime/Media
# (generate.sh there), which the GStreamer tests send too.
set(_qgc_gst_selftest_media "${CMAKE_CURRENT_SOURCE_DIR}/Runtime/Media")
qt_add_resources(
    ${CMAKE_PROJECT_NAME} "GStreamerSelfTestMedia_res"
    PREFIX "/gstreamer/selftest"
    BASE "${_qgc_gst_selftest_media}"
    FILES
        "${_qgc_gst_selftest_media}/h264_160x120.h264"
        "${_qgc_gst_selftest_media}/h265_320x240.h265"
)

# Its file-scope `using namespace GStreamer;` would leak into later sources of a unity unit.
set_source_files_properties(
    Pipeline/GstSourceFactory.cc
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
    qgc_gst_layout_get(NOTICE_DIR _qgc_gst_notice_dir)
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
# Deferred to the top-level directory, which creates the app target and evaluates the arguments, so they name only
# global variables. Mach-O symbols carry a leading underscore.
if(ANDROID OR IOS)
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
        "-DSYMBOL_PREFIX=$<$<PLATFORM_ID:iOS>:_>"
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
    # Public, so the environment tests know the build bundles no runtime.
    target_compile_definitions(QGCGStreamer PUBLIC QGC_GST_SYSTEM_PLUGINS)
endif()

# Native packages install only the Runtime component. Keep this assignment
# scoped here so bundled SDK files are included without packaging dependency
# development files declared elsewhere in the source tree.
set(CMAKE_INSTALL_DEFAULT_COMPONENT_NAME Runtime)
qgc_gst_install_platform_sdk(${CMAKE_PROJECT_NAME})
