find_package(Qt6 REQUIRED COMPONENTS Core QmlIntegration)

qt_add_library(VideoManager STATIC
    SubtitleWriter.cc
    SubtitleWriter.h
    VideoManager.cc
    VideoManager.h
)

add_subdirectory(VideoReceiver)

target_link_libraries(VideoManager
    PRIVATE
        API
        Camera
        FactSystem
        GStreamerReceiver
        QmlControls
        QtMultimediaReceiver
        Settings
        Utilities
        Vehicle
        VideoReceiver
    PUBLIC
        Qt6::Core
        Qt6::QmlIntegration
)

target_include_directories(VideoManager PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
