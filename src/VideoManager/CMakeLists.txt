find_package(Qt6 REQUIRED COMPONENTS Core Multimedia)

# QGC_DISABLE_UVC

qt_add_library(VideoManager STATIC
    GLVideoItemStub.cc
    GLVideoItemStub.h
    SubtitleWriter.cc
    SubtitleWriter.h
    VideoManager.cc
    VideoManager.h
)

target_link_libraries(VideoManager
    PRIVATE
        Qt6::Multimedia
    PUBLIC
        qgc
        VideoReceiver
)

target_include_directories(VideoManager PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
