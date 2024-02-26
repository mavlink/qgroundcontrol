qt_add_library(VideoManager
    GLVideoItemStub.cc
    GLVideoItemStub.h
    SubtitleWriter.cc
    SubtitleWriter.h
    VideoManager.cc
    VideoManager.h
)

target_link_libraries(VideoManager
    PUBLIC
        qgc
        Qt6::Multimedia
        Qt6::OpenGL
        VideoReceiver
)

target_include_directories(VideoManager INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
