find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)
find_package(GStreamer REQUIRED COMPONENTS Allocators Audio Codecparsers Controller Fft Mpegts Net Pbutils Riff Rtp Rtsp Sdp Tag OPTIONAL_COMPONENTS Photography GlPrototypes Va GlX11 GlEGL GlWayland)

qt_add_library(gstqml6gl STATIC)

target_link_libraries(gstqml6gl
    PRIVATE
        Qt6::GuiPrivate
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
        GStreamer::GStreamer
        GStreamer::Allocators
        GStreamer::Audio
        GStreamer::Codecparsers
        GStreamer::Controller
        GStreamer::Fft
        GStreamer::Mpegts
        GStreamer::Net
        GStreamer::Pbutils
        GStreamer::Riff
        GStreamer::Rtp
        GStreamer::Rtsp
        GStreamer::Sdp
        GStreamer::Tag
)

# Photography not found on ubuntu 20.04?
if(GStreamer_Photography_FOUND)
    target_link_libraries(gstqml6gl PUBLIC GStreamer::Photography)
endif()

if(GStreamer_GlPrototypes_FOUND)
    target_link_libraries(gstqml6gl PUBLIC GStreamer::GlPrototypes)
endif()

if(GStreamer_Va_FOUND)
    target_link_libraries(gstqml6gl PUBLIC GStreamer::Va)
endif()

################################################################################

# TODO: Don't Download & Build if gstreamer1.0-qt6 was found
if(GStreamer_VERSION VERSION_GREATER_EQUAL 1.22)
    FetchContent_Declare(gstreamer_good_plugins
        # URL https://gitlab.freedesktop.org/gstreamer/gstreamer/-/archive/${GST_PLUGINS_VERSION}/gstreamer-${GST_PLUGINS_VERSION}.zip?path=subprojects/gst-plugins-good/ext/qt6
        URL https://gstreamer.freedesktop.org/src/gst-plugins-good/gst-plugins-good-${GST_PLUGINS_VERSION}.tar.xz
        DOWNLOAD_EXTRACT_TIMESTAMP true
    )
    FetchContent_MakeAvailable(gstreamer_good_plugins)
    set(QGC_GST_QT6_PLUGIN_PATH "${gstreamer_good_plugins_SOURCE_DIR}/ext/qt6" CACHE PATH "Location of downloaded GST Qt6 plugin sources")
else()
    set(QGC_GST_QT6_PLUGIN_PATH "${CMAKE_CURRENT_SOURCE_DIR}/qt6" CACHE PATH "Location of custom local GST plugin")
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
    target_link_libraries(gstqml6gl PUBLIC GStreamer::GlX11)
    target_compile_definitions(gstqml6gl PRIVATE HAVE_QT_X11)
endif()

if(GStreamer_GlEGL_FOUND)
    target_link_libraries(gstqml6gl PUBLIC GStreamer::GlEGL)
    target_compile_definitions(gstqml6gl PRIVATE HAVE_QT_EGLFS)
endif()

if(GStreamer_GlWayland_FOUND)
    target_link_libraries(gstqml6gl PUBLIC GStreamer::GlWayland)
    target_compile_definitions(gstqml6gl PRIVATE HAVE_QT_WAYLAND)
endif()

if(ANDROID)
    target_compile_definitions(gstqml6gl PRIVATE HAVE_QT_ANDROID)
elseif(WIN32)
    target_compile_definitions(gstqml6gl PRIVATE HAVE_QT_WIN32)
elseif(MACOS)
    # appleframeworks
    # Foundation, QuartzCore, CoreFoundation
    # Cocoa
    target_compile_definitions(gstqml6gl PRIVATE HAVE_QT_MAC)
elseif(IOS)
    # appleframeworks
    # Foundation, CoreFoundation, CoreGraphics, QuartzCore, UIkit
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

if(LINUX)
    install(DIRECTORY ${GSTREAMER_LIB_PATH}/gstreamer1.0 DESTINATION ${CMAKE_INSTALL_LIBDIR})
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
    # cmake_path(CONVERT "${GSTREAMER_PREFIX}/bin/*.dylib" TO_CMAKE_PATH_LIST GST_MACOS_BINS_PATH)
    # file(GLOB GST_MACOS_BINS ${GST_MACOS_BINS_PATH})
    # install(FILES ${GST_MACOS_BINS} DESTINATION ${CMAKE_INSTALL_BINDIR})

    # cmake_path(CONVERT "${GSTREAMER_LIB_PATH}/gio/modules/*.dylib" TO_CMAKE_PATH_LIST GST_GIO_MODULES_PATH)
    # file(GLOB GST_GIO_MODULES ${GST_GIO_MODULES_PATH})
    # install(FILES ${GST_GIO_MODULES} DESTINATION ${CMAKE_INSTALL_LIBDIR}/gio/modules)

    # cmake_path(CONVERT "${GSTREAMER_PLUGIN_PATH}/*.dylib" TO_CMAKE_PATH_LIST GST_MACOS_PLUGINS_PATH)
    # file(GLOB GST_MACOS_PLUGINS ${GST_MACOS_PLUGINS_PATH})
    # install(FILES ${GST_MACOS_PLUGINS} DESTINATION ${CMAKE_INSTALL_LIBDIR}/gstreamer-1.0)

    # cmake_path(CONVERT "${GSTREAMER_PREFIX}/libexec/gstreamer-1.0/*" TO_CMAKE_PATH_LIST GST_HELPER_BINS_PATH)
    # file(GLOB GST_HELPER_BINS ${GST_HELPER_BINS_PATH})
    # install(FILES ${GST_HELPER_BINS} DESTINATION ${CMAKE_INSTALL_LIBEXECDIR}/gstreamer-1.0)
elseif(ANDROID)
    if(CMAKE_HOST_WIN32)
        cmake_path(CONVERT "${GSTREAMER_PREFIX}/share/gst-android/ndk-build/tools/windows/*.dll" TO_CMAKE_PATH_LIST GST_WIN_TOOLS_PATH)
        file(GLOB GST_WIN_TOOLS ${GST_WIN_TOOLS_PATH})
        install(FILES ${GST_WIN_TOOLS} DESTINATION ${CMAKE_INSTALL_BINDIR})
    endif()
endif()

################################################################################

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
