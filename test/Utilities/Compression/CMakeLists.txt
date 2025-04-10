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

# set(QGC_TEST_RESOURCES
#     ${QGC_TEST_RESOURCES}
#     manifest.json.gz
#     manifest.json.xz
#     manifest.json.zip
#     CACHE INTERNAL "" FORCE
# )

# set_source_files_properties(manifest.json.gz PROPERTIES QT_RESOURCE_ALIAS manifest.json.gz)
# set_source_files_properties(manifest.json.xz PROPERTIES QT_RESOURCE_ALIAS manifest.json.xz)
# set_source_files_properties(manifest.json.zip PROPERTIES QT_RESOURCE_ALIAS manifest.json.zip)
