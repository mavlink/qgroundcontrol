if(NOT WIN32 OR ANDROID)
    return()
endif()

if(NOT GStreamer_VERSION VERSION_GREATER_EQUAL "1.24")
    message(STATUS "gstqml6d3d11: Requires GStreamer >= 1.24 (have ${GStreamer_VERSION}), skipping")
    return()
endif()

include(GStreamerHelpers)

gstreamer_get_recommended_version("${GStreamer_VERSION}" _d3d11_version)

set(_d3d11_cache "${CMAKE_BINARY_DIR}/_deps/gstreamer-bad-qt6d3d11")

gstreamer_get_package_url(bad_plugins_qt6d3d11 ${_d3d11_version} _d3d11_url)
gstreamer_resilient_download(
    URLS "${_d3d11_url}"
    FILENAME "gstreamer-${_d3d11_version}-qt6d3d11.tar.gz"
    DESTINATION_DIR "${_d3d11_cache}"
    RESULT_VAR _d3d11_archive
    ALLOW_FAILURE
)

if(NOT _d3d11_archive)
    message(STATUS "gstqml6d3d11: Download failed, D3D11 sink unavailable")
    return()
endif()

CPMAddPackage(
    NAME gstreamer_bad_qt6d3d11
    VERSION ${_d3d11_version}
    URL "file://${_d3d11_archive}"
    DOWNLOAD_ONLY YES
)

set(_d3d11_src_path "")
if(gstreamer_bad_qt6d3d11_SOURCE_DIR)
    if(EXISTS "${gstreamer_bad_qt6d3d11_SOURCE_DIR}/ext/qt6d3d11")
        set(_d3d11_src_path "${gstreamer_bad_qt6d3d11_SOURCE_DIR}/ext/qt6d3d11")
    elseif(EXISTS "${gstreamer_bad_qt6d3d11_SOURCE_DIR}/subprojects/gst-plugins-bad/ext/qt6d3d11")
        set(_d3d11_src_path "${gstreamer_bad_qt6d3d11_SOURCE_DIR}/subprojects/gst-plugins-bad/ext/qt6d3d11")
    endif()
endif()

if(NOT _d3d11_src_path)
    message(STATUS "gstqml6d3d11: ext/qt6d3d11 not found in sources, D3D11 sink unavailable")
    return()
endif()

# Copy to build tree to avoid modifying CPM's download cache
set(_d3d11_build_sources "${CMAKE_CURRENT_BINARY_DIR}/qt6d3d11_sources")
file(REMOVE_RECURSE "${_d3d11_build_sources}")
file(COPY "${_d3d11_src_path}/" DESTINATION "${_d3d11_build_sources}")

file(GLOB _d3d11_srcs CONFIGURE_DEPENDS
    "${_d3d11_build_sources}/*.cpp"
    "${_d3d11_build_sources}/*.cc"
    "${_d3d11_build_sources}/*.h"
)

if(NOT _d3d11_srcs)
    message(STATUS "gstqml6d3d11: No sources found, skipping")
    return()
endif()

# SDK doesn't ship gstd3d11-private.h — download from monorepo if missing
set(_d3d11_hdr_dir "${_d3d11_build_sources}/gst/d3d11")
if(NOT EXISTS "${GSTREAMER_INCLUDE_PATH}/gstreamer-1.0/gst/d3d11/gstd3d11-private.h")
    file(MAKE_DIRECTORY "${_d3d11_hdr_dir}")
    set(_d3d11_hdr_base "https://gitlab.freedesktop.org/gstreamer/gstreamer/-/raw/${_d3d11_version}/subprojects/gst-plugins-bad/gst-libs/gst/d3d11")
    foreach(_hdr IN ITEMS gstd3d11-private.h)
        if(NOT EXISTS "${_d3d11_hdr_dir}/${_hdr}")
            set(_hdr_tmp "${_d3d11_hdr_dir}/${_hdr}.tmp")
            set(_hdr_ok FALSE)
            foreach(_attempt RANGE 1 3)
                file(DOWNLOAD "${_d3d11_hdr_base}/${_hdr}" "${_hdr_tmp}"
                    STATUS _dl_status TIMEOUT 30 TLS_VERIFY ON)
                list(GET _dl_status 0 _dl_code)
                if(_dl_code EQUAL 0)
                    file(RENAME "${_hdr_tmp}" "${_d3d11_hdr_dir}/${_hdr}")
                    set(_hdr_ok TRUE)
                    break()
                endif()
                file(REMOVE "${_hdr_tmp}")
            endforeach()
            if(NOT _hdr_ok)
                message(STATUS "gstqml6d3d11: Failed to download ${_hdr}, D3D11 sink unavailable")
                return()
            endif()
        endif()
    endforeach()
endif()

if(DEFINED _gst_d3d11_lib AND NOT EXISTS "${_gst_d3d11_lib}")
    unset(_gst_d3d11_lib CACHE)
endif()
find_library(_gst_d3d11_lib gstd3d11-1.0
    HINTS "${GSTREAMER_LIB_PATH}"
    NO_DEFAULT_PATH
)
if(NOT _gst_d3d11_lib)
    message(STATUS "gstqml6d3d11: gstd3d11-1.0 not found in SDK, D3D11 sink unavailable")
    return()
endif()

set(_d3d11_generated "${CMAKE_CURRENT_BINARY_DIR}/qt6d3d11_generated")
file(MAKE_DIRECTORY "${_d3d11_generated}")
file(WRITE "${_d3d11_generated}/config.h"
    "#pragma once\n"
    "#define VERSION \"${_d3d11_version}\"\n"
    "#define PACKAGE \"gst-plugins-bad\"\n"
    "#define GST_PACKAGE_NAME \"GStreamer Bad Plug-ins\"\n"
    "#define GST_PACKAGE_ORIGIN \"https://gstreamer.freedesktop.org\"\n"
)

qt_add_library(gstqml6d3d11 STATIC)

target_sources(gstqml6d3d11
    PRIVATE
        ${_d3d11_srcs}
        "${CMAKE_CURRENT_SOURCE_DIR}/gstqml6d3d11register.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/gstqml6d3d11register.h"
)
target_link_libraries(gstqml6d3d11 PUBLIC GStreamer::GStreamer)

qgc_disable_dependency_warnings(gstqml6d3d11)

target_link_libraries(gstqml6d3d11
    PRIVATE
        Qt6::GuiPrivate
        "${_gst_d3d11_lib}"
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
        d3d11
        dxgi
)

target_include_directories(gstqml6d3d11
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${_d3d11_build_sources}"
    PRIVATE
        "${_d3d11_generated}"
        "${_d3d11_build_sources}"
        "${GSTREAMER_INCLUDE_PATH}/gstreamer-1.0"
        "${GSTREAMER_INCLUDE_PATH}/glib-2.0"
        "${GSTREAMER_LIB_PATH}/glib-2.0/include"
)

target_precompile_headers(gstqml6d3d11
    PRIVATE
        <gst/gst.h>
        <gst/video/video.h>
)

target_compile_definitions(gstqml6d3d11 PRIVATE
    GST_USE_UNSTABLE_API
    HAVE_CONFIG_H
    HAVE_D3D11_SINK
)
