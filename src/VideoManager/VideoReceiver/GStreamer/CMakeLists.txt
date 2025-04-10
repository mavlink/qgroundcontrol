set(QGC_DIRECTORIES
    ${QGC_DIRECTORIES}
    ${CMAKE_CURRENT_SOURCE_DIR}
    CACHE INTERNAL "" FORCE
)

if(NOT QGC_ENABLE_GST_VIDEOSTREAMING)
    set(QGC_SOURCES
        ${QGC_SOURCES}
        ${CMAKE_CURRENT_SOURCE_DIR}/GLVideoItemStub.h
        CACHE INTERNAL "" FORCE
    )
    return()
endif()

set(QGC_SOURCES
    ${QGC_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/gstqgc.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/gstqgcelement.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/gstqgcelements.h
    ${CMAKE_CURRENT_SOURCE_DIR}/gstqgcvideosinkbin.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/gstqgcvideosinkbin.h
    ${CMAKE_CURRENT_SOURCE_DIR}/GStreamer.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/GStreamer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/GstVideoReceiver.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/GstVideoReceiver.h
    CACHE INTERNAL "" FORCE
)

if(IOS)
    set(QGC_SOURCES
        ${QGC_SOURCES}
        ${CMAKE_CURRENT_SOURCE_DIR}/gst_ios_init.m
        ${CMAKE_CURRENT_SOURCE_DIR}/gst_ios_init.h
        CACHE INTERNAL "" FORCE
    )
endif()

add_subdirectory(gstqml6gl)
set(QGC_LIBRARIES
    ${QGC_LIBRARIES}
    Qt6::OpenGL
    gstqml6gl
    CACHE INTERNAL "" FORCE
)

set(QGC_DEFINES
    ${QGC_DEFINES}
    QGC_GST_STREAMING
    CACHE INTERNAL "" FORCE
)
