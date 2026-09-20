target_sources(
    ${CMAKE_PROJECT_NAME}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/AllocationTracker.cc
            ${CMAKE_CURRENT_SOURCE_DIR}/AllocationTracker.h
            ${CMAKE_CURRENT_SOURCE_DIR}/MultiSignalSpy.cc
            ${CMAKE_CURRENT_SOURCE_DIR}/MultiSignalSpy.h
            ${CMAKE_CURRENT_SOURCE_DIR}/PropertyTestHelper.h
            ${CMAKE_CURRENT_SOURCE_DIR}/UnitTest.cc
            ${CMAKE_CURRENT_SOURCE_DIR}/UnitTest.h
            ${CMAKE_CURRENT_SOURCE_DIR}/UnitTestTileGenerator.cc
            ${CMAKE_CURRENT_SOURCE_DIR}/UnitTestTileGenerator.h
)

set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/AllocationTracker.cc" TARGET_DIRECTORY ${CMAKE_PROJECT_NAME}
                            PROPERTIES SKIP_PRECOMPILE_HEADERS ON SKIP_UNITY_BUILD_INCLUSION ON
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(BaseClasses)
add_subdirectory(Benchmarking)
add_subdirectory(Fixtures)
add_subdirectory(PropertyTesting)
add_subdirectory(QmlTesting)
add_subdirectory(Tests)
