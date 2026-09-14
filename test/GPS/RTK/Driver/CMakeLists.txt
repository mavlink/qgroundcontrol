# RTK/Driver tests.
target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSDriverTest.cc GPSDriverTest.h)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Only the callback-bridge test needs PX4 implementation headers and definitions.
set_source_files_properties(
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSDriverTest.cc TARGET_DIRECTORY ${CMAKE_PROJECT_NAME}
    PROPERTIES INCLUDE_DIRECTORIES "$<TARGET_PROPERTY:px4-gpsdrivers,INCLUDE_DIRECTORIES>"
               COMPILE_DEFINITIONS "$<TARGET_PROPERTY:px4-gpsdrivers,COMPILE_DEFINITIONS>"
               SKIP_PRECOMPILE_HEADERS ON
)

add_qgc_test(GPSDriverTest LABELS Unit)
