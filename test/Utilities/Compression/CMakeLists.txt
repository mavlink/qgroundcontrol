set(QGC_SOURCES
    ${QGC_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/DecompressionTest.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/DecompressionTest.h
    CACHE INTERNAL "" FORCE
)

set(QGC_DIRECTORIES
    ${QGC_DIRECTORIES}
    ${CMAKE_CURRENT_SOURCE_DIR}
    CACHE INTERNAL "" FORCE
)

qt_add_resources(${CMAKE_PROJECT_NAME} "UtilitiesTest_CompressionTest_res"
    PREFIX "/"
    FILES
        manifest.json.gz
        manifest.json.xz
        manifest.json.zip
)
