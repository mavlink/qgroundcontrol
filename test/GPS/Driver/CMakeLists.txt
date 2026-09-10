# GPS driver tests.
add_subdirectory(Protocols)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSDriverTest.cc GPSDriverTest.h GPSDriverTimeTest.cc GPSDriverTimeTest.h)
target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSDriverUBXTest.cc GPSDriverUBXTest.h)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Driver bridge and UTC conversion tests exercise the production protocol implementation.
set_source_files_properties(
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSDriverTest.cc ${CMAKE_CURRENT_SOURCE_DIR}/GPSDriverTimeTest.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSDriverUBXTest.cc TARGET_DIRECTORY ${CMAKE_PROJECT_NAME}
    PROPERTIES INCLUDE_DIRECTORIES "$<TARGET_PROPERTY:QGCGPSNative,INCLUDE_DIRECTORIES>"
               COMPILE_DEFINITIONS "$<TARGET_PROPERTY:QGCGPSNative,COMPILE_DEFINITIONS>"
               SKIP_PRECOMPILE_HEADERS ON
)

add_qgc_test(GPSDriverTest LABELS Unit)
add_qgc_test(GPSDriverUBXTest LABELS Unit)
# A non-UTC process timezone makes accidental mktime use fail even on UTC CI hosts.
add_qgc_test(GPSDriverTimeTest LABELS Unit ENV_MODIFICATION "TZ=set:EST5EDT")

add_subdirectory(Transport)
