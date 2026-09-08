# RTK/Driver tests.
target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSDriverTest.cc GPSDriverTest.h GPSDriverTimeTest.cc GPSDriverTimeTest.h)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Driver bridge and UTC conversion tests exercise the actual PX4 implementation.
set_source_files_properties(
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSDriverTest.cc ${CMAKE_CURRENT_SOURCE_DIR}/GPSDriverTimeTest.cc TARGET_DIRECTORY
    ${CMAKE_PROJECT_NAME}
    PROPERTIES INCLUDE_DIRECTORIES "$<TARGET_PROPERTY:px4-gpsdrivers,INCLUDE_DIRECTORIES>"
               COMPILE_DEFINITIONS "$<TARGET_PROPERTY:px4-gpsdrivers,COMPILE_DEFINITIONS>"
               SKIP_PRECOMPILE_HEADERS ON
)

add_qgc_test(GPSDriverTest LABELS Unit)
# A non-UTC process timezone makes accidental mktime use fail even on UTC CI hosts.
add_qgc_test(GPSDriverTimeTest LABELS Unit ENV_MODIFICATION "TZ=set:EST5EDT")
