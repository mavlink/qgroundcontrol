# ============================================================================
# Fact System Unit Tests
# Tests for parameter management and fact metadata
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        FactGroupTest.cc
        FactGroupTest.h
        FactMetaDataTest.cc
        FactMetaDataTest.h
        PX4ParameterMetaDataTest.cc
        PX4ParameterMetaDataTest.h
        FactSystemTestBase.cc
        FactSystemTestBase.h
        FactSystemTestPX4.cc
        FactSystemTestPX4.h
        FactTest.cc
        FactTest.h
        FactValueSliderListModelTest.cc
        FactValueSliderListModelTest.h
        HashCheckTest.cc
        HashCheckTest.h
        ParameterEditorControllerTest.cc
        ParameterEditorControllerTest.h
        ParameterManagerTest.cc
        ParameterManagerTest.h
        ParameterMetaDataTestHelper.h
)

if(NOT QGC_DISABLE_APM_PLUGIN)
    target_sources(${CMAKE_PROJECT_NAME}
        PRIVATE
            APMParameterMetaDataTest.cc
            APMParameterMetaDataTest.h
    )
endif()

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_qgc_test(APMParameterMetaDataTest LABELS Unit RESOURCE_LOCK TempFiles)
add_qgc_test(FactGroupTest LABELS Unit)
add_qgc_test(FactMetaDataTest LABELS Unit)
add_qgc_test(PX4ParameterMetaDataTest LABELS Unit RESOURCE_LOCK TempFiles)
add_qgc_test(FactSystemTestPX4 LABELS Integration Vehicle)
add_qgc_test(FactTest LABELS Unit)
add_qgc_test(FactValueSliderListModelTest LABELS Unit)
add_qgc_test(HashCheckTest LABELS Integration Vehicle TIMEOUT ${QGC_TEST_TIMEOUT_EXTENDED})
add_qgc_test(ParameterEditorControllerTest LABELS Integration Vehicle)
add_qgc_test(ParameterManagerTest LABELS Integration Vehicle TIMEOUT ${QGC_TEST_TIMEOUT_EXTENDED} SERIAL)
