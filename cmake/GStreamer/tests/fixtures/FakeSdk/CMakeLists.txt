cmake_minimum_required(VERSION 3.25)
project(QGCGStreamerFakeSdk LANGUAGES CXX)

if(NOT QGC_MODULE_DIR)
    message(FATAL_ERROR "QGC_MODULE_DIR is required")
endif()
list(APPEND CMAKE_MODULE_PATH "${QGC_MODULE_DIR}" "${QGC_MODULE_DIR}/find-modules")
include("${QGC_MODULE_DIR}/GStreamer/tests/_assert.cmake")

# Stand-ins for BuildConfig.cmake, which reads build-config.json from the top-level source dir.
set(QGC_CONFIG_GSTREAMER_MIN_VERSION 1.20.0)
set(QGC_BUILD_CONFIG_CONTENT [=[{"gstreamer": {"plugins": {"common": ["coreelements"]}}}]=])
# The fake SDK's libraries are empty files, so feature probes must compile without linking.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(QGCGStreamer_FIND_COMPONENTS Core Base Video App Rtsp Allocators)
include(GStreamer/Orchestrator)

set(_qgc_state_targets
    GStreamer::GStreamer
    GStreamer::deps
    GStreamer::api_base
    GStreamer::api_gl
    GStreamer::api_gl_prototypes
    GStreamer::api_rtsp
    GStreamer::api_video
    GStreamer::api_app
    GStreamer::api_allocators
)
set(_qgc_state "")
foreach(_target IN LISTS _qgc_state_targets)
    qgc_test_assert_target("${_target} exists" ${_target})
    foreach(_property IN ITEMS INTERFACE_INCLUDE_DIRECTORIES INTERFACE_LINK_LIBRARIES INTERFACE_COMPILE_DEFINITIONS
                               INTERFACE_COMPILE_OPTIONS INTERFACE_LINK_OPTIONS
    )
        get_target_property(_value ${_target} ${_property})
        string(APPEND _qgc_state "${_target}.${_property}=${_value}\n")
    endforeach()
endforeach()
foreach(_key IN ITEMS LAYOUT_TYPE LIB_DIR PLUGIN_DIR INCLUDE_DIR)
    gstreamer_layout_get(${_key} _value)
    string(APPEND _qgc_state "GStreamer::Layout.${_key}=${_value}\n")
endforeach()
foreach(_var IN ITEMS GStreamer_VERSION GStreamer_ROOT_DIR GSTREAMER_APIS GSTREAMER_PLUGINS QGC_GST_HAS_DMABUF)
    string(APPEND _qgc_state "${_var}=${${_var}}\n")
endforeach()
file(WRITE "${CMAKE_BINARY_DIR}/gst-state.txt" "${_qgc_state}")

if(NOT DEFINED EXPECT_ROOT)
    return()
endif()

qgc_test_assert_streq("version" "${EXPECT_VERSION}" "${GStreamer_VERSION}")
qgc_test_assert_property("core include dirs" GStreamer::GStreamer INTERFACE_INCLUDE_DIRECTORIES
                         "${EXPECT_ROOT}/include/gstreamer-1.0"
)
qgc_test_assert_property("core libraries" GStreamer::deps INTERFACE_LINK_LIBRARIES
                         "${EXPECT_ROOT}/lib/libgstreamer-1.0.so"
)
qgc_test_assert_property("video include dirs" GStreamer::api_video INTERFACE_INCLUDE_DIRECTORIES
                         "${EXPECT_ROOT}/include/gstreamer-1.0"
)
qgc_test_assert_property("video libraries" GStreamer::api_video INTERFACE_LINK_LIBRARIES
                         "${EXPECT_ROOT}/lib/libgstvideo-1.0.so"
)
qgc_test_assert_property("layout lib dir" GStreamer::Layout GSTREAMER_LIB_DIR "${EXPECT_ROOT}/lib")
qgc_test_assert_property("layout plugin dir" GStreamer::Layout GSTREAMER_PLUGIN_DIR "${EXPECT_ROOT}/lib/gstreamer-1.0")
qgc_test_assert_property("layout include dir" GStreamer::Layout GSTREAMER_INCLUDE_DIR "${EXPECT_ROOT}/include")
qgc_test_assert_streq("plugin scan" "TRUE" "${GST_PLUGIN_coreelements_FOUND}")

# The fake SDK ships the DMABuf and orientation-meta headers only when EXPECT_PROBES is set,
# so a stale try-compile result shows up as a mismatch here.
if(EXPECT_PROBES)
    set(_qgc_expected_probe TRUE)
else()
    set(_qgc_expected_probe FALSE)
endif()
if(QGC_GST_HAS_DMABUF)
    set(_qgc_dmabuf TRUE)
else()
    set(_qgc_dmabuf FALSE)
endif()
qgc_test_assert_streq("DMABuf probe" "${_qgc_expected_probe}" "${_qgc_dmabuf}")
get_target_property(_qgc_core_defs GStreamer::GStreamer INTERFACE_COMPILE_DEFINITIONS)
if("QGC_HAS_GST_VIDEO_ORIENTATION_META=1" IN_LIST _qgc_core_defs)
    set(_qgc_orientation TRUE)
else()
    set(_qgc_orientation FALSE)
endif()
qgc_test_assert_streq("orientation-meta probe" "${_qgc_expected_probe}" "${_qgc_orientation}")
