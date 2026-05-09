target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/HwBuffers
)

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

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        GStreamer.cc
        GStreamer.h
        GStreamerHelpers.cc
        GStreamerHelpers.h
        GStreamerLogging.cc
        GStreamerLogging.h
        GstAppSinkAdapter.cc
        GstAppSinkAdapter.h
        GstSourceFactory.cc
        GstSourceFactory.h
        GstVideoReceiver.cc
        GstVideoReceiver.h
)

# Bundle a Mozilla NSS-extract CA list for libgioopenssl on iOS — the xcframework
# ships none, and Apple's keychain isn't reachable from OpenSSL. _setGstEnvVars
# points CA_CERTIFICATES at this path; gst_init_static_plugins installs it as the
# default GTlsBackend database. Same pattern as upstream gst-docs/examples/
# tutorials/xcode iOS/Tutorial 5, except we fetch the bundle at configure time
# from https://curl.se/ca/cacert.pem (see _qgc_download_ios_ca_bundle in
# cmake/GStreamer/platform/IOS.cmake) instead of vendoring it in-tree.
if(IOS AND GStreamer_IOS_CA_BUNDLE)
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE "${GStreamer_IOS_CA_BUNDLE}")
    set_source_files_properties("${GStreamer_IOS_CA_BUNDLE}" PROPERTIES
        MACOSX_PACKAGE_LOCATION "ssl/certs"
        HEADER_FILE_ONLY TRUE
    )
endif()
target_link_libraries(${CMAKE_PROJECT_NAME}
    PRIVATE
        GStreamer::GStreamer
        Qt6::Multimedia
        Qt6::MultimediaQuickPrivate  # found by root CMakeLists.txt find_package
)

# GStreamer.cc::_registerPlugins() calls gst_init_static_plugins() on every
# QGC_GST_STATIC_BUILD configuration. The function is provided by:
#   • Android: GStreamer::mobile (libgstreamer_android.so; Android.cmake)
#   • iOS:     GStreamer::mobile (GStreamerMobileXcfw.framework; IOS.cmake)
#   • Desktop static: gst_static_plugins.c generated below from the same template
# Mobile shims also load gioopenssl + the bundled CA bundle.
if((ANDROID OR IOS) AND TARGET GStreamer::mobile)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE GStreamer::mobile)
elseif(GStreamer_USE_STATIC_LIBS)
    set(_qgc_decl "")
    set(_qgc_reg  "")
    foreach(_p IN LISTS GSTREAMER_PLUGINS)
        if(GST_PLUGIN_${_p}_FOUND)
            string(APPEND _qgc_decl "GST_PLUGIN_STATIC_DECLARE(${_p});\n")
            string(APPEND _qgc_reg  "    QGC_REGISTER_STATIC_PLUGIN(${_p});\n")
        endif()
    endforeach()
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
    unset(_qgc_decl)
    unset(_qgc_reg)
endif()

add_subdirectory(HwBuffers)
add_subdirectory(gstqgc)

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
