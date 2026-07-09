# ============================================================================
# JSON Parsing Utility Unit Tests
# Tests for JsonParsing validation and file parsing helpers
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        JsonHelperTest.cc
        JsonHelperTest.h
        JsonParsingTest.cc
        JsonParsingTest.h
        JsonResourceAuditTest.cc
        JsonResourceAuditTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_qgc_test(JsonHelperTest LABELS Unit Utilities)
add_qgc_test(JsonParsingTest LABELS Unit Utilities)
add_qgc_test(JsonResourceAuditTest LABELS Unit Utilities)
