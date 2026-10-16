cmake_minimum_required(VERSION 3.25)
# Builds the iOS GStreamer targets of platform/IOS.cmake from a fake merged archive on a Linux host and links an app the
# way QGC does: QGCGStreamer links GStreamer::GStreamer PUBLIC, the app links GStreamer::mobile.
project(QGCGStreamerIosLink LANGUAGES C CXX)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
include("${QGC_MODULE_DIR}/GStreamer/Probe.cmake")
include("${QGC_MODULE_DIR}/GStreamer/StaticInit.cmake")
include("${QGC_MODULE_DIR}/GStreamer/platform/IOS.cmake")
# ELF symbol names, unlike Mach-O ones, have no leading underscore.
set(QGC_GST_IOS_APP_CORE_SYMBOLS ${QGC_TEST_CORE_SYMBOLS})

# The merged archive: core objects that the plugin and the registration shim use (registry) or not (debug), GLib, and
# one plugin. Each core object also defines a symbol IOSSingleCoreCheck looks for.
set(_archive "${CMAKE_BINARY_DIR}/sdk/libGStreamer.a")
add_library(FakeGStreamerArchive STATIC sdk/core_registry.c sdk/core_debug.c sdk/glib.c sdk/plugin.c)
target_include_directories(FakeGStreamerArchive PRIVATE include)
set_target_properties(FakeGStreamerArchive PROPERTIES OUTPUT_NAME GStreamer ARCHIVE_OUTPUT_DIRECTORY
                                                                            "${CMAKE_BINARY_DIR}/sdk"
)

_qgc_gst_ios_usage_targets("${_archive}" "${CMAKE_CURRENT_SOURCE_DIR}/include")
_qgc_gst_ios_mobile_framework(coreelements)
add_dependencies(GStreamerMobileXcfw FakeGStreamerArchive)
if(QGC_TEST_KEEP_SYMBOLS)
    qgc_gst_ios_keep_symbols(${QGC_TEST_KEEP_SYMBOLS})
endif()

# The Orchestrator's iOS probe runs before anything is built, so it must not need the archive.
qgc_check_gst_header(VAR QGC_TEST_PROBE HEADER gst/gst.h SYMBOL "sizeof(GstRegistry *)")
if(NOT QGC_TEST_PROBE)
    message(FATAL_ERROR "ASSERT: a GStreamer::GStreamer probe fails before the archive is built")
endif()

add_library(QGCGStreamer STATIC qgc_gstreamer.c)
target_link_libraries(QGCGStreamer PUBLIC GStreamer::GStreamer)
add_executable(qgc_ios_link_app app.c)
target_link_libraries(qgc_ios_link_app PRIVATE QGCGStreamer GStreamer::mobile)
