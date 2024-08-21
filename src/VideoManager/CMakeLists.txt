find_package(Qt6 REQUIRED COMPONENTS Core)

qt_add_library(VideoManager STATIC
    SubtitleWriter.cc
    SubtitleWriter.h
    VideoManager.cc
    VideoManager.h
)

# option(QGC_ENABLE_VIDEOSTREAMING "Enable video streaming" ON)
# cmake_print_variables(QGC_ENABLE_VIDEOSTREAMING)
# if(NOT QGC_ENABLE_VIDEOSTREAMING)
#     message(STATUS "Video streaming support disabled")
#     return()
# endif()
# message(STATUS "Video streaming support enabled")

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
        QGC
)

target_include_directories(VideoManager PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
