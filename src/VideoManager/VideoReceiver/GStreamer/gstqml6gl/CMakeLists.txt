find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

qt_add_library(gstqml6gl STATIC)

if(MACOS)
    # Using FindGStreamer.cmake is currently bypassed on MACOS since it doesn't work
    # So for now we hack in a simple hardwired setup which does work
    find_library(GSTREAMER_FRAMEWORK GStreamer)
    set(GST_PLUGINS_VERSION 1.24.12)
    set(GSTREAMER_FRAMEWORK_PATH "/Library/Frameworks/GStreamer.framework")
    target_link_libraries(gstqml6gl PUBLIC "$<LINK_LIBRARY:FRAMEWORK,${GSTREAMER_FRAMEWORK_PATH}>")
    target_include_directories(gstqml6gl PUBLIC "${GSTREAMER_FRAMEWORK_PATH}/Headers")
else()
    set(GST_COMPONENTS GlEgl GlPrototypes GlWayland GlX11)
    find_package(GStreamer REQUIRED COMPONENTS OPTIONAL_COMPONENTS ${GST_COMPONENTS})
    target_link_libraries(gstqml6gl PUBLIC GStreamer::GStreamer)
endif()

target_link_libraries(gstqml6gl
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::GuiPrivate
        Qt6::Qml
        Qt6::Quick
)

foreach(component IN LISTS GST_COMPONENTS)
    if(GStreamer_${component}_FOUND)
        target_link_libraries(gstqml6gl PUBLIC GStreamer::${component})
    endif()
endforeach()

################################################################################

if(LINUX)
    install(DIRECTORY ${GSTREAMER_LIB_PATH}/gstreamer1.0 DESTINATION ${CMAKE_INSTALL_LIBDIR})
    install(CODE "execute_process(COMMAND chmod +x \"${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/gstreamer1.0/gstreamer-1.0/gst-plugin-scanner\")")
    install(CODE "execute_process(COMMAND chmod +x \"${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/gstreamer1.0/gstreamer-1.0/gst-ptp-helper\")")
    install(DIRECTORY ${GSTREAMER_LIB_PATH}/gio DESTINATION ${CMAKE_INSTALL_LIBDIR})
    get_target_property(LINKED_PLUGINS GStreamer::Plugins INTERFACE_LINK_LIBRARIES)
    install(FILES ${LINKED_PLUGINS} DESTINATION ${CMAKE_INSTALL_LIBDIR}/gstreamer-1.0)
elseif(WIN32)
    cmake_path(CONVERT "${GSTREAMER_PREFIX}/bin/*.dll" TO_CMAKE_PATH_LIST GST_WIN_BINS_PATH)
    file(GLOB GST_WIN_BINS ${GST_WIN_BINS_PATH})
    install(FILES ${GST_WIN_BINS} DESTINATION ${CMAKE_INSTALL_BINDIR})

    cmake_path(CONVERT "${GSTREAMER_LIB_PATH}/gio/modules/*.dll" TO_CMAKE_PATH_LIST GST_GIO_MODULES_PATH)
    file(GLOB GST_GIO_MODULES ${GST_GIO_MODULES_PATH})
    install(FILES ${GST_GIO_MODULES} DESTINATION ${CMAKE_INSTALL_LIBDIR}/gio/modules)

    cmake_path(CONVERT "${GSTREAMER_PLUGIN_PATH}/*.dll" TO_CMAKE_PATH_LIST GST_WIN_PLUGINS_PATH)
    file(GLOB GST_WIN_PLUGINS ${GST_WIN_PLUGINS_PATH})
    install(FILES ${GST_WIN_PLUGINS} DESTINATION ${CMAKE_INSTALL_LIBDIR}/gstreamer-1.0)

    cmake_path(CONVERT "${GSTREAMER_PREFIX}/libexec/gstreamer-1.0/*.exe" TO_CMAKE_PATH_LIST GST_HELPER_BINS_PATH)
    file(GLOB GST_HELPER_BINS ${GST_HELPER_BINS_PATH})
    install(FILES ${GST_HELPER_BINS} DESTINATION ${CMAKE_INSTALL_LIBEXECDIR}/gstreamer-1.0)
elseif(MACOS)
    if(NOT EXISTS "${GSTREAMER_FRAMEWORK_PATH}")
        message(FATAL_ERROR "GStreamer.framework not found at ${GSTREAMER_FRAMEWORK_PATH}")
    endif()
    install(
        DIRECTORY "${GSTREAMER_FRAMEWORK_PATH}"
        DESTINATION "${CMAKE_INSTALL_PREFIX}/${CMAKE_PROJECT_NAME}.app/Contents/Frameworks"
        PATTERN "*.la" EXCLUDE
        PATTERN "*.a" EXCLUDE
        PATTERN "*/bin" EXCLUDE
        PATTERN "*/etc" EXCLUDE
        PATTERN "*/gst-validate-launcher" EXCLUDE
        PATTERN "*/Headers" EXCLUDE
        PATTERN "*/include" EXCLUDE
        PATTERN "*/pkgconfig" EXCLUDE
        PATTERN "*/share" EXCLUDE
    )
    target_compile_definitions(gstqml6gl INTERFACE QGC_GST_MACOS_FRAMEWORK)
endif()

################################################################################

# DYLD_LIBRARY_PATH

# <key>LSEnvironment</key>
# <dict>
#     <key>GST_REGISTRY_REUSE_PLUGIN_SCANNER</key>
#     <string>no</string>
#     <key>GST_PLUGIN_SCANNER</key>
#     <string>@executable_path/../Frameworks/GStreamer.framework/Versions/1.0/libexec/gstreamer-1.0/gst-plugin-scanner</string>
#     <key>GST_PTP_HELPER_1_0</key>
#     <string>@executable_path/../Frameworks/GStreamer.framework/Versions/1.0/libexec/gstreamer-1.0/gst-ptp-helper</string>
#     <key>GIO_EXTRA_MODULES</key>
#     <string>@executable_path/../Frameworks/GStreamer.framework/Versions/1.0/lib/gio/modules</string>
#     <key>GST_PLUGIN_SYSTEM_PATH_1_0</key>
#     <string>@executable_path/../Frameworks/GStreamer.framework/Versions/1.0/lib/gstreamer-1.0</string>
#     <key>GST_PLUGIN_SYSTEM_PATH</key>
#     <string>@executable_path/../Frameworks/GStreamer.framework/Versions/1.0/lib/gstreamer-1.0</string>
#     <key>GST_PLUGIN_PATH_1_0</key>
#     <string>@executable_path/../Frameworks/GStreamer.framework/Versions/1.0/lib/gstreamer-1.0</string>
#     <key>GST_PLUGIN_PATH</key>
#     <string>@executable_path/../Frameworks/GStreamer.framework/Versions/1.0/lib/gstreamer-1.0</string>
# </dict>

################################################################################

# Don't Download & Build if gstreamer1.0-qt6 was found
if(GST_PLUGIN_gstqml6_FOUND)
    return()
endif()

################################################################################

if(GStreamer_VERSION VERSION_GREATER_EQUAL 1.22)
    include(CPM)
    CPMAddPackage(
        NAME gstreamer_good_plugins
        VERSION ${GST_PLUGINS_VERSION}
        # URL https://gitlab.freedesktop.org/gstreamer/gstreamer/-/archive/${GST_PLUGINS_VERSION}/gstreamer-${GST_PLUGINS_VERSION}.zip?path=subprojects/gst-plugins-good/ext/qt6
        URL https://gstreamer.freedesktop.org/src/gst-plugins-good/gst-plugins-good-${GST_PLUGINS_VERSION}.tar.xz
    )
    set(QGC_GST_QT6_PLUGIN_PATH "${gstreamer_good_plugins_SOURCE_DIR}/ext/qt6")
else()
    set(QGC_GST_QT6_PLUGIN_PATH "${CMAKE_CURRENT_SOURCE_DIR}/qt6")
endif()
cmake_print_variables(QGC_GST_QT6_PLUGIN_PATH)

################################################################################

file(READ ${QGC_GST_QT6_PLUGIN_PATH}/qt6glitem.h FILE_CONTENTS)
string(FIND "${FILE_CONTENTS}" "#include <QtQuick/QQuickWindow>" GST_FIX_INCLUDES)
if(GST_FIX_INCLUDES EQUAL -1)
    string(REPLACE "#include <QtGui/QOpenGLFunctions>" "#include <QtGui/QOpenGLFunctions>\n#include <QtQuick/QQuickWindow>" FILE_CONTENTS "${FILE_CONTENTS}")
    file(WRITE ${QGC_GST_QT6_PLUGIN_PATH}/qt6glitem.h "${FILE_CONTENTS}")
endif()

################################################################################

file(GLOB gstqml6gl_SRCS
    ${QGC_GST_QT6_PLUGIN_PATH}/*.cc
    ${QGC_GST_QT6_PLUGIN_PATH}/*.h
)
target_sources(gstqml6gl PRIVATE ${gstqml6gl_SRCS})

target_include_directories(gstqml6gl PUBLIC ${QGC_GST_QT6_PLUGIN_PATH})

################################################################################

if(GStreamer_GlX11_FOUND)
    target_compile_definitions(gstqml6gl PRIVATE HAVE_QT_X11)
endif()

if(GStreamer_GlEGL_FOUND)
    target_compile_definitions(gstqml6gl PRIVATE HAVE_QT_EGLFS)
endif()

if(GStreamer_GlWayland_FOUND)
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

################################################################################

if(EXISTS "${QGC_GST_QT6_PLUGIN_PATH}/resources.qrc")
    find_package(Qt6 REQUIRED COMPONENTS ShaderTools)
    if(Qt6ShaderTools_FOUND)
        set(SHADERS
            ${QGC_GST_QT6_PLUGIN_PATH}/vertex.vert
            ${QGC_GST_QT6_PLUGIN_PATH}/YUV_TRIPLANAR.frag
            ${QGC_GST_QT6_PLUGIN_PATH}/RGBA.frag
        )
        set(OUTPUTS
            vertex.vert.qsb
            YUV_TRIPLANAR.frag.qsb
            RGBA.frag.qsb
        )
        if(EXISTS "${QGC_GST_QT6_PLUGIN_PATH}/YUV_BIPLANAR.frag")
            LIST(APPEND SHADERS ${QGC_GST_QT6_PLUGIN_PATH}/YUV_BIPLANAR.frag)
            LIST(APPEND OUTPUTS YUV_BIPLANAR.frag.qsb)
        endif()
        qt6_add_shaders(gstqml6gl "gstqml6gl_shaders"
            PREFIX "/org/freedesktop/gstreamer/qml6"
            GLSL "100 es,120,330"
            OUTPUT_TARGETS gstqml6gl_shaders
            FILES ${SHADERS}
            OUTPUTS ${OUTPUTS}
            BATCHABLE
        )

        if(EXISTS "${QGC_GST_QT6_PLUGIN_PATH}/RGBA_gles.frag")
            file(COPY_FILE "${CMAKE_CURRENT_BINARY_DIR}/.qsb/RGBA.frag.qsb" "${CMAKE_CURRENT_BINARY_DIR}/.qsb/RGBA.frag.qsb.external")
            qt6_add_shaders(gstqml6gl "gstqml6gl_shaders1"
                PREFIX "/org/freedesktop/gstreamer/qml6"
                OUTPUT_TARGETS gstqml6gl_shaders1
                FILES "${CMAKE_CURRENT_BINARY_DIR}/.qsb/RGBA.frag.qsb.external@glsl,100es,${QGC_GST_QT6_PLUGIN_PATH}/RGBA_gles.frag"
                OUTPUTS "RGBA.frag.qsb.external"
            )
        endif()
    endif()
endif()

################################################################################

if(UNIX)
    target_compile_options(gstqml6gl
        PRIVATE
            -Wno-unused-parameter
            -Wno-implicit-fallthrough
            -Wno-unused-private-field
    )
endif()

################################################################################
