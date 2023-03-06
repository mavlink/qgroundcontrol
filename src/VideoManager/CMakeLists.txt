add_library(VideoManager
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
        Qt5::OpenGL
        VideoReceiver
)

if (NOT QGC_DISABLE_UVC)
    target_link_libraries(VideoManager
            PUBLIC
            Qt5::Multimedia
            )
endif ()

target_include_directories(VideoManager INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

if (QGC_DISABLE_UVC)
#    add_compile_definitions(QGC_DISABLE_UVC)
    message(STATUS "VideoManager: UVC disabled")

endif ()