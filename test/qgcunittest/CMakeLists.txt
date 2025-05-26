target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        MultiSignalSpy.cc
        MultiSignalSpy.h
        MultiSignalSpyV2.cc
        MultiSignalSpyV2.h
        UnitTest.cc
        UnitTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
