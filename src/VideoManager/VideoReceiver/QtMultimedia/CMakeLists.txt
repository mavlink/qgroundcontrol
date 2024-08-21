find_package(Qt6 REQUIRED COMPONENTS Core)

qt_add_library(QtMultimediaReceiver STATIC)

option(QGC_ENABLE_QT_VIDEOSTREAMING "Enable QtMultimedia Video Streaming Backend" ON)
cmake_print_variables(QGC_ENABLE_QT_VIDEOSTREAMING)
if(NOT QGC_ENABLE_QT_VIDEOSTREAMING)
    return()
endif()

find_package(Qt6 REQUIRED COMPONENTS Multimedia MultimediaQuickPrivate Quick)

target_sources(QtMultimediaReceiver
    PRIVATE
        QtMultimediaReceiver.cc
        QtMultimediaReceiver.h
)

target_link_libraries(QtMultimediaReceiver
    PRIVATE
        Qt6::MultimediaQuickPrivate
        Qt6::Quick
        QGC
        Utilities
    PUBLIC
        Qt6::Core
        Qt6::Multimedia
        VideoReceiver
)

target_include_directories(QtMultimediaReceiver PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_definitions(QtMultimediaReceiver PUBLIC QGC_QT_STREAMING)

option(QGC_ENABLE_UVC "Disable UVC Devices" ON)
if(NOT QGC_ENABLE_UVC)
    target_compile_definitions(QtMultimediaReceiver PUBLIC QGC_DISABLE_UVC)
    return()
endif()

target_sources(QtMultimediaReceiver
    PRIVATE
        UVCReceiver.cc
        UVCReceiver.h
)
