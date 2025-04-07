if(NOT QGC_ENABLE_QT_VIDEOSTREAMING)
    return()
endif()

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        QtMultimediaReceiver.cc
        QtMultimediaReceiver.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE QGC_QT_STREAMING)

if(NOT QGC_ENABLE_UVC)
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE QGC_DISABLE_UVC)
    return()
endif()

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        UVCReceiver.cc
        UVCReceiver.h
)
