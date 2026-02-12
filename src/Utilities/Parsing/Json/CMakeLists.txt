# ============================================================================
# JSON Parsing Utilities
# JSON validation and parsing helpers shared across subsystems
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        JsonParsing.cc
        JsonParsing.h
)

target_include_directories(${CMAKE_PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
