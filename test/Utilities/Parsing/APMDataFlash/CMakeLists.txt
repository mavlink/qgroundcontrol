# ============================================================================
# DataFlash Utility Unit Tests
# Tests for DataFlashUtility (general-purpose DataFlash parsing)
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        APMDataFlashUtilityTest.cc
        APMDataFlashUtilityTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_qgc_test(APMDataFlashUtilityTest LABELS Unit Utilities)
