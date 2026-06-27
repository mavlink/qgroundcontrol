cmake_minimum_required(VERSION 3.22)
include("${CMAKE_CURRENT_LIST_DIR}/_assert.cmake")

file(READ "${CMAKE_CURRENT_LIST_DIR}/../../../.github/build-config.json" _build_config)

function(_qgc_json_version key out_var)
    string(JSON _version ERROR_VARIABLE _err
        GET "${_build_config}" "gstreamer" "version" "${key}")
    if(_err OR NOT _version)
        message(FATAL_ERROR "ASSERT [version.${key}]: missing gstreamer.version.${key}")
    endif()
    set(${out_var} "${_version}" PARENT_SCOPE)
endfunction()

_qgc_json_version(default _version_default)
foreach(_platform IN ITEMS android ios macos windows)
    _qgc_json_version(${_platform} _version_platform)
    if(_version_platform VERSION_LESS "1.28.0")
        message(FATAL_ERROR "ASSERT [version.${_platform}]: expected >= 1.28.0 actual='${_version_platform}'")
    endif()
endforeach()

_qgc_json_version(minimum _version_minimum)
if(NOT _version_minimum VERSION_LESS "1.28.0")
    message(FATAL_ERROR "ASSERT [version.minimum]: Linux/system floor should remain older than 1.28 actual='${_version_minimum}'")
endif()
qgc_test_pass("build config platform version floors")

file(READ "${CMAKE_CURRENT_LIST_DIR}/../../../src/VideoManager/VideoReceiver/GStreamer/GstSourceFactory.cc" _source_factory)
file(READ "${CMAKE_CURRENT_LIST_DIR}/../../../src/VideoManager/VideoReceiver/GStreamer/CMakeLists.txt" _gstreamer_cmake)
if(NOT _gstreamer_cmake MATCHES "LINUX AND GStreamer_VERSION VERSION_LESS \"1\\.28\\.0\""
   OR NOT _gstreamer_cmake MATCHES "QGC_GST_ENABLE_LEGACY_PARSEBIN_CAPS_FILTER=1")
    message(FATAL_ERROR
        "ASSERT [parsebin legacy define]: QGCGStreamer must define QGC_GST_ENABLE_LEGACY_PARSEBIN_CAPS_FILTER "
        "only for Linux builds discovered below GStreamer 1.28.")
endif()
if(_source_factory MATCHES "#if[^\n]*![^\n]*GST_CHECK_VERSION\\(1,[ \t]*28,[ \t]*0\\)")
    message(FATAL_ERROR
        "ASSERT [parsebin legacy gate]: raw !GST_CHECK_VERSION(1, 28, 0) found in GstSourceFactory.cc; "
        "route old-GStreamer parsebin workarounds through a Linux-only project define.")
endif()
if(NOT _source_factory MATCHES "QGC_GST_ENABLE_LEGACY_PARSEBIN_CAPS_FILTER")
    message(FATAL_ERROR
        "ASSERT [parsebin legacy gate]: GstSourceFactory.cc must use QGC_GST_ENABLE_LEGACY_PARSEBIN_CAPS_FILTER "
        "for the Linux-only <1.28 parsebin workaround.")
endif()
qgc_test_pass("parsebin legacy caps workaround gate")

file(READ "${CMAKE_CURRENT_LIST_DIR}/../../../test/VideoManager/GStreamer/GStreamerTest.cc" _gstreamer_test)
if(NOT _gstreamer_test MATCHES "#if defined\\(Q_OS_LINUX\\)"
   OR NOT _gstreamer_test MATCHES "minor >= 20"
   OR NOT _gstreamer_test MATCHES "minor >= 28"
   OR NOT _gstreamer_test MATCHES "bundled SDK minimum 1\\.28\\.0")
    message(FATAL_ERROR
        "ASSERT [runtime version test]: GStreamerTest must keep the 1.20 runtime floor Linux-only "
        "and enforce 1.28+ for bundled SDK platforms.")
endif()
qgc_test_pass("runtime version test platform split")
