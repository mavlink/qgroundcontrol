# ----------------------------------------------------------------------------
# RapidCheck (Property-Based Testing Framework)
# ----------------------------------------------------------------------------
# Save and remove coverage flags to avoid gcov symbol issues
set(_RC_SAVED_CXX_FLAGS "${CMAKE_CXX_FLAGS_DEBUG}")
set(_RC_SAVED_C_FLAGS "${CMAKE_C_FLAGS_DEBUG}")
string(REPLACE "--coverage" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
string(REPLACE "-fprofile-arcs" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
string(REPLACE "-ftest-coverage" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
string(REPLACE "--coverage" "" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
string(REPLACE "-fprofile-arcs" "" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
string(REPLACE "-ftest-coverage" "" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")

CPMAddPackage(
    NAME rapidcheck
    GITHUB_REPOSITORY emil-e/rapidcheck
    GIT_TAG 0aaf5a26d1d4d67421da72dee1b5e81aee41efde
    OPTIONS
        "RC_ENABLE_GTEST OFF"
        "RC_ENABLE_DOCTEST OFF"
        "RC_ENABLE_TESTS OFF"
        "RC_ENABLE_EXAMPLES OFF"
)

# Restore coverage flags
set(CMAKE_CXX_FLAGS_DEBUG "${_RC_SAVED_CXX_FLAGS}")
set(CMAKE_C_FLAGS_DEBUG "${_RC_SAVED_C_FLAGS}")

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE rapidcheck)

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/PropertyTesting.h
)

target_include_directories(${CMAKE_PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
