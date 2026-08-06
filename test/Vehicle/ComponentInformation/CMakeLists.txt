# ============================================================================
# Component Information Unit Tests
# Tests for vehicle component metadata caching and translation
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        ComponentInformationCacheTest.cc
        ComponentInformationCacheTest.h
        ComponentInformationManagerTest.cc
        ComponentInformationManagerTest.h
        ComponentInformationTranslationTest.cc
        ComponentInformationTranslationTest.h
        JsonSchemaValidatorTest.cc
        JsonSchemaValidatorTest.h
        RequestMetaDataTypeStateMachineTest.cc
        RequestMetaDataTypeStateMachineTest.h
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
        actuators.example.json
)

add_qgc_test(ComponentInformationCacheTest LABELS Integration Vehicle RESOURCE_LOCK TempFiles)
add_qgc_test(ComponentInformationTranslationTest LABELS Unit Vehicle)
add_qgc_test(JsonSchemaValidatorTest LABELS Unit Vehicle)
add_qgc_test(ComponentInformationManagerTest LABELS Integration Vehicle)
add_qgc_test(RequestMetaDataTypeStateMachineTest LABELS Integration Vehicle)
