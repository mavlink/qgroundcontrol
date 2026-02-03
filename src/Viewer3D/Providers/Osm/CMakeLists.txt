# ============================================================================
# OSM Provider
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        CityMapGeometry.cc
        CityMapGeometry.h
        OsmParser.cc
        OsmParser.h
        OsmParserThread.cc
        OsmParserThread.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# ----------------------------------------------------------------------------
# earcut.hpp — polygon triangulation
# ----------------------------------------------------------------------------

CPMAddPackage(
    NAME earcut_hpp
    GITHUB_REPOSITORY mapbox/earcut.hpp
    GIT_TAG f36ced7e50254738c4e5af1a239f5fb7b1094007
    OPTIONS
        "EARCUT_BUILD_TESTS OFF"
        "EARCUT_BUILD_BENCH OFF"
        "EARCUT_BUILD_VIZ OFF"
)

qgc_disable_dependency_warnings(earcut_hpp)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE earcut_hpp::earcut_hpp)

# ----------------------------------------------------------------------------
# libexpat — XML parser backend for libosmium
# ----------------------------------------------------------------------------

CPMAddPackage(
    NAME libexpat
    GITHUB_REPOSITORY libexpat/libexpat
    GIT_TAG R_2_7_4
    SOURCE_SUBDIR expat
    OPTIONS
        "EXPAT_BUILD_TESTS OFF"
        "EXPAT_BUILD_EXAMPLES OFF"
        "EXPAT_BUILD_TOOLS OFF"
        "EXPAT_BUILD_DOCS OFF"
        "EXPAT_SHARED_LIBS OFF"
)

qgc_disable_dependency_warnings(expat)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE expat)

# ----------------------------------------------------------------------------
# protozero — required by libosmium headers
# ----------------------------------------------------------------------------

CPMAddPackage(
    NAME protozero
    GITHUB_REPOSITORY mapbox/protozero
    GIT_TAG v1.8.1
    DOWNLOAD_ONLY YES
)

# ----------------------------------------------------------------------------
# libosmium — OSM data parsing
# ----------------------------------------------------------------------------

CPMAddPackage(
    NAME libosmium
    GITHUB_REPOSITORY osmcode/libosmium
    GIT_TAG v2.23.0
    DOWNLOAD_ONLY YES
)

target_include_directories(${CMAKE_PROJECT_NAME}
    SYSTEM
    PRIVATE
        ${protozero_SOURCE_DIR}/include
        ${libosmium_SOURCE_DIR}/include
)
