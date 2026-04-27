qt_add_library(gstqml6gl STATIC)

target_link_libraries(gstqml6gl PUBLIC GStreamer::GStreamer)

# GL/video headers (gstglconfig.h etc.) live in component-specific include dirs
foreach(_gst_api api_gl api_gl_prototypes api_video)
    if(TARGET GStreamer::${_gst_api})
        target_link_libraries(gstqml6gl PUBLIC GStreamer::${_gst_api})
    endif()
endforeach()

include(GStreamerHelpers)

if(GStreamer_VERSION VERSION_GREATER_EQUAL "1.22")
    gstreamer_get_recommended_version("${GStreamer_VERSION}" _gst_plugins_version)

    set(_gst_qt6_cache "${CMAKE_BINARY_DIR}/_deps/gstreamer-good-plugins")
    gstreamer_download_sdk(good_plugins ${_gst_plugins_version}
        "gst-plugins-good-${_gst_plugins_version}.tar.xz" "${_gst_qt6_cache}" _gst_good_archive
        ALLOW_FAILURE)

    if(NOT _gst_good_archive)
        message(STATUS "gstqml6gl: Primary tarball unavailable, trying GitLab qt6 fallback")
        gstreamer_get_package_url(good_plugins_qt6 ${_gst_plugins_version} _gst_qt6_url)
        gstreamer_resilient_download(
            URLS "${_gst_qt6_url}"
            FILENAME "gstreamer-${_gst_plugins_version}-qt6.tar.gz"
            DESTINATION_DIR "${_gst_qt6_cache}"
            RESULT_VAR _gst_good_archive
        )
    endif()

    if(NOT _gst_good_archive)
        message(FATAL_ERROR "gstqml6gl: Could not download gst-plugins-good sources")
    endif()

    CPMAddPackage(
        NAME gstreamer_good_plugins
        VERSION ${_gst_plugins_version}
        URL "file://${_gst_good_archive}"
        DOWNLOAD_ONLY YES
    )

    if(gstreamer_good_plugins_SOURCE_DIR)
        if(EXISTS "${gstreamer_good_plugins_SOURCE_DIR}/ext/qt6")
            set(QGC_GST_QT6_PLUGIN_PATH "${gstreamer_good_plugins_SOURCE_DIR}/ext/qt6")
        elseif(EXISTS "${gstreamer_good_plugins_SOURCE_DIR}/subprojects/gst-plugins-good/ext/qt6")
            set(QGC_GST_QT6_PLUGIN_PATH "${gstreamer_good_plugins_SOURCE_DIR}/subprojects/gst-plugins-good/ext/qt6")
        endif()
    endif()

    if(NOT QGC_GST_QT6_PLUGIN_PATH)
        message(FATAL_ERROR "gstqml6gl: ext/qt6 not found in downloaded sources")
    endif()
else()
    message(STATUS "gstqml6gl: GStreamer < 1.22, using local fallback sources")
    set(QGC_GST_QT6_PLUGIN_PATH "${CMAKE_CURRENT_SOURCE_DIR}/qt6")
endif()

# Copy to build tree to avoid modifying CPM's download cache
set(_qt6_build_sources "${CMAKE_CURRENT_BINARY_DIR}/qt6_sources")
file(REMOVE_RECURSE "${_qt6_build_sources}")
file(COPY "${QGC_GST_QT6_PLUGIN_PATH}/" DESTINATION "${_qt6_build_sources}")
set(QGC_GST_QT6_PLUGIN_PATH "${_qt6_build_sources}")

# Patch missing QQuickWindow include that breaks moc with newer Qt
set(_qt6glitem_header "${QGC_GST_QT6_PLUGIN_PATH}/qt6glitem.h")
if(EXISTS "${_qt6glitem_header}")
    file(READ "${_qt6glitem_header}" _file_contents)
    string(FIND "${_file_contents}" "#include <QtQuick/QQuickWindow>" _has_include)
    if(_has_include EQUAL -1)
        string(REPLACE
            "#include <QtGui/QOpenGLFunctions>"
            "#include <QtGui/QOpenGLFunctions>\n#include <QtQuick/QQuickWindow>"
            _file_contents "${_file_contents}")
        file(WRITE "${_qt6glitem_header}" "${_file_contents}")
    endif()
endif()

file(GLOB _gl_srcs CONFIGURE_DEPENDS
    "${QGC_GST_QT6_PLUGIN_PATH}/*.cc"
    "${QGC_GST_QT6_PLUGIN_PATH}/*.h"
)
target_sources(gstqml6gl PRIVATE ${_gl_srcs}
    gstqml6glregister.cpp
    gstqml6glregister.h
)

qgc_disable_dependency_warnings(gstqml6gl)

target_link_libraries(gstqml6gl
    PRIVATE
        Qt6::GuiPrivate
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
)

target_include_directories(gstqml6gl PUBLIC ${QGC_GST_QT6_PLUGIN_PATH} ${CMAKE_CURRENT_SOURCE_DIR})

target_precompile_headers(gstqml6gl
    PRIVATE
        <gst/gst.h>
        <gst/gl/gl.h>
        <gst/video/video.h>
        # 6 TUs (qt6glitem/qsg6material/qml6gl{mixer,overlay,sink,src}) hit
        # qquickitem.h -> qqml.h cold; share via PCH.
        <QtCore/QObject>
        <QtCore/QString>
        <QtGui/QOpenGLFunctions>
        <QtQml/QQmlEngine>
        <QtQuick/QQuickItem>
        <QtQuick/QQuickWindow>
)

file(GLOB _shader_files
    "${QGC_GST_QT6_PLUGIN_PATH}/*.vert"
    "${QGC_GST_QT6_PLUGIN_PATH}/*.frag"
)
list(FILTER _shader_files EXCLUDE REGEX "_gles\\.frag$")

if(_shader_files)
    find_package(Qt6 REQUIRED COMPONENTS ShaderTools)

    set(_outputs)
    foreach(_shader IN LISTS _shader_files)
        get_filename_component(_name "${_shader}" NAME)
        list(APPEND _outputs "${_name}.qsb")
    endforeach()

    qt_add_shaders(gstqml6gl "gstqml6gl_shaders"
        PREFIX "/org/freedesktop/gstreamer/qml6"
        GLSL "100 es,120,330"
        OUTPUT_TARGETS gstqml6gl_shaders
        FILES ${_shader_files}
        OUTPUTS ${_outputs}
        BATCHABLE
    )

    if(EXISTS "${QGC_GST_QT6_PLUGIN_PATH}/RGBA_gles.frag")
        find_package(Python3 REQUIRED COMPONENTS Interpreter)

        find_program(QSB_PROGRAM
            NAMES qsb
            HINTS ${QT_HOST_PATH} ${QT_ROOT_DIR} ${QTDIR}
            ENV QTDIR
            PATH_SUFFIXES bin
            REQUIRED
        )

        set(_qsb_wrapper "${QGC_GST_QT6_PLUGIN_PATH}/qsb-wrapper.py")
        if(NOT EXISTS "${_qsb_wrapper}")
            message(FATAL_ERROR "qsb-wrapper.py not found at ${_qsb_wrapper}")
        endif()

        set(_rgba_base_qsb "${CMAKE_CURRENT_BINARY_DIR}/.qsb/RGBA.frag.qsb")
        set(_rgba_gles_src "${QGC_GST_QT6_PLUGIN_PATH}/RGBA_gles.frag")
        set(_rgba_external "${CMAKE_CURRENT_BINARY_DIR}/.qsb/RGBA.frag.qsb.external")

        file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/.qsb")

        add_custom_command(
            OUTPUT ${_rgba_external}
            COMMAND ${Python3_EXECUTABLE}
                ${_qsb_wrapper}
                ${QSB_PROGRAM}
                ${_rgba_external}
                ${_rgba_gles_src}
                ${_rgba_base_qsb}
            DEPENDS
                ${_rgba_base_qsb}
                ${_rgba_gles_src}
                ${_qsb_wrapper}
            COMMENT "Generating external GLES shader: ${_rgba_external}"
            VERBATIM
        )

        add_custom_target(gstqml6gl_external_shader DEPENDS ${_rgba_external})
        if(gstqml6gl_shaders)
            add_dependencies(gstqml6gl_external_shader ${gstqml6gl_shaders})
        endif()

        set_source_files_properties("${_rgba_external}" PROPERTIES QT_RESOURCE_ALIAS RGBA.frag.qsb.external)
        qt_add_resources(gstqml6gl "gstqml6gl_shaders1"
            PREFIX "/org/freedesktop/gstreamer/qml6"
            FILES "${_rgba_external}"
        )

        add_dependencies(gstqml6gl gstqml6gl_external_shader)
    endif()
endif()

if(GStreamer_GlX11_FOUND)
    target_compile_definitions(gstqml6gl PRIVATE HAVE_QT_X11)
endif()

if(GStreamer_GlEgl_FOUND)
    target_compile_definitions(gstqml6gl PRIVATE HAVE_QT_EGLFS)
endif()

if(GStreamer_GlWayland_FOUND)
    find_package(Qt6 COMPONENTS WaylandClient)
    if(TARGET Qt6::WaylandClient)
        target_link_libraries(gstqml6gl PRIVATE Qt6::WaylandClient)
        target_compile_definitions(gstqml6gl PRIVATE HAVE_QT_WAYLAND)
    endif()
endif()

if(ANDROID)
    target_compile_definitions(gstqml6gl PRIVATE HAVE_QT_ANDROID)
elseif(WIN32)
    target_compile_definitions(gstqml6gl PRIVATE HAVE_QT_WIN32)
elseif(MACOS)
    target_compile_definitions(gstqml6gl PRIVATE HAVE_QT_MAC)
elseif(IOS)
    target_compile_definitions(gstqml6gl PRIVATE HAVE_QT_IOS)
endif()

target_compile_definitions(gstqml6gl
    PRIVATE
        HAVE_QT_QPA_HEADER
        QT_QPA_HEADER=<QtGui/qpa/qplatformnativeinterface.h>
)
