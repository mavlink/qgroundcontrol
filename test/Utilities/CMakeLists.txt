# ============================================================================
# Utilities Unit Tests
# Tests for various utility modules
# ============================================================================

add_subdirectory(Audio)
add_subdirectory(Compression)
add_subdirectory(FileSystem)
add_subdirectory(Geo)
add_subdirectory(Network)

# ----------------------------------------------------------------------------
# Test Data Resources
# ----------------------------------------------------------------------------
qt_add_resources(${CMAKE_PROJECT_NAME} "UtilitiesTest_res"
    PREFIX "/unittest"
    FILES
        arducopter.apj
)
