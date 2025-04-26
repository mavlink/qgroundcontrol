find_package(GStreamer REQUIRED COMPONENTS D3d11)

qt_add_library(gstqt6d3d11 STATIC
    gstqml6d3d11sink.cpp
    gstqml6d3d11sink.h
    gstqml6d3d11node.cpp
    gstqml6d3d11node.h
    gstqml6d3d11videoitem.cpp
    gstqml6d3d11videoitem.h
)

target_link_libraries(gstqt6d3d11 PUBLIC GStreamer::GStreamer)

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
