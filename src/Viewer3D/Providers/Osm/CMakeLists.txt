# ============================================================================
# OSM Provider
# ============================================================================

qt_add_library(
    Osm3D STATIC
    OsmParserThread.cc
    OsmParserThread.h
)

target_include_directories(Osm3D PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(
    Osm3D
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
    target_compile_definitions(
        Osm3D
        PRIVATE
            _USE_MATH_DEFINES
            NOMINMAX
            WIN32_LEAN_AND_MEAN
    )
endif()

# ----------------------------------------------------------------------------
# libexpat — XML parser backend for libosmium
# ----------------------------------------------------------------------------

# With QGC_USE_SYSTEM_LIBS, an installed expat of any version: distributions fix theirs without a new version number
# (Ubuntu 24.04's 2.6.1), so a floor would reject fixed copies, and an unfixed one would be linked just as well. This
# looks for it itself: CPM runs find_package() inside a function, so its EXPAT_FOUND never reaches this file. FindEXPAT
# makes EXPAT::EXPAT, the CMake package expat installs expat::expat.
set(_qgc_system_expat "")
if(QGC_USE_SYSTEM_LIBS OR QGC_SYSTEM_LIBS_ONLY)
    find_package(EXPAT QUIET)
    if(NOT TARGET EXPAT::EXPAT)
        find_package(expat CONFIG QUIET)
    endif()
    foreach(_qgc_expat_target IN ITEMS EXPAT::EXPAT expat::expat)
        if(TARGET ${_qgc_expat_target})
            set(_qgc_system_expat ${_qgc_expat_target})
            break()
        endif()
    endforeach()
endif()

if(_qgc_system_expat)
    # Osm3D, the only target that links expat, is in this directory
    add_library(expat ALIAS ${_qgc_system_expat})
    if(EXPAT_VERSION)
        set(_qgc_expat_version "${EXPAT_VERSION}")
    else()
        set(_qgc_expat_version "${expat_VERSION}")
    endif()
    message(STATUS "QGC: EXPAT: the installed ${_qgc_expat_version} (QGC_USE_SYSTEM_LIBS)")
else()
    CPMAddPackage(
        NAME EXPAT
        GITHUB_REPOSITORY libexpat/libexpat
        GIT_TAG R_2_11_0
        SOURCE_SUBDIR expat
        OPTIONS
            "EXPAT_BUILD_TESTS OFF"
            "EXPAT_BUILD_EXAMPLES OFF"
            "EXPAT_BUILD_TOOLS OFF"
            "EXPAT_BUILD_DOCS OFF"
            "EXPAT_SHARED_LIBS OFF"
    )
    # The library is built here (target expat)
    qgc_require_cpm_added(EXPAT)
    qgc_disable_dependency_warnings(expat)
    get_directory_property(_qgc_expat_version DIRECTORY "${EXPAT_SOURCE_DIR}/expat" DEFINITION PACKAGE_VERSION)
    message(STATUS "QGC: EXPAT: the bundled ${_qgc_expat_version}")
    # The MIT licence asks a copy to carry the notice. A system copy is not shipped
    qgc_install_license(expat "${EXPAT_SOURCE_DIR}/expat/COPYING")
endif()

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

target_include_directories(
    Osm3D SYSTEM
    PRIVATE
        ${protozero_SOURCE_DIR}/include
        ${libosmium_SOURCE_DIR}/include
)

# ============================================================================
# Main-target sources — OsmParser + CityMapGeometry pull QGC headers
# (Fact, SettingsManager, Viewer3DSettings, Viewer3DMapProvider) so they stay
# on ${CMAKE_PROJECT_NAME}. They use Osm3D via the link below.
# ============================================================================

target_sources(
    ${CMAKE_PROJECT_NAME}
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

# Its ISC licence asks every copy to carry the notice. libosmium's Boost licence does not ask it of compiled code, and
# nothing of protozero is compiled: the XML reader that OsmParserThread.cc includes does not include it
if(earcut_hpp_ADDED)
    qgc_install_license(earcut.hpp "${earcut_hpp_SOURCE_DIR}/LICENSE")
endif()

target_link_libraries(
    ${CMAKE_PROJECT_NAME}
    PRIVATE
        Osm3D
        earcut_hpp::earcut_hpp
)
