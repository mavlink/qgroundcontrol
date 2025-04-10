set(QGC_SOURCES
    ${QGC_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/ComponentInformationCacheTest.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/ComponentInformationCacheTest.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ComponentInformationTranslationTest.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/ComponentInformationTranslationTest.h
    CACHE INTERNAL "" FORCE
)

set(QGC_DIRECTORIES
    ${QGC_DIRECTORIES}
    ${CMAKE_CURRENT_SOURCE_DIR}
    CACHE INTERNAL "" FORCE
)

# set(QGC_TEST_RESOURCES
#     ${QGC_TEST_RESOURCES}
#     TranslationTest.json
#     TranslationTest_de_DE.ts
#     CACHE INTERNAL "" FORCE
# )

# set_source_files_properties(TranslationTest.json PROPERTIES QT_RESOURCE_ALIAS TranslationTest.json)
# set_source_files_properties(TranslationTest_de_DE.ts PROPERTIES QT_RESOURCE_ALIAS TranslationTest_de_DE.ts)
