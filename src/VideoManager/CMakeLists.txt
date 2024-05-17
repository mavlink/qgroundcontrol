add_subdirectory(VideoReceiver)

find_package(Qt6 REQUIRED COMPONENTS Core)

qt_add_library(VideoManager STATIC
    SubtitleWriter.cc
    SubtitleWriter.h
    VideoManager.cc
    VideoManager.h
)

target_link_libraries(VideoManager
    PRIVATE
        API
        Camera
        FactSystem
        QmlControls
        QtMultimediaReceiver
        Settings
        Utilities
        Vehicle
    PUBLIC
        Qt6::Core
        GStreamerReceiver
        QGC
)

target_include_directories(VideoManager PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
