target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/MultiSignalSpyTest.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/MultiSignalSpyTest.h
        ${CMAKE_CURRENT_SOURCE_DIR}/TestBaseClassesTest.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/TestBaseClassesTest.h
        ${CMAKE_CURRENT_SOURCE_DIR}/TestFixturesTest.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/TestFixturesTest.h
        ${CMAKE_CURRENT_SOURCE_DIR}/UnitTestAsyncHelpersTest.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/UnitTestAsyncHelpersTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
