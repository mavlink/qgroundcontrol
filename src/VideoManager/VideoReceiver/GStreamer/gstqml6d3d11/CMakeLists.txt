qt_add_library(gstqt6d3d11 STATIC
    plugin.cpp
    gstqml6d3d11sink.cpp
    gstqml6d3d11sink.h
    gstqsg6d3d11node.cpp
    gstqsg6d3d11node.h
    gstqt6d3d11videoitem.cpp
    gstqt6d3d11videoitem.h
)

target_link_libraries(gstqt6d3d11
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
    PUBLIC
        GStreamer::GStreamer
)

target_include_directories(gstqt6d3d11 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
