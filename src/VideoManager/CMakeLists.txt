find_package(Qt6 REQUIRED COMPONENTS Core Quick)

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
        api
        Camera
        FactSystem
        QmlControls
        Settings
        Utilities
        Vehicle
    PUBLIC
        Qt6::Core
        Qt6::Quick
        qgc
        VideoReceiver
)

option(QGC_DISABLE_UVC "Disable UVC Devices" OFF)
if(QGC_DISABLE_UVC)
    target_compile_options(VideoManager PUBLIC QGC_DISABLE_UVC)
else()
    find_package(Qt6 REQUIRED COMPONENTS Multimedia)
    target_link_libraries(VideoManager PRIVATE Qt6::Multimedia)
endif()

target_include_directories(VideoManager PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
