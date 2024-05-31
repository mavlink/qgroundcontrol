if(NOT GST_QT6_PLUGIN_PATH)
    set(GST_QT6_PLUGIN_PATH ${CMAKE_SOURCE_DIR}/libs/qmlglsink/qt6)
endif()
cmake_print_variables(GST_QT6_PLUGIN_PATH)
if(EXISTS ${GST_QT6_PLUGIN_PATH})
    set(GST_QT6_PLUGIN_FOUND TRUE CACHE INTERNAL "")
    target_sources(qmlglsink
        PRIVATE
            ${GST_QT6_PLUGIN_PATH}/gstplugin.cc
            ${GST_QT6_PLUGIN_PATH}/gstqml6glsink.cc
            ${GST_QT6_PLUGIN_PATH}/gstqml6glsink.h
            ${GST_QT6_PLUGIN_PATH}/gstqsg6glnode.cc
            ${GST_QT6_PLUGIN_PATH}/gstqsg6glnode.h
            ${GST_QT6_PLUGIN_PATH}/gstqt6element.cc
            ${GST_QT6_PLUGIN_PATH}/gstqt6elements.h
            ${GST_QT6_PLUGIN_PATH}/gstqt6gl.h
            ${GST_QT6_PLUGIN_PATH}/gstqt6glutility.cc
            ${GST_QT6_PLUGIN_PATH}/gstqt6glutility.h
            ${GST_QT6_PLUGIN_PATH}/qt6glitem.cc
            ${GST_QT6_PLUGIN_PATH}/qt6glitem.h
    )

    find_package(Qt6 REQUIRED COMPONENTS Core Gui OpenGL Qml Quick)
    target_link_libraries(qmlglsink
        PUBLIC
            Qt6::Core
            Qt6::Gui
            Qt6::GuiPrivate
            Qt6::OpenGL
            Qt6::Qml
            Qt6::Quick
    )
    if(WIN32)
        find_package(OpenGL)
        target_link_libraries(qmlglsink PUBLIC OpenGL::GL)
    elseif(LINUX)
        # find_package(Qt6 COMPONENTS WaylandClient)
        if(Qt6WaylandClient_FOUND)
            target_link_libraries(qmlglsink PRIVATE Qt6::WaylandClient)
        endif()
    endif()

    target_include_directories(qmlglsink PUBLIC qt6)

    target_compile_definitions(qmlglsink
        PRIVATE
            HAVE_QT_QPA_HEADER
            QT_QPA_HEADER=<QtGui/qpa/qplatformnativeinterface.h>
    )
    if(LINUX)
        target_compile_definitions(qmlglsink PRIVATE HAVE_QT_X11)
        if(EGL_FOUND)
            target_compile_definitions(qmlglsink PRIVATE HAVE_QT_EGLFS)
        endif()
        if(Qt6WaylandClient_FOUND)
            target_compile_definitions(qmlglsink PRIVATE HAVE_QT_WAYLAND)
        endif()
    elseif(ANDROID)
        target_compile_definitions(qmlglsink PRIVATE HAVE_QT_ANDROID)
    elseif(WIN32)
        target_compile_definitions(qmlglsink PRIVATE HAVE_QT_WIN32)
    elseif(MACOS)
        target_compile_definitions(qmlglsink PRIVATE HAVE_QT_MAC)
    elseif(IOS)
        target_compile_definitions(qmlglsink PRIVATE HAVE_QT_IOS)
        message(WARNING "qmlglsink not supported for IOS")
    endif()

    if(UNIX)
        target_compile_options(qmlglsink
            PRIVATE
                -Wno-unused-parameter
                -Wno-implicit-fallthrough
                -Wno-unused-private-field
        )
    endif()
else()
    message(WARNING "GST Qt Plugin Not Found")
endif()
