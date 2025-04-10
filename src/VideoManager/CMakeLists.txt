set(QGC_SOURCES
    ${QGC_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/SubtitleWriter.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/SubtitleWriter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/VideoManager.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/VideoManager.h
    CACHE INTERNAL "" FORCE
)

set(QGC_DIRECTORIES
    ${QGC_DIRECTORIES}
    ${CMAKE_CURRENT_SOURCE_DIR}
    CACHE INTERNAL "" FORCE
)

add_subdirectory(VideoReceiver)
