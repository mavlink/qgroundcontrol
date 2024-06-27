find_package(Qt6 REQUIRED COMPONENTS Core Positioning Test)

qt_add_library(TerrainTest
    STATIC
        TerrainQueryTest.cc
        TerrainQueryTest.h
)

target_link_libraries(TerrainTest
    PRIVATE
        Qt6::Test
    PUBLIC
        Qt6::Positioning
        qgcunittest
        Terrain
)

target_include_directories(TerrainTest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
