set(QGC_SOURCES
    ${QGC_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/VideoReceiver.h
    CACHE INTERNAL "" FORCE
)

set(QGC_DIRECTORIES
    ${QGC_DIRECTORIES}
    ${CMAKE_CURRENT_SOURCE_DIR}
    CACHE INTERNAL "" FORCE
)

add_subdirectory(GStreamer)
add_subdirectory(QtMultimedia)
