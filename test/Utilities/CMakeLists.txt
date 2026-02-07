# ============================================================================
# Utilities Unit Tests
# Tests for various utility modules
# ============================================================================

add_subdirectory(Audio)
add_subdirectory(Compression)
add_subdirectory(FileSystem)
add_subdirectory(Geo)
add_subdirectory(Logging)
add_subdirectory(Network)
add_subdirectory(Parsing)
add_subdirectory(Platform)

# ----------------------------------------------------------------------------
# Test Data Resources
# ----------------------------------------------------------------------------
qt_add_resources(${CMAKE_PROJECT_NAME} "UtilitiesTest_res"
    PREFIX "/unittest"
    FILES
        arducopter.apj
)
