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

qt_add_resources(${CMAKE_PROJECT_NAME}
    "VehicleTest_ComponentInformation_res"
    FILES
        TranslationTest.json
        TranslationTest_de_DE.ts
)
