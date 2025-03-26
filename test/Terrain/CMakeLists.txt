find_package(Qt6 REQUIRED COMPONENTS Core Positioning Test)

target_sources(qgctest
    PRIVATE
        TerrainQueryTest.cc
        TerrainQueryTest.h
        TerrainTileTest.cc
        TerrainTileTest.h
)

target_link_libraries(qgctest
    PRIVATE
        Qt6::Positioning
        Qt6::Test
)

target_include_directories(qgctest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
