target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/MultiSignalSpyTest.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/MultiSignalSpyTest.h
        ${CMAKE_CURRENT_SOURCE_DIR}/SignalEmitter.h
        ${CMAKE_CURRENT_SOURCE_DIR}/TestBaseClassesTest.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/TestBaseClassesTest.h
        ${CMAKE_CURRENT_SOURCE_DIR}/TestFixturesTest.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/TestFixturesTest.h
        ${CMAKE_CURRENT_SOURCE_DIR}/UnitTestAsyncHelpersTest.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/UnitTestAsyncHelpersTest.h
        ${CMAKE_CURRENT_SOURCE_DIR}/UnitTestLogExpectationTest.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/UnitTestLogExpectationTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_qgc_test(MultiSignalSpyTest LABELS Unit)
add_qgc_test(TestBaseClassesTest LABELS Unit)
add_qgc_test(TestFixturesTest LABELS Unit)
add_qgc_test(UnitTestAsyncHelpersTest LABELS Unit Utilities)
add_qgc_test(UnitTestLogExpectationTest LABELS Unit)
