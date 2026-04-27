# ============================================================================
# OSM Provider
# ============================================================================

qt_add_library(Osm3D STATIC
    OsmParserThread.cc
    OsmParserThread.h
)

target_include_directories(Osm3D PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(Osm3D
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Positioning
    PRIVATE
        QGCGeoMath
        QGCLogging
)

# libosmium pulls <windows.h> via std::filesystem; without these the
# min/max macros and missing M_PI break MSVC compilation of OsmParserThread.cc.
if(WIN32)
    target_compile_definitions(Osm3D
        PRIVATE
            _USE_MATH_DEFINES
            NOMINMAX
            WIN32_LEAN_AND_MEAN
    )
endif()

# ----------------------------------------------------------------------------
# libexpat — XML parser backend for libosmium
# ----------------------------------------------------------------------------

CPMAddPackage(
    NAME libexpat
    GITHUB_REPOSITORY libexpat/libexpat
    GIT_TAG R_2_7_5
    SOURCE_SUBDIR expat
    OPTIONS
        "EXPAT_BUILD_TESTS OFF"
        "EXPAT_BUILD_EXAMPLES OFF"
        "EXPAT_BUILD_TOOLS OFF"
        "EXPAT_BUILD_DOCS OFF"
        "EXPAT_SHARED_LIBS OFF"
)

qgc_disable_dependency_warnings(expat)

target_link_libraries(Osm3D PRIVATE expat)

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
# libosmium — OSM data parsing (header-only, only OsmParserThread.cc consumes it)
# ----------------------------------------------------------------------------

CPMAddPackage(
    NAME libosmium
    GITHUB_REPOSITORY osmcode/libosmium
    GIT_TAG v2.23.0
    DOWNLOAD_ONLY YES
)

target_include_directories(Osm3D
    SYSTEM
    PRIVATE
        ${protozero_SOURCE_DIR}/include
        ${libosmium_SOURCE_DIR}/include
)

# ============================================================================
# Main-target sources — OsmParser + CityMapGeometry pull QGC headers
# (Fact, SettingsManager, Viewer3DSettings, Viewer3DMapProvider) so they stay
# on ${CMAKE_PROJECT_NAME}. They use Osm3D via the link below.
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        CityMapGeometry.cc
        CityMapGeometry.h
        OsmParser.cc
        OsmParser.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# ----------------------------------------------------------------------------
# earcut.hpp — header-only polygon triangulation, only used by OsmParser.cc
# (which lives on the main target). EXCLUDE_FROM_ALL prevents earcut's
# unconditional "fixtures" object library from compiling.
# ----------------------------------------------------------------------------

CPMAddPackage(
    NAME earcut_hpp
    GITHUB_REPOSITORY mapbox/earcut.hpp
    GIT_TAG f36ced7e50254738c4e5af1a239f5fb7b1094007
    EXCLUDE_FROM_ALL YES
    OPTIONS
        "EARCUT_BUILD_TESTS OFF"
        "EARCUT_BUILD_BENCH OFF"
        "EARCUT_BUILD_VIZ OFF"
    SYSTEM YES
)

target_link_libraries(${CMAKE_PROJECT_NAME}
    PRIVATE
        Osm3D
        earcut_hpp::earcut_hpp
)
