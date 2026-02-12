# ============================================================================
# JSON Parsing Utility Unit Tests
# Tests for JsonParsing validation and file parsing helpers
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        JsonParsingTest.cc
        JsonParsingTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
