# ============================================================================
# Component Information Unit Tests
# Tests for vehicle component metadata caching and translation
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        ComponentInformationCacheTest.cc
        ComponentInformationCacheTest.h
        ComponentInformationTranslationTest.cc
        ComponentInformationTranslationTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# ----------------------------------------------------------------------------
# Test Data Resources
# ----------------------------------------------------------------------------
qt_add_resources(${CMAKE_PROJECT_NAME} "VehicleTest_ComponentInformation_res"
    PREFIX "/unittest"
    FILES
        TranslationTest.json
        TranslationTest_de_DE.ts
)
