set(QGC_SOURCES
    ${QGC_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/FileSystem/QGCFileDownloadTest.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/FileSystem/QGCFileDownloadTest.h
    CACHE INTERNAL "" FORCE
)

set(QGC_DIRECTORIES
    ${QGC_DIRECTORIES}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/FileSystem
    CACHE INTERNAL "" FORCE
)

qt_add_resources(${CMAKE_PROJECT_NAME} "UtilitiesTest_res"
    PREFIX "/"
    FILES
        arducopter.apj
)

add_subdirectory(Audio)
add_subdirectory(Compression)
add_subdirectory(Geo)
