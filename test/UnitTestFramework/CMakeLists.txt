target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/MultiSignalSpy.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/MultiSignalSpy.h
        ${CMAKE_CURRENT_SOURCE_DIR}/UnitTest.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/UnitTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_subdirectory(BaseClasses)
add_subdirectory(Benchmarking)
add_subdirectory(Fixtures)
add_subdirectory(PropertyTesting)
add_subdirectory(QmlTesting)
add_subdirectory(Tests)
