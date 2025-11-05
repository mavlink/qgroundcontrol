# ============================================================================
# Fact System Unit Tests
# Tests for parameter management and fact metadata
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        FactSystemTestBase.cc
        FactSystemTestBase.h
        FactSystemTestGeneric.cc
        FactSystemTestGeneric.h
        FactSystemTestPX4.cc
        FactSystemTestPX4.h
        ParameterManagerTest.cc
        ParameterManagerTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# ----------------------------------------------------------------------------
# Test Data Resources
# ----------------------------------------------------------------------------
qt_add_resources(${CMAKE_PROJECT_NAME} "FactSystemTest_res"
    PREFIX "/unittest"
    FILES
        FactSystemTest.qml
)

# TODO: Enable QML module for fact system tests
# qt_add_library(FactSystemTestModule STATIC)
#
# qt_add_qml_module(FactSystemTestModule
#     URI QGroundControl.Test.FactSystem
#     VERSION 1.0
#     RESOURCE_PREFIX /unittest
#     QML_FILES
#         FactSystemTest.qml
#     NO_PLUGIN
# )
#
# target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE FactSystemTestModule)
