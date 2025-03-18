find_package(Qt6 REQUIRED COMPONENTS Core)

if(NOT QGC_ENABLE_QT_VIDEOSTREAMING)
    return()
endif()

find_package(Qt6 REQUIRED COMPONENTS Multimedia MultimediaQuickPrivate Quick)

target_sources(QGC
    PRIVATE
        QtMultimediaReceiver.cc
        QtMultimediaReceiver.h
)

target_link_libraries(QGC
    PRIVATE
        Qt6::MultimediaQuickPrivate
        Qt6::Quick
    PUBLIC
        Qt6::Core
        Qt6::Multimedia
)

target_include_directories(QGC PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_definitions(QGC PUBLIC QGC_QT_STREAMING)

if(NOT QGC_ENABLE_UVC)
    target_compile_definitions(QGC PUBLIC QGC_DISABLE_UVC)
    return()
endif()

target_sources(QGC
    PRIVATE
        UVCReceiver.cc
        UVCReceiver.h
)
