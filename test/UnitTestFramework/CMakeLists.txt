# ============================================================================
# QGC Unit Test Framework
# Core testing utilities and helpers
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        MockHelpers.h
        MockHttpServer.h
        MultiSignalSpy.cc
        MultiSignalSpy.h
        MultiSignalSpyTest.cc
        MultiSignalSpyTest.h
        ParameterizedTest.h
        QtTestExtensions.h
        TestDataFactories.h
        TestDataPaths.h
        TestFixtures.cc
        TestFixtures.h
        TestFixturesTest.cc
        TestFixturesTest.h
        TestHelpers.h
        UnitTest.cc
        UnitTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
