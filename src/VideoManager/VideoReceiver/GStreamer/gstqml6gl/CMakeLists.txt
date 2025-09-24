# ============================================================================
# GStreamer Qt6 QML GL Plugin
# Qt6 integration for GStreamer video rendering
# ============================================================================

qt_add_library(gstqml6gl STATIC)

# ----------------------------------------------------------------------------
# Platform-Specific GStreamer Linking
# ----------------------------------------------------------------------------
if(MACOS)
    # NOTE: Using FindGStreamer.cmake is currently bypassed on macOS
    # Using hardwired framework path as a workaround
    find_library(GSTREAMER_FRAMEWORK GStreamer)
    set(GST_PLUGINS_VERSION 1.24.12)
    set(GSTREAMER_FRAMEWORK_PATH "/Library/Frameworks/GStreamer.framework")
    if(NOT EXISTS "${GSTREAMER_FRAMEWORK_PATH}")
        message(FATAL_ERROR "GStreamer.framework not found at ${GSTREAMER_FRAMEWORK_PATH}. Install GStreamer using tools/setup/install-dependencies-osx.sh script")
    endif()
    target_link_libraries(gstqml6gl PUBLIC "$<LINK_LIBRARY:FRAMEWORK,${GSTREAMER_FRAMEWORK_PATH}>")
    target_include_directories(gstqml6gl PUBLIC "${GSTREAMER_FRAMEWORK_PATH}/Headers")
else()
    target_link_libraries(gstqml6gl PUBLIC GStreamer::GStreamer)
endif()

# TODO: Skip custom build if system gstreamer1.0-qt6 plugin is found
# if(GST_PLUGIN_qml6_FOUND)
#     return()
# endif()

# ============================================================================
# GStreamer Good Plugins Integration
# ============================================================================

if(GStreamer_VERSION VERSION_GREATER_EQUAL "1.22")
    # Use Latest Revisions for each minor version: 1.20.7, 1.22.12, 1.24.13, 1.26.6
    string(REPLACE "." ";" GST_VERSION_LIST "${GStreamer_VERSION}")
    list(GET GST_VERSION_LIST 0 GST_VERSION_MAJOR)
    list(GET GST_VERSION_LIST 1 GST_VERSION_MINOR)
    list(GET GST_VERSION_LIST 2 GST_VERSION_PATCH)

    if(GST_VERSION_MINOR EQUAL 20)
        set(GST_VERSION_PATCH 7)
    elseif(GST_VERSION_MINOR EQUAL 22)
        set(GST_VERSION_PATCH 12)
    elseif(GST_VERSION_MINOR EQUAL 24)
        set(GST_VERSION_PATCH 13)
    elseif(GST_VERSION_MINOR EQUAL 26)
        set(GST_VERSION_PATCH 6)
    endif()

    set(GST_PLUGINS_VERSION ${GST_VERSION_MAJOR}.${GST_VERSION_MINOR}.${GST_VERSION_PATCH})

    # Download GStreamer good plugins for Qt6 support
    CPMAddPackage(
        NAME gstreamer_good_plugins
        VERSION ${GST_PLUGINS_VERSION}
        URL https://gstreamer.freedesktop.org/src/gst-plugins-good/gst-plugins-good-${GST_PLUGINS_VERSION}.tar.xz
        # TODO: Add URL_HASH for verification
        # URL_HASH https://gstreamer.freedesktop.org/src/gst-plugins-good/gst-plugins-good-${GST_PLUGINS_VERSION}.tar.xz.sha256sum
    )
    set(QGC_GST_QT6_PLUGIN_PATH "${gstreamer_good_plugins_SOURCE_DIR}/ext/qt6")
else()
    set(QGC_GST_QT6_PLUGIN_PATH "${CMAKE_CURRENT_SOURCE_DIR}/qt6")
endif()

# ============================================================================
# Source Code Patching
# ============================================================================

# Fix missing includes in GStreamer Qt6 plugin
file(READ "${QGC_GST_QT6_PLUGIN_PATH}/qt6glitem.h" FILE_CONTENTS)
string(FIND "${FILE_CONTENTS}" "#include <QtQuick/QQuickWindow>" GST_FIX_INCLUDES)
if(GST_FIX_INCLUDES EQUAL -1)
    string(REPLACE "#include <QtGui/QOpenGLFunctions>" "#include <QtGui/QOpenGLFunctions>\n#include <QtQuick/QQuickWindow>" FILE_CONTENTS "${FILE_CONTENTS}")
    file(WRITE "${QGC_GST_QT6_PLUGIN_PATH}/qt6glitem.h" "${FILE_CONTENTS}")
endif()

# ----------------------------------------------------------------------------
# Build GStreamer QML6 GL Library
# ----------------------------------------------------------------------------

# NOTE: Using file(GLOB) for external GStreamer plugin sources
# CONFIGURE_DEPENDS ensures CMake re-runs if files are added/removed
file(GLOB gstqml6gl_SRCS
    CONFIGURE_DEPENDS
    "${QGC_GST_QT6_PLUGIN_PATH}/*.cc"
    "${QGC_GST_QT6_PLUGIN_PATH}/*.h"
)
target_sources(gstqml6gl PRIVATE ${gstqml6gl_SRCS})

target_link_libraries(gstqml6gl
    PRIVATE
        Qt6::GuiPrivate
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
)

target_include_directories(gstqml6gl PUBLIC ${QGC_GST_QT6_PLUGIN_PATH})

target_precompile_headers(gstqml6gl
    PRIVATE
        <gst/gst.h>
        <gst/gl/gl.h>
        <gst/video/video.h>
)

# ============================================================================
# Shader Compilation
# ============================================================================

if(EXISTS "${QGC_GST_QT6_PLUGIN_PATH}/resources.qrc")
    find_package(Qt6 REQUIRED COMPONENTS ShaderTools)

    set(SHADERS
        "${QGC_GST_QT6_PLUGIN_PATH}/vertex.vert"
        "${QGC_GST_QT6_PLUGIN_PATH}/YUV_TRIPLANAR.frag"
        "${QGC_GST_QT6_PLUGIN_PATH}/RGBA.frag"
    )

    set(OUTPUTS
        vertex.vert.qsb
        YUV_TRIPLANAR.frag.qsb
        RGBA.frag.qsb
    )

    if(EXISTS "${QGC_GST_QT6_PLUGIN_PATH}/YUV_BIPLANAR.frag")
        LIST(APPEND SHADERS "${QGC_GST_QT6_PLUGIN_PATH}/YUV_BIPLANAR.frag")
        LIST(APPEND OUTPUTS "YUV_BIPLANAR.frag.qsb")
    endif()

    # Compile shaders for different GLSL versions
    qt_add_shaders(gstqml6gl "gstqml6gl_shaders"
        PREFIX "/org/freedesktop/gstreamer/qml6"
        GLSL "100 es,120,330"
        OUTPUT_TARGETS gstqml6gl_shaders
        FILES ${SHADERS}
        OUTPUTS ${OUTPUTS}
        BATCHABLE
    )

    # Handle additional GLES-specific RGBA shader if present
    if(EXISTS "${QGC_GST_QT6_PLUGIN_PATH}/RGBA_gles.frag")
        find_program(QSB_PROGRAM
            NAMES qsb
            HINTS ${QT_HOST_PATH} ${QT_ROOT_DIR} ${QTDIR}
            ENV QTDIR
            PATH_SUFFIXES bin
            REQUIRED
        )

        find_program(QSB_WRAPPER
            NAMES qsb-wrapper.py
            PATHS "${QGC_GST_QT6_PLUGIN_PATH}"
            REQUIRED
        )

        set(RGBA_BASE_QSB "${CMAKE_CURRENT_BINARY_DIR}/.qsb/RGBA.frag.qsb")
        set(RGBA_GLES_SRC "${QGC_GST_QT6_PLUGIN_PATH}/RGBA_gles.frag")
        set(RGBA_EXTERNAL "${CMAKE_CURRENT_BINARY_DIR}/.qsb/RGBA.frag.qsb.external")

        add_custom_command(
            OUTPUT ${RGBA_EXTERNAL}
            COMMAND ${QSB_WRAPPER}
                ${QSB_PROGRAM}
                ${RGBA_EXTERNAL}
                ${RGBA_GLES_SRC}
                ${RGBA_BASE_QSB}
            DEPENDS
                ${RGBA_BASE_QSB}
                ${RGBA_GLES_SRC}
            COMMENT "Generating external GLES shader: ${RGBA_EXTERNAL}"
            VERBATIM
        )

        set_source_files_properties("${RGBA_EXTERNAL}" PROPERTIES QT_RESOURCE_ALIAS RGBA.frag.qsb.external)
        qt_add_resources(gstqml6gl "gstqml6gl_shaders1"
            PREFIX "/org/freedesktop/gstreamer/qml6"
            FILES "${RGBA_EXTERNAL}"
        )
    endif()
endif()

# ============================================================================
# Platform-Specific OpenGL Support
# ============================================================================

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
    endif()
    target_compile_definitions(gstqml6gl PRIVATE HAVE_QT_WAYLAND)
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
