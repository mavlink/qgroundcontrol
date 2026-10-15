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
set(_qgc_gst_components
    Core
    Base
    Gl
    GlPrototypes
    Video
    App
    Rtsp
)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    list(APPEND _qgc_gst_components RtspServer)
endif()
if(LINUX)
    list(APPEND _qgc_gst_components Allocators)
endif()
set(QGC_GST_COMPONENTS ${_qgc_gst_components})
include(GStreamer/Orchestrator)

# QGCGStreamer: one static lib for the self-contained HwBuffers + gstqgc leaf layer (no app deps).
# PUBLIC usage reqs flow to the app via the link below; facade sources stay app-side (need app headers).
qt_add_library(QGCGStreamer STATIC)

target_link_libraries(QGCGStreamer PUBLIC GStreamer::GStreamer Qt6::Multimedia Qt6::MultimediaPrivate QGCLogging)

target_compile_definitions(QGCGStreamer PUBLIC QGC_GST_STREAMING)

# gstreamer-rtsp-server-1.0 is only requested in Debug (MockLink test server) and may be absent
# on minimal installs. Its api_rtsp_server target is already aggregated into GStreamer::GStreamer
# when found; just flag availability for MockVideoStreamServer.
if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND GStreamer_RtspServer_FOUND)
    target_compile_definitions(QGCGStreamer PUBLIC QGC_GST_RTSP_SERVER)
endif()

if(LINUX AND GStreamer_VERSION VERSION_LESS "1.28.0")
    target_compile_definitions(QGCGStreamer PUBLIC QGC_GST_ENABLE_LEGACY_PARSEBIN_CAPS_FILTER=1)
endif()

# MockVideoStreamServer feeds MockLink's synthetic video stream; MockLink is Debug-only
# (see Comms/MockLink/CMakeLists.txt), so only compile it in Debug to keep it out of release.
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_sources(QGCGStreamer PRIVATE MockVideoStreamServer.cc MockVideoStreamServer.h)
endif()

target_include_directories(
    QGCGStreamer
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
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
    PRIVATE GStreamer.cc
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
            RecordingBranch.cc
            RecordingBranch.h
            RecordingFinalizer.cc
            RecordingFinalizer.h
            DecodingBranch.cc
            DecodingBranch.h
)

# GStreamer.cc fails initialization when a runtime-required plugin (PluginPolicy.cmake) is not registered. Each entry
# lists, "|"-separated, the plugin sets that satisfy it, a set's plugins joined by "+".
gstreamer_runtime_required_plugins(_qgc_gst_required_plugins)
set(_qgc_gst_required_entries "")
foreach(_qgc_gst_plugin IN LISTS _qgc_gst_required_plugins)
    gstreamer_plugin_alternatives("${_qgc_gst_plugin}" _qgc_gst_alternatives)
    list(JOIN _qgc_gst_alternatives "|" _qgc_gst_entry)
    list(APPEND _qgc_gst_required_entries "\"${_qgc_gst_entry}\"")
endforeach()
list(JOIN _qgc_gst_required_entries "," _qgc_gst_required_entries)
set_property(
    SOURCE GStreamer.cc TARGET_DIRECTORY ${CMAKE_PROJECT_NAME}
    APPEND
    PROPERTY COMPILE_DEFINITIONS "QGC_GST_RUNTIME_REQUIRED_PLUGINS=${_qgc_gst_required_entries}"
)

# iOS xcframework ships no CA list / no keychain-via-OpenSSL; bundle a Mozilla NSS extract for
# libgioopenssl (fetched at configure time, IOS.cmake). On the app target for MACOSX_PACKAGE_LOCATION.
if(IOS AND GStreamer_IOS_CA_BUNDLE)
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE "${GStreamer_IOS_CA_BUNDLE}")
    set_source_files_properties("${GStreamer_IOS_CA_BUNDLE}" PROPERTIES MACOSX_PACKAGE_LOCATION "ssl/certs"
                                                                        HEADER_FILE_ONLY TRUE
    )
endif()

# Static layouts register their plugins in a generated gst_init_static_plugins() (called by
# GStreamer.cc::_registerPlugins) for GSTREAMER_STATIC_PLUGINS, which Orchestrator.cmake resolves. iOS generates it
# into GStreamer::mobile (platform/IOS.cmake); Android and desktop static SDKs into the app, which carries the only
# GStreamer core. The Android SDK's gst_init() does NOT register the bundled plugins (verified on-device: without
# this, coreelements/tcp/udp/rtp/... stay unregistered).
if(ANDROID AND TARGET GStreamer::mobile)
    # Pull the whole-archived plugin .a's + helpers + GIO modules into the app's one core.
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE GStreamer::mobile)
    qgc_gst_generate_static_init(TARGET ${CMAKE_PROJECT_NAME} PLUGINS ${GSTREAMER_STATIC_PLUGINS} GIO_MODULES
                                 ${G_IO_MODULES}
    )
elseif(IOS AND TARGET GStreamer::mobile)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE GStreamer::mobile)
elseif(GStreamer_USE_STATIC_LIBS)
    qgc_gst_generate_static_init(TARGET ${CMAKE_PROJECT_NAME} PLUGINS ${GSTREAMER_STATIC_PLUGINS})
endif()

# Mobile platforms ship plugins inside the static .a / xcfw — no install steps below.
if(ANDROID OR IOS)
    return()
endif()

include(GStreamer/Helpers)

# Build-time rpath so the unrelocated binary finds component dylibs in the SDK
# (framework /lib or flat /lib); install-time rpath is set in cmake/install/Install.cmake.
if(MACOS AND GSTREAMER_LIB_PATH)
    set_property(
        TARGET ${CMAKE_PROJECT_NAME}
        APPEND
        PROPERTY BUILD_RPATH "${GSTREAMER_LIB_PATH}"
    )
endif()

# Native packages install only the Runtime component. Keep this assignment
# scoped here so bundled SDK files are included without packaging dependency
# development files declared elsewhere in the source tree.
set(CMAKE_INSTALL_DEFAULT_COMPONENT_NAME Runtime)
gstreamer_install_platform_sdk(${CMAKE_PROJECT_NAME})
