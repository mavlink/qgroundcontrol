# Custom plugin unit tests. The .cc/.h pairs are appended to the CUSTOM_SOURCES cache that
# src/CMakeLists.txt hands to the application target, so AUTOMOC and the test-framework include
# directories the application already carries under QGC_BUILD_TESTING apply with no extra wiring.
# Registration with CTest is done directly: this directory is processed before the application
# target exists, so add_qgc_test()'s check-target dependencies are not available yet, and
# add_test() resolves the binary through a generator expression at generate time.

if(NOT QGC_BUILD_TESTING)
    return()
endif()

list(APPEND CUSTOM_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/CustomPluginTest.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/CustomPluginTest.h
)
set(CUSTOM_SOURCES ${CUSTOM_SOURCES} CACHE INTERNAL "" FORCE)

list(APPEND CUSTOM_INCLUDE_DIRECTORIES
    ${CMAKE_CURRENT_SOURCE_DIR}
)
set(CUSTOM_INCLUDE_DIRECTORIES ${CUSTOM_INCLUDE_DIRECTORIES} CACHE INTERNAL "" FORCE)

add_test(
    NAME CustomPluginTest
    COMMAND $<TARGET_FILE:${CMAKE_PROJECT_NAME}> --unittest:CustomPluginTest --allow-multiple
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

set_tests_properties(CustomPluginTest PROPERTIES
    TIMEOUT 60
    LABELS "Unit"
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software;LIBGL_ALWAYS_SOFTWARE=1"
)
