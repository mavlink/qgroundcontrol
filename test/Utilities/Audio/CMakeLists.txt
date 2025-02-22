find_package(Qt6 REQUIRED COMPONENTS Core Test)

target_sources(UtilitiesTest
    PRIVATE
        AudioOutputTest.cc
        AudioOutputTest.h
)

target_link_libraries(UtilitiesTest
    PRIVATE
        Qt6::Test
    PUBLIC
        qgcunittest
)

target_include_directories(UtilitiesTest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
