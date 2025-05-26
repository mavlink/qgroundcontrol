target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        TerrainQueryTest.cc
        TerrainQueryTest.h
        TerrainTileTest.cc
        TerrainTileTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
