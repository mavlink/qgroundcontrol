if(NOT QGC_ENABLE_GST_VIDEOSTREAMING)
    return()
endif()

# Linux adds Allocators (gstreamer-allocators-1.0) for the DMABuf header probe in Orchestrator.cmake.
set(_qgc_gst_components Core Base Video App Rtsp)
if(LINUX)
    list(APPEND _qgc_gst_components Allocators)
endif()
set(QGCGStreamer_FIND_COMPONENTS ${_qgc_gst_components})
include(GStreamer/Orchestrator)

# QGCGStreamer: one static lib for the self-contained HwBuffers + gstqgc leaf layer (no app deps).
# PUBLIC usage reqs flow to the app via the link below; facade sources stay app-side (need app headers).
qt_add_library(QGCGStreamer STATIC)

target_link_libraries(QGCGStreamer
    PUBLIC
        GStreamer::GStreamer
        Qt6::Multimedia
        Qt6::MultimediaPrivate
        QGCLogging
)

target_compile_definitions(QGCGStreamer PUBLIC QGC_GST_STREAMING)

if(LINUX AND GStreamer_VERSION VERSION_LESS "1.28.0")
    target_compile_definitions(QGCGStreamer PUBLIC QGC_GST_ENABLE_LEGACY_PARSEBIN_CAPS_FILTER=1)
endif()

# The *ForTest() helpers (singleFdImportEnabledForTest, clearForTest) compile only under this flag.
# PUBLIC so both QGCGStreamer (helper defs) and the app target's GStreamerTest.cc (decls) see it.
if(QGC_BUILD_TESTING)
    target_compile_definitions(QGCGStreamer PUBLIC QGC_GST_BUILD_TESTING)
endif()

target_include_directories(QGCGStreamer
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
    PRIVATE
        ${CMAKE_BINARY_DIR}
)

# Qt6::MultimediaQuickPrivate is OPTIONAL at the root find_package; link it when present
# (GStreamer.cc / GStreamerFrameMap.cc need MultimediaQuick) rather than failing configure.
if(TARGET Qt6::MultimediaQuickPrivate)
    target_link_libraries(QGCGStreamer PUBLIC Qt6::MultimediaQuickPrivate)
else()
    message(WARNING "QGCGStreamer: Qt6::MultimediaQuickPrivate not found - GStreamer video sink integration may not build")
endif()

add_subdirectory(HwBuffers)
add_subdirectory(gstqgc)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCGStreamer)

# GStreamer:: facade: app-facing API + source/receiver glue. App-coupled, so it stays on the app
# target and inherits QGCGStreamer's PUBLIC deps/defs/includes via the link above.
target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
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
)

# iOS xcframework ships no CA list / no keychain-via-OpenSSL; bundle a Mozilla NSS extract for
# libgioopenssl (fetched at configure time, IOS.cmake). On the app target for MACOSX_PACKAGE_LOCATION.
if(IOS AND GStreamer_IOS_CA_BUNDLE)
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE "${GStreamer_IOS_CA_BUNDLE}")
    set_source_files_properties("${GStreamer_IOS_CA_BUNDLE}" PROPERTIES
        MACOSX_PACKAGE_LOCATION "ssl/certs"
        HEADER_FILE_ONLY TRUE
    )
endif()

# gst_init_static_plugins() shim source (called in GStreamer.cc::_registerPlugins), per platform:
#   iOS: provided by GStreamer.xcframework (via GStreamer::mobile); no shim generated.
#   Android: shim generated here from the plugin/GIO lists the Android backend put in scope via
#            include(GStreamer/Orchestrator); GStreamer::mobile whole-archives the .a's into the app core.
#   Desktop static: shim generated from GSTREAMER_PLUGINS below.
if(IOS AND TARGET GStreamer::mobile)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE GStreamer::mobile)
elseif(ANDROID AND TARGET GStreamer::mobile)
    # Pull the whole-archived plugin .a's + helpers + GIO modules into the app's one core.
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE GStreamer::mobile)
    # The Android SDK gst_init() does NOT register the bundled plugins (verified on-device: without this,
    # coreelements/tcp/udp/rtp/... stay unregistered), so the generated gst_init_static_plugins() is required.
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/gst_static_plugins.c.in"
        "${CMAKE_CURRENT_BINARY_DIR}/gst_static_plugins.c"
        @ONLY)
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}/gst_static_plugins.c")
    # This generated C source must not pull the app's C++ precompiled header (the app PCH
    # includes C++-only Qt headers); a C-language PCH built from it fails to compile.
    set_source_files_properties(
        "${CMAKE_CURRENT_BINARY_DIR}/gst_static_plugins.c"
        TARGET_DIRECTORY ${CMAKE_PROJECT_NAME}
        PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
elseif(GStreamer_USE_STATIC_LIBS)
    set(_qgc_found_static "")
    set(_qgc_missing_static "")
    foreach(_p IN LISTS GSTREAMER_PLUGINS)
        if(GST_PLUGIN_${_p}_FOUND)
            list(APPEND _qgc_found_static "${_p}")
        else()
            list(APPEND _qgc_missing_static "${_p}")
        endif()
    endforeach()
    _gst_emit_static_plugin_registration(_qgc_found_static _qgc_decl _qgc_reg)
    if(_qgc_missing_static)
        # AVAILABLE = what was found, not the full request, else real gaps are hidden.
        gstreamer_filter_alternates(IN_OUT_PLUGINS _qgc_missing_static AVAILABLE ${_qgc_found_static})
    endif()
    if(_qgc_missing_static)
        message(WARNING "GStreamer (static): requested plugins not found as targets and omitted from static registration: ${_qgc_missing_static}")
    endif()
    set(PLUGINS_DECLARATION  "${_qgc_decl}")
    set(PLUGINS_REGISTRATION "${_qgc_reg}")
    set(G_IO_MODULES_DECLARE "")
    set(G_IO_MODULES_LOAD    "")
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/gst_static_plugins.c.in"
        "${CMAKE_CURRENT_BINARY_DIR}/gst_static_plugins.c"
        @ONLY)
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}/gst_static_plugins.c")
    set_source_files_properties(
        "${CMAKE_CURRENT_BINARY_DIR}/gst_static_plugins.c"
        TARGET_DIRECTORY ${CMAKE_PROJECT_NAME}
        PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
    unset(_qgc_decl)
    unset(_qgc_reg)
endif()

# Mobile platforms ship plugins inside the static .a / xcfw — no install steps below.
if(ANDROID OR IOS)
    return()
endif()

include(GStreamer/Helpers)

# Build-time rpath so the unrelocated binary finds component dylibs in the SDK
# (framework /lib or flat /lib); install-time rpath is set in cmake/install/Install.cmake.
if(MACOS AND GSTREAMER_LIB_PATH)
    set_property(TARGET ${CMAKE_PROJECT_NAME} APPEND PROPERTY
        BUILD_RPATH "${GSTREAMER_LIB_PATH}")
endif()

gstreamer_install_platform_sdk(${CMAKE_PROJECT_NAME})
