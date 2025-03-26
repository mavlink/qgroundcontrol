find_package(Qt6 REQUIRED COMPONENTS Core Positioning Test)

target_sources(qgctest
    PRIVATE
        MultiSignalSpy.cc
        MultiSignalSpy.h
        MultiSignalSpyV2.cc
        MultiSignalSpyV2.h
        UnitTest.cc
        UnitTest.h
)

target_link_libraries(qgctest
    PRIVATE
        Qt6::Test
    PUBLIC
        Qt6::Core
        Qt6::Positioning
)

target_include_directories(qgctest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
