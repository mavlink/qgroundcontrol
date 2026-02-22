# ============================================================================
# GStreamer Qt6 QML GL Plugin
# Qt6 integration for GStreamer video rendering
# ============================================================================

qt_add_library(gstqml6gl STATIC)

# On mobile platforms, link GStreamer::mobile (shared lib with all plugins)
# On desktop, link GStreamer::GStreamer (uses system GStreamer)
if((ANDROID OR IOS) AND TARGET GStreamer::mobile)
    # GStreamer::mobile is libgstreamer_android.so which contains all static
    # plugins. Don't also link GStreamer::GStreamer as that would duplicate
    # the static libraries.
    target_link_libraries(gstqml6gl PUBLIC GStreamer::mobile)
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

message(STATUS "gstqml6gl: GStreamer_VERSION = '${GStreamer_VERSION}'")

if(GStreamer_VERSION VERSION_GREATER_EQUAL "1.22")
    # Get recommended stable version for this GStreamer release
    string(REPLACE "." ";" GST_VERSION_LIST "${GStreamer_VERSION}")
    list(GET GST_VERSION_LIST 0 GST_VERSION_MAJOR)
    list(GET GST_VERSION_LIST 1 GST_VERSION_MINOR)
    set(GST_VERSION_PATCH "")
    if(GStreamer_VERSION MATCHES "^${GST_VERSION_MAJOR}\\.${GST_VERSION_MINOR}\\.([0-9]+)")
        set(GST_VERSION_PATCH "${CMAKE_MATCH_1}")
    endif()

    # Use centralized version mapping from GStreamerHelpers.cmake
    gstreamer_get_recommended_version(
        ${GST_VERSION_MAJOR}
        ${GST_VERSION_MINOR}
        GST_PLUGINS_VERSION
        "${GST_VERSION_PATCH}"
    )

    # Download gst-plugins-good sources for ext/qt6.
    # Primary: standalone tarball from freedesktop.org (~4MB)
    # Fallback: path-filtered GitLab archive of just ext/qt6 (~few KB)
    gstreamer_get_package_url(good_plugins ${GST_PLUGINS_VERSION} _gst_good_url)
    gstreamer_get_package_url(good_plugins_qt6 ${GST_PLUGINS_VERSION} _gst_qt6_url)
    gstreamer_fetch_checksum(good_plugins ${GST_PLUGINS_VERSION} _gst_good_hash)
    _gstreamer_is_release_like_config(_gst_release_like)

    set(_gst_qt6_cache "${CMAKE_BINARY_DIR}/_deps/gstreamer-good-plugins")
    gstreamer_resilient_download(
        URLS "${_gst_good_url}"
        FILENAME "gst-plugins-good-${GST_PLUGINS_VERSION}.tar.xz"
        DESTINATION_DIR "${_gst_qt6_cache}"
        RESULT_VAR _gst_good_archive
        EXPECTED_HASH "${_gst_good_hash}"
        ALLOW_FAILURE
    )

    if(NOT _gst_good_archive)
        if(_gst_release_like)
            message(FATAL_ERROR
                "gstqml6gl: Primary gst-plugins-good source download failed.\n"
                "Release-like builds do not allow the unverified GitLab qt6 fallback archive.")
        endif()
        message(STATUS "gstqml6gl: Primary source tarball unavailable, trying GitLab qt6 fallback archive")
        gstreamer_resilient_download(
            URLS "${_gst_qt6_url}"
            FILENAME "gstreamer-${GST_PLUGINS_VERSION}-qt6.tar.gz"
            DESTINATION_DIR "${_gst_qt6_cache}"
            RESULT_VAR _gst_good_archive
        )
    endif()

    CPMAddPackage(
        NAME gstreamer_good_plugins
        VERSION ${GST_PLUGINS_VERSION}
        URL "file://${_gst_good_archive}"
        DOWNLOAD_ONLY YES
    )

    # Standalone tarball: ext/qt6 at root
    # GitLab path-filtered archive: subprojects/gst-plugins-good/ext/qt6
    if(gstreamer_good_plugins_SOURCE_DIR)
        if(EXISTS "${gstreamer_good_plugins_SOURCE_DIR}/ext/qt6")
            set(QGC_GST_QT6_PLUGIN_PATH "${gstreamer_good_plugins_SOURCE_DIR}/ext/qt6")
        elseif(EXISTS "${gstreamer_good_plugins_SOURCE_DIR}/subprojects/gst-plugins-good/ext/qt6")
            set(QGC_GST_QT6_PLUGIN_PATH "${gstreamer_good_plugins_SOURCE_DIR}/subprojects/gst-plugins-good/ext/qt6")
        endif()
    endif()

    if(NOT QGC_GST_QT6_PLUGIN_PATH)
        message(FATAL_ERROR "Failed to locate ext/qt6 in downloaded gst-plugins-good sources")
    endif()
else()
    message(STATUS "gstqml6gl: GStreamer < 1.22, using local Qt6 fallback sources")
    set(QGC_GST_QT6_PLUGIN_PATH "${CMAKE_CURRENT_SOURCE_DIR}/qt6")
endif()

message(STATUS "gstqml6gl: QGC_GST_QT6_PLUGIN_PATH = ${QGC_GST_QT6_PLUGIN_PATH}")

# Copy to build tree so we don't modify CPM's download cache
set(_qt6_build_sources "${CMAKE_CURRENT_BINARY_DIR}/qt6_sources")
file(REMOVE_RECURSE "${_qt6_build_sources}")
file(COPY "${QGC_GST_QT6_PLUGIN_PATH}/" DESTINATION "${_qt6_build_sources}")
set(QGC_GST_QT6_PLUGIN_PATH "${_qt6_build_sources}")

# ============================================================================
# Source Code Patching
# ============================================================================

# Upstream qt6glitem.h can miss QQuickWindow include, which breaks moc-generated
# metatype code with newer Qt. Patch the copied source tree only.
set(_qt6glitem_header "${QGC_GST_QT6_PLUGIN_PATH}/qt6glitem.h")
if(EXISTS "${_qt6glitem_header}")
    file(READ "${_qt6glitem_header}" FILE_CONTENTS)
    string(FIND "${FILE_CONTENTS}" "#include <QtQuick/QQuickWindow>" GST_FIX_INCLUDES)
    if(GST_FIX_INCLUDES EQUAL -1)
        string(REPLACE
            "#include <QtGui/QOpenGLFunctions>"
            "#include <QtGui/QOpenGLFunctions>\n#include <QtQuick/QQuickWindow>"
            FILE_CONTENTS
            "${FILE_CONTENTS}"
        )
        file(WRITE "${_qt6glitem_header}" "${FILE_CONTENTS}")
    endif()
endif()

file(GLOB gstqml6gl_SRCS
    CONFIGURE_DEPENDS
    "${QGC_GST_QT6_PLUGIN_PATH}/*.cc"
    "${QGC_GST_QT6_PLUGIN_PATH}/*.h"
)
target_sources(gstqml6gl PRIVATE ${gstqml6gl_SRCS})

# Suppress compiler warnings for third-party GStreamer plugin sources
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

# Discover shader files dynamically to support different GStreamer versions
file(GLOB _shader_files
    "${QGC_GST_QT6_PLUGIN_PATH}/*.vert"
    "${QGC_GST_QT6_PLUGIN_PATH}/*.frag"
)
# GLES-specific shaders are handled separately via qsb-wrapper below
list(FILTER _shader_files EXCLUDE REGEX "_gles\\.frag$")

if(_shader_files)
    find_package(Qt6 REQUIRED COMPONENTS ShaderTools)

    set(SHADERS ${_shader_files})
    set(OUTPUTS)
    foreach(_shader IN LISTS SHADERS)
        get_filename_component(_name "${_shader}" NAME)
        list(APPEND OUTPUTS "${_name}.qsb")
    endforeach()

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
    # This generates RGBA.frag.qsb.external which adds GLES 100 support to the base shader
    if(EXISTS "${QGC_GST_QT6_PLUGIN_PATH}/RGBA_gles.frag")
        find_package(Python3 REQUIRED COMPONENTS Interpreter)

        find_program(QSB_PROGRAM
            NAMES qsb
            HINTS ${QT_HOST_PATH} ${QT_ROOT_DIR} ${QTDIR}
            ENV QTDIR
            PATH_SUFFIXES bin
            REQUIRED
        )

        set(QSB_WRAPPER "${QGC_GST_QT6_PLUGIN_PATH}/qsb-wrapper.py")
        if(NOT EXISTS "${QSB_WRAPPER}")
            message(FATAL_ERROR "qsb-wrapper.py not found at ${QSB_WRAPPER}")
        endif()

        # The base shader is generated by qt_add_shaders into the .qsb directory
        set(RGBA_BASE_QSB "${CMAKE_CURRENT_BINARY_DIR}/.qsb/RGBA.frag.qsb")
        set(RGBA_GLES_SRC "${QGC_GST_QT6_PLUGIN_PATH}/RGBA_gles.frag")
        set(RGBA_EXTERNAL "${CMAKE_CURRENT_BINARY_DIR}/.qsb/RGBA.frag.qsb.external")

        # Ensure the .qsb directory exists
        file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/.qsb")

        add_custom_command(
            OUTPUT ${RGBA_EXTERNAL}
            COMMAND ${Python3_EXECUTABLE}
                ${QSB_WRAPPER}
                ${QSB_PROGRAM}
                ${RGBA_EXTERNAL}
                ${RGBA_GLES_SRC}
                ${RGBA_BASE_QSB}
            DEPENDS
                ${RGBA_BASE_QSB}
                ${RGBA_GLES_SRC}
                ${QSB_WRAPPER}
            COMMENT "Generating external GLES shader: ${RGBA_EXTERNAL}"
            VERBATIM
        )

        # Create a target for the external shader to ensure proper dependency ordering
        # The custom command depends on RGBA_BASE_QSB file, which is generated by gstqml6gl_shaders target
        add_custom_target(gstqml6gl_external_shader DEPENDS ${RGBA_EXTERNAL})
        if(gstqml6gl_shaders)
            add_dependencies(gstqml6gl_external_shader ${gstqml6gl_shaders})
        endif()

        set_source_files_properties("${RGBA_EXTERNAL}" PROPERTIES QT_RESOURCE_ALIAS RGBA.frag.qsb.external)
        qt_add_resources(gstqml6gl "gstqml6gl_shaders1"
            PREFIX "/org/freedesktop/gstreamer/qml6"
            FILES "${RGBA_EXTERNAL}"
        )

        # Ensure the resource target depends on the external shader being generated
        add_dependencies(gstqml6gl gstqml6gl_external_shader)
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
