cmake_minimum_required(VERSION 3.25)
include("${CMAKE_CURRENT_LIST_DIR}/_assert.cmake")

file(READ "${CMAKE_CURRENT_LIST_DIR}/../../../src/VideoManager/GStreamer/Pipeline/GstSourceFactory.cc" _source_factory)
file(READ "${CMAKE_CURRENT_LIST_DIR}/../../../src/VideoManager/GStreamer/CMakeLists.txt" _gstreamer_cmake)
if(NOT _gstreamer_cmake MATCHES "LINUX AND GStreamer_VERSION VERSION_LESS \"1\\.28\\.0\""
   OR NOT _gstreamer_cmake MATCHES "QGC_GST_ENABLE_LEGACY_PARSEBIN_CAPS_FILTER=1"
)
    message(
        FATAL_ERROR
            "ASSERT [parsebin legacy define]: QGCGStreamer must define QGC_GST_ENABLE_LEGACY_PARSEBIN_CAPS_FILTER "
            "only for Linux builds discovered below GStreamer 1.28."
    )
endif()
if(_source_factory MATCHES "#if[^\n]*![^\n]*GST_CHECK_VERSION\\(1,[ \t]*28,[ \t]*0\\)")
    message(
        FATAL_ERROR "ASSERT [parsebin legacy gate]: raw !GST_CHECK_VERSION(1, 28, 0) found in GstSourceFactory.cc; "
                    "route old-GStreamer parsebin workarounds through a Linux-only project define."
    )
endif()
if(NOT _source_factory MATCHES "QGC_GST_ENABLE_LEGACY_PARSEBIN_CAPS_FILTER")
    message(
        FATAL_ERROR
            "ASSERT [parsebin legacy gate]: GstSourceFactory.cc must use QGC_GST_ENABLE_LEGACY_PARSEBIN_CAPS_FILTER "
            "for the Linux-only <1.28 parsebin workaround."
    )
endif()
qgc_test_pass("parsebin legacy caps workaround gate")

file(READ "${CMAKE_CURRENT_LIST_DIR}/../../../test/VideoManager/GStreamer/GStreamerTest.cc" _gstreamer_test)
file(READ "${CMAKE_CURRENT_LIST_DIR}/../../../test/VideoManager/GStreamer/CMakeLists.txt" _gstreamer_test_cmake)
if(NOT _gstreamer_test MATCHES "QGC_GST_RUNTIME_MINIMUM_VERSION"
   OR NOT _gstreamer_test_cmake MATCHES "qgc_config_read\\(_qgc_gst_runtime_minimum gstreamer version minimum\\)"
   OR NOT _gstreamer_test_cmake MATCHES
      "qgc_config_read\\(_qgc_gst_runtime_minimum gstreamer version bundled_minimum\\)"
)
    message(
        FATAL_ERROR "ASSERT [runtime version test]: GStreamerTest must check the runtime against "
                    "gstreamer.version.minimum on Linux and gstreamer.version.bundled_minimum elsewhere."
    )
endif()
qgc_test_pass("runtime version test platform split")
