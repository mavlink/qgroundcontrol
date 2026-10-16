cmake_minimum_required(VERSION 3.25)
project(QGCGStreamerFakeSdk LANGUAGES C CXX)

if(NOT QGC_MODULE_DIR)
    message(FATAL_ERROR "QGC_MODULE_DIR is required")
endif()
list(APPEND CMAKE_MODULE_PATH "${QGC_MODULE_DIR}")
include("${QGC_MODULE_DIR}/GStreamer/tests/_assert.cmake")

# Stand-in for the build-config.json that BuildConfig.cmake loads from the top-level source dir.
# QGC_TEST_PLUGINS: comma-separated plugin allowlist, default coreelements.
if(NOT QGC_TEST_PLUGINS)
    set(QGC_TEST_PLUGINS coreelements)
endif()
string(REPLACE "," "\", \"" _qgc_test_plugins_json "${QGC_TEST_PLUGINS}")
string(CONCAT QGC_BUILD_CONFIG_CONTENT
              "{\"gstreamer\": {\"version\": {\"minimum\": \"1.20.0\", \"android\": \"1.20.0\", "
              "\"macos\": \"1.22.0\"}, \"plugins\": {\"common\": [\"${_qgc_test_plugins_json}\"]}}}"
)
# The fake SDK's libraries are empty files, so feature probes must compile without linking.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# QGC_TEST_ANDROID_ABI: configure the GStreamer modules as an Android build for that ABI, with a shared library in
# place of the app (the real app target exists before the GStreamer directory is processed).
if(QGC_TEST_ANDROID_ABI)
    set(ANDROID TRUE)
    set(CMAKE_ANDROID_ARCH_ABI "${QGC_TEST_ANDROID_ABI}")
    set(QGC_ANDROID_EXTRA_JAVA_SOURCES_DIR "${CMAKE_BINARY_DIR}/extra_java_sources")
    set(QGC_ANDROID_EXTRA_ASSETS_DIR "${CMAKE_BINARY_DIR}/extra_assets")
    add_library(${PROJECT_NAME} SHARED "${CMAKE_CURRENT_SOURCE_DIR}/fake_app.cpp")
endif()

# QGC_TEST_MACOS: configure the GStreamer modules as a macOS build (platform/MacOS.cmake).
if(QGC_TEST_MACOS)
    set(APPLE TRUE)
    set(MACOS TRUE)
    set(LINUX FALSE)
endif()

set(QGC_GST_COMPONENTS
    Base
    Gl
    GlPrototypes
    Video
    App
    Rtsp
    Allocators
)
set(_qgc_prefix_path_before "${CMAKE_PREFIX_PATH}")
set(_qgc_pkg_config_path_before "$ENV{PKG_CONFIG_PATH}")
set(_qgc_pkg_config_libdir_before "$ENV{PKG_CONFIG_LIBDIR}")
include(GStreamer/Orchestrator)

# EXPECT_PROBE_ROOT: probe the optional gstreamer-qgcprobe-1.0 library before the pkg-config environment is restored, as
# HwBuffers/CMakeLists.txt probes gst-d3d11 or gst-vulkan, and check that it and the core resolved to that SDK.
if(EXPECT_PROBE_ROOT)
    _qgc_probe_gst_lib(
        QGCProbe
        HAS_VAR
        QGC_GST_HAS_TEST_PROBE
        HEADER
        gst/qgcprobe.h
        SYMBOL
        qgc_probe
        PC_MODULE
        gstreamer-qgcprobe-1.0
        LIBNAMES
        gstqgcprobe-1.0
        LIB_VAR
        QGC_GST_TEST_PROBE_LIB
    )
    qgc_test_assert_streq("core version" "${EXPECT_VERSION}" "${GStreamer_VERSION}")
    qgc_test_assert_property("core include dirs" GStreamer::GStreamer INTERFACE_INCLUDE_DIRECTORIES
                             "${EXPECT_PROBE_ROOT}/include/gstreamer-1.0"
    )
    qgc_test_assert_streq("probe found" TRUE "${QGCProbe_FOUND}")
    qgc_test_assert_streq("probe include dirs" "${EXPECT_PROBE_ROOT}/include/gstreamer-1.0" "${QGCProbe_INCLUDE_DIRS}")
    qgc_test_assert_streq("probe library" "${EXPECT_PROBE_ROOT}/lib/libgstqgcprobe-1.0.so" "${QGCProbe_LIBRARIES}")
endif()

# Discovery state stays out of the cache and the global search paths; the derived pkg-config flags
# remain visible to the rest of the GStreamer directory and the environment is restored afterwards.
if(NOT DEFINED EXPECT_CACHED_PKG_CONFIG_ARGN)
    set(EXPECT_CACHED_PKG_CONFIG_ARGN "")
endif()
qgc_test_assert_streq("cached PKG_CONFIG_ARGN" "${EXPECT_CACHED_PKG_CONFIG_ARGN}" "$CACHE{PKG_CONFIG_ARGN}")
if(GStreamer_USE_STATIC_LIBS)
    qgc_test_assert_in_list("derived pkg-config flags" --static PKG_CONFIG_ARGN)
endif()
qgc_test_assert_streq("CMAKE_PREFIX_PATH" "${_qgc_prefix_path_before}" "${CMAKE_PREFIX_PATH}")
qgc_gst_restore_pkgconfig_env()
qgc_test_assert_streq("restored PKG_CONFIG_PATH" "${_qgc_pkg_config_path_before}" "$ENV{PKG_CONFIG_PATH}")
qgc_test_assert_streq("restored PKG_CONFIG_LIBDIR" "${_qgc_pkg_config_libdir_before}" "$ENV{PKG_CONFIG_LIBDIR}")

set(_qgc_state_targets
    GStreamer::GStreamer
    GStreamer::deps
    GStreamer::api_base
    GStreamer::api_rtsp
    GStreamer::api_video
    GStreamer::api_app
    GStreamer::api_allocators
)
# EXPECT_GL=OFF: the SDK lacks the optional gst-gl modules, so configure succeeds without their targets.
set(_qgc_gl_targets GStreamer::api_gl GStreamer::api_gl_prototypes)
if(NOT DEFINED EXPECT_GL OR EXPECT_GL)
    list(APPEND _qgc_state_targets ${_qgc_gl_targets})
    qgc_test_assert_streq("Gl found" TRUE "${GStreamer_Gl_FOUND}")
else()
    foreach(_target IN LISTS _qgc_gl_targets)
        if(TARGET ${_target})
            message(FATAL_ERROR "ASSERT [optional GL]: ${_target} was created")
        endif()
    endforeach()
    if(GStreamer_Gl_FOUND OR GStreamer_GlPrototypes_FOUND)
        message(FATAL_ERROR "ASSERT [optional GL]: GL reported found without its pkg-config modules")
    endif()
endif()
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
foreach(
    _var IN
    ITEMS GStreamer_VERSION
          GStreamer_ROOT_DIR
          GSTREAMER_LIB_PATH
          GSTREAMER_PLUGIN_PATH
          GSTREAMER_INCLUDE_PATH
          GSTREAMER_APIS
          GSTREAMER_PLUGINS
          QGC_GST_HAS_DMABUF
)
    string(APPEND _qgc_state "${_var}=${${_var}}\n")
endforeach()
foreach(_var IN ITEMS QGC_GST_RESOLVED_ROOT QGC_GST_RESOLVED_VERSION QGC_GST_RESOLVED_AUTO)
    string(APPEND _qgc_state "${_var}=$CACHE{${_var}}\n")
endforeach()
file(WRITE "${CMAKE_BINARY_DIR}/gst-state.txt" "${_qgc_state}")

# Links the app the way src/VideoManager/VideoReceiver/GStreamer/CMakeLists.txt does: through a static library
# carrying GStreamer::GStreamer, then GStreamer::mobile and the generated plugin registration.
if(QGC_TEST_ANDROID_ABI)
    add_library(QGCFakeGStreamer STATIC "${CMAKE_CURRENT_SOURCE_DIR}/fake_app.cpp")
    target_link_libraries(QGCFakeGStreamer PUBLIC GStreamer::GStreamer)
    target_link_libraries(${PROJECT_NAME} PRIVATE QGCFakeGStreamer GStreamer::mobile)
    qgc_gst_generate_static_init(TARGET ${PROJECT_NAME} PLUGINS ${GSTREAMER_STATIC_PLUGINS} GIO_MODULES ${G_IO_MODULES})
endif()

# QGC_TEST_INSTALL_REQUIRED_PLUGINS: comma-separated plugins the install rules must verify.
if(QGC_TEST_INSTALL_REQUIRED_PLUGINS)
    string(REPLACE "," ";" _qgc_required_plugins "${QGC_TEST_INSTALL_REQUIRED_PLUGINS}")
    gstreamer_install_platform_sdk(QGCFake REQUIRED_PLUGINS ${_qgc_required_plugins})
endif()
# QGC_TEST_INSTALL_MACOS_FRAMEWORK: install this GStreamer.framework into a QGCFake.app bundle.
# QGC_TEST_INSTALL_MACOS_FLAT: install the SDK's .dylib files into a QGCFake.app bundle.
if(QGC_TEST_INSTALL_MACOS_FRAMEWORK)
    gstreamer_layout_set(FRAMEWORK_BUNDLE "${QGC_TEST_INSTALL_MACOS_FRAMEWORK}")
    gstreamer_install_macos_sdk(QGCFake _qgc_plugin_dest)
    qgc_test_assert_streq("framework plugin dir"
                          "QGCFake.app/Contents/Frameworks/GStreamer.framework/Versions/1.0/lib/gstreamer-1.0"
                          "${_qgc_plugin_dest}"
    )
elseif(QGC_TEST_INSTALL_MACOS_FLAT)
    gstreamer_install_macos_sdk(QGCFake _qgc_plugin_dest)
    qgc_test_assert_streq("flat plugin dir" "QGCFake.app/Contents/lib/gstreamer-1.0" "${_qgc_plugin_dest}")
endif()

if(NOT DEFINED EXPECT_ROOT)
    return()
endif()

qgc_test_assert_streq("version" "${EXPECT_VERSION}" "${GStreamer_VERSION}")
qgc_test_assert_property("core include dirs" GStreamer::GStreamer INTERFACE_INCLUDE_DIRECTORIES
                         "${EXPECT_ROOT}/include/gstreamer-1.0"
)
qgc_test_assert_property("video include dirs" GStreamer::api_video INTERFACE_INCLUDE_DIRECTORIES
                         "${EXPECT_ROOT}/include/gstreamer-1.0"
)
get_target_property(_qgc_core_libs GStreamer::GStreamer INTERFACE_LINK_LIBRARIES)
qgc_test_assert_in_list("core links video" GStreamer::api_video _qgc_core_libs)
if(GStreamer_USE_STATIC_LIBS)
    # Static layouts resolve archives and link a target per plugin that has a .pc file.
    qgc_test_assert_in_list("core archive" "${EXPECT_ROOT}/lib/libgstreamer-1.0.a" _qgc_core_libs)
    qgc_test_assert_in_list("core links plugin" GStreamer::coreelements _qgc_core_libs)
    qgc_test_assert_property("video libraries" GStreamer::api_video INTERFACE_LINK_LIBRARIES
                             "${EXPECT_ROOT}/lib/libgstvideo-1.0.a"
    )
    qgc_test_assert_property("plugin libraries" GStreamer::coreelements INTERFACE_LINK_LIBRARIES
                             "${EXPECT_ROOT}/lib/gstreamer-1.0/libgstcoreelements.a"
    )
    # The static plugins get a generated gst_init_static_plugins() compiled into the consuming target.
    qgc_test_assert_streq("static plugins" coreelements "${GSTREAMER_STATIC_PLUGINS}")
    add_library(QGCFakeApp STATIC "${CMAKE_CURRENT_SOURCE_DIR}/fake_app.cpp")
    qgc_gst_generate_static_init(TARGET QGCFakeApp PLUGINS ${GSTREAMER_STATIC_PLUGINS})
    set(_qgc_static_init "${CMAKE_CURRENT_BINARY_DIR}/QGCFakeApp_gst_static_plugins.c")
    get_target_property(_qgc_app_sources QGCFakeApp SOURCES)
    qgc_test_assert_in_list("static init source" "${_qgc_static_init}" _qgc_app_sources)
    get_source_file_property(_qgc_skip_pch "${_qgc_static_init}" TARGET_DIRECTORY QGCFakeApp SKIP_PRECOMPILE_HEADERS)
    qgc_test_assert_streq("static init skips the precompiled header" ON "${_qgc_skip_pch}")
else()
    qgc_test_assert_property("core libraries" GStreamer::deps INTERFACE_LINK_LIBRARIES
                             "${EXPECT_ROOT}/lib/libgstreamer-1.0.so"
    )
    qgc_test_assert_property("video libraries" GStreamer::api_video INTERFACE_LINK_LIBRARIES
                             "${EXPECT_ROOT}/lib/libgstvideo-1.0.so"
    )
    # Shared layouts load plugins at runtime, so no plugin target is created.
    if(TARGET GStreamer::coreelements)
        message(FATAL_ERROR "ASSERT [shared plugins]: GStreamer::coreelements was created")
    endif()
endif()
qgc_test_assert_streq("plugin dir" "${EXPECT_ROOT}/lib/gstreamer-1.0" "${GSTREAMER_PLUGIN_PATH}")
qgc_test_assert_streq("recorded version" "${EXPECT_VERSION}" "$CACHE{QGC_GST_RESOLVED_SDK_VERSION}")
if(NOT DEFINED EXPECT_AUTO)
    set(EXPECT_AUTO FALSE)
endif()
qgc_test_assert_streq("recorded root" "${EXPECT_ROOT}" "$CACHE{QGC_GST_RESOLVED_ROOT}")
qgc_test_assert_streq("recorded auto-downloaded" "${EXPECT_AUTO}" "$CACHE{QGC_GST_RESOLVED_AUTO}")

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
