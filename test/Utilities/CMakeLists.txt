# ============================================================================
# Utilities Unit Tests
# Tests for various utility modules
# ============================================================================

add_subdirectory(Audio)
add_subdirectory(Compression)
add_subdirectory(FileSystem)
add_subdirectory(Geo)
add_subdirectory(Network)
add_subdirectory(Parsing)
add_subdirectory(Platform)
add_subdirectory(StateMachine)

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        QGCMathTest.cc
        QGCMathTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# ----------------------------------------------------------------------------
# Test Data Resources
# ----------------------------------------------------------------------------
qt_add_resources(${CMAKE_PROJECT_NAME} "UtilitiesTest_res"
    PREFIX "/unittest"
    FILES
        arducopter.apj
)
