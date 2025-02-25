find_package(Qt6 REQUIRED COMPONENTS Core Positioning Test)

qt_add_library(TerrainTest
    STATIC
        TerrainQueryTest.cc
        TerrainQueryTest.h
        TerrainTileTest.cc
        TerrainTileTest.h
)

target_link_libraries(TerrainTest
    PRIVATE
        Qt6::Positioning
        Qt6::Test
        Terrain
    PUBLIC
        qgcunittest
)

target_include_directories(TerrainTest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
