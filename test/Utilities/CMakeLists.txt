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

# set(QGC_TEST_RESOURCES
#     ${QGC_TEST_RESOURCES}
#     arducopter.apj
#     CACHE INTERNAL "" FORCE
# )

# set_source_files_properties(arducopter.apj PROPERTIES QT_RESOURCE_ALIAS arducopter.apj)

add_subdirectory(Audio)
add_subdirectory(Compression)
add_subdirectory(Geo)
