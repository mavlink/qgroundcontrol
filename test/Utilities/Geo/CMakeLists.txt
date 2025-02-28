find_package(Qt6 REQUIRED COMPONENTS Core Positioning Test)

target_sources(UtilitiesTest
    PRIVATE
        GeoTest.cc
        GeoTest.h
)

target_link_libraries(UtilitiesTest
    PRIVATE
        Qt6::Test
    PUBLIC
        Qt6::Positioning
        qgcunittest
)

target_include_directories(UtilitiesTest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
