# ============================================================================
# Geo Subsystem
# Geographic data handling (GeoJSON, GPX, KML, Shapefiles)
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        # Core files
        GeoFileIO.cc
        GeoFileIO.h
        GeoFormatRegistry.cc
        GeoFormatRegistry.h
        GeoValidation.h
        GeoUtilities.cc
        GeoUtilities.h
        QGCGeo.cc
        QGCGeo.h
        XsdValidator.cc
        XsdValidator.h

        # CSV format
        CSV/CSVHelper.cc
        CSV/CSVHelper.h

        # GeoJSON format
        GeoJSON/GeoJsonHelper.cc
        GeoJSON/GeoJsonHelper.h

        # GPX format
        GPX/GPXHelper.cc
        GPX/GPXHelper.h
        GPX/GPXSchemaValidator.cc
        GPX/GPXSchemaValidator.h

        # KML format
        KML/KMLDomDocument.cc
        KML/KMLDomDocument.h
        KML/KMLHelper.cc
        KML/KMLHelper.h
        KML/KMLSchemaValidator.cc
        KML/KMLSchemaValidator.h

        # OpenAir format
        OpenAir/OpenAirParser.cc
        OpenAir/OpenAirParser.h

        # SHP format
        SHP/SHPHelper.cc
        SHP/SHPHelper.h

        # WKT format
        WKT/WKTHelper.cc
        WKT/WKTHelper.h
)

target_include_directories(${CMAKE_PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/CSV
        ${CMAKE_CURRENT_SOURCE_DIR}/GeoJSON
        ${CMAKE_CURRENT_SOURCE_DIR}/GPX
        ${CMAKE_CURRENT_SOURCE_DIR}/KML
        ${CMAKE_CURRENT_SOURCE_DIR}/OpenAir
        ${CMAKE_CURRENT_SOURCE_DIR}/SHP
        ${CMAKE_CURRENT_SOURCE_DIR}/WKT
)

# KML Schema Resources (OGC KML 2.2 XSD for validation)
qt_add_resources(${CMAKE_PROJECT_NAME} "kml_schema"
    PREFIX "/kml"
    FILES
        KML/ogckml22.xsd
)

# GPX Schema Resources (GPX 1.1 XSD for validation)
qt_add_resources(${CMAKE_PROJECT_NAME} "gpx_schema"
    PREFIX "/gpx"
    FILES
        GPX/gpx11.xsd
)

# Promote imported targets to global visibility when needed (Qt autogen can
# resolve dependencies from parent scope).
function(_qgc_promote_imported_target_global target_name)
    if(TARGET ${target_name})
        get_target_property(_qgc_is_imported ${target_name} IMPORTED)
        if(_qgc_is_imported)
            set_property(TARGET ${target_name} PROPERTY IMPORTED_GLOBAL TRUE)
        endif()
    endif()
endfunction()

# ============================================================================
# GeographicLib Integration
# ============================================================================

CPMAddPackage(
    NAME geographiclib
    VERSION 2.7
    GITHUB_REPOSITORY geographiclib/geographiclib
    GIT_TAG r2.7
    OPTIONS
        "BUILD_BOTH_LIBS OFF"
        "BUILD_DOCUMENTATION OFF"
        "BUILD_MANPAGES OFF"
        "PACKAGE_DEBUG_LIBS OFF"
        "APPLE_MULTIPLE_ARCHITECTURES OFF"
        "INCDIR OFF"
        "BINDIR OFF"
        "SBINDIR OFF"
        "LIBDIR ${CMAKE_INSTALL_LIBDIR}"
        "DLLDIR ${CMAKE_INSTALL_BINDIR}"
        "MANDIR OFF"
        "CMAKEDIR OFF"
        "PKGDIR OFF"
        "DOCDIR OFF"
        "EXAMPLEDIR OFF"
)

if(NOT TARGET GeographicLib::GeographicLib AND TARGET GeographicLib_STATIC)
    add_library(GeographicLib::GeographicLib ALIAS GeographicLib_STATIC)
endif()
if(NOT TARGET GeographicLib::GeographicLib)
    message(FATAL_ERROR "QGC: GeographicLib dependency is required and must provide target GeographicLib::GeographicLib.")
endif()
_qgc_promote_imported_target_global(GeographicLib::GeographicLib)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE GeographicLib::GeographicLib)
if(geographiclib_ADDED)
    qgc_disable_dependency_warnings(GeographicLib_STATIC)
endif()

# ============================================================================
# Shapelib Integration
# ============================================================================

CPMAddPackage(
    NAME Shapelib
    VERSION 1.6.2
    GITHUB_REPOSITORY OSGeo/shapelib
    OPTIONS
        "BUILD_SHAPELIB_CONTRIB OFF"
        "BUILD_APPS OFF"
        "BUILD_TESTING OFF"
)

if(NOT TARGET Shapelib::shp AND TARGET shp)
    add_library(Shapelib::shp ALIAS shp)
endif()
if(NOT TARGET Shapelib::shp)
    message(FATAL_ERROR "QGC: Shapelib dependency is required and must provide target Shapelib::shp.")
endif()
_qgc_promote_imported_target_global(Shapelib::shp)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Shapelib::shp)
if(Shapelib_ADDED)
    qgc_disable_dependency_warnings(shp)
endif()

# ============================================================================
# Required GEOS Integration (via CPMAddPackage)
# ============================================================================

CPMAddPackage(
    NAME GEOS
    VERSION 3.14.1
    GITHUB_REPOSITORY libgeos/geos
    GIT_TAG 3.14.1
    OPTIONS
        "BUILD_SHARED_LIBS OFF"
        "BUILD_BENCHMARKS OFF"
        "BUILD_TESTING OFF"
        "GEOS_BUILD_DEVELOPER OFF"
)

if(NOT TARGET GEOS::geos_c AND TARGET geos_c)
    add_library(GEOS::geos_c ALIAS geos_c)
endif()

if(NOT TARGET GEOS::geos_c)
    message(FATAL_ERROR "QGC: GEOS dependency is required and must provide target GEOS::geos_c.")
endif()
_qgc_promote_imported_target_global(GEOS::geos_c)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE GEOS::geos_c)
if(GEOS_ADDED)
    qgc_disable_dependency_warnings(geos)
    qgc_disable_dependency_warnings(geos_c)
endif()

message(STATUS "QGC: GEOS linked for geo geometry checks")

# ============================================================================
# Required SQLite3 Integration (for PROJ/GDAL)
# ============================================================================

set(QGC_SQLITE_VERSION 3.51.2)
set(QGC_SQLITE_AMALGAMATION_URL "https://sqlite.org/2026/sqlite-amalgamation-3510200.zip")
set(QGC_SQLITE_AMALGAMATION_SHA256 "6e2a845a493026bdbad0618b2b5a0cf48584faab47384480ed9f592d912f23ec")

CPMAddPackage(
    NAME sqlite_amalgamation
    VERSION ${QGC_SQLITE_VERSION}
    URL ${QGC_SQLITE_AMALGAMATION_URL}
    URL_HASH SHA256=${QGC_SQLITE_AMALGAMATION_SHA256}
    DOWNLOAD_ONLY YES
)

if(NOT EXISTS "${sqlite_amalgamation_SOURCE_DIR}/sqlite3.c" OR NOT EXISTS "${sqlite_amalgamation_SOURCE_DIR}/sqlite3.h" OR NOT EXISTS "${sqlite_amalgamation_SOURCE_DIR}/shell.c")
    message(FATAL_ERROR
        "QGC: SQLite amalgamation archive is missing required files. "
        "Expected sqlite3.c/sqlite3.h/shell.c in ${sqlite_amalgamation_SOURCE_DIR}"
    )
endif()

add_library(sqlite3 STATIC "${sqlite_amalgamation_SOURCE_DIR}/sqlite3.c")
target_include_directories(sqlite3 PUBLIC
    $<BUILD_INTERFACE:${sqlite_amalgamation_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include/sqlite-amalgamation>
)
target_compile_definitions(sqlite3 PRIVATE
    SQLITE_ENABLE_COLUMN_METADATA
    SQLITE_ENABLE_FTS4
    SQLITE_ENABLE_FTS5
    SQLITE_ENABLE_JSON1
    SQLITE_ENABLE_RBU
    SQLITE_ENABLE_RTREE
    SQLITE_ENABLE_STAT4
    SQLITE_THREADSAFE=1
)

if(NOT WIN32)
    find_package(Threads REQUIRED)
    target_link_libraries(sqlite3 PUBLIC Threads::Threads)
    if(APPLE)
        target_link_libraries(sqlite3 PUBLIC m)
    else()
        target_link_libraries(sqlite3 PUBLIC m dl)
    endif()
endif()

if(NOT CMAKE_SKIP_INSTALL_RULES)
    install(FILES
        "${sqlite_amalgamation_SOURCE_DIR}/sqlite3.h"
        "${sqlite_amalgamation_SOURCE_DIR}/sqlite3ext.h"
        DESTINATION include/sqlite-amalgamation
    )
    install(TARGETS sqlite3
        EXPORT targets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

add_executable(sqlite "${sqlite_amalgamation_SOURCE_DIR}/shell.c")
target_link_libraries(sqlite PRIVATE sqlite3)

if(NOT TARGET SQLite::SQLite3)
    add_library(SQLite::SQLite3 ALIAS sqlite3)
endif()

set(SQLite3_INCLUDE_DIR "${sqlite_amalgamation_SOURCE_DIR}")
set(SQLite3_LIBRARY "sqlite3")
set(SQLite3_VERSION "${QGC_SQLITE_VERSION}")
set(SQLite3_INCLUDE_DIRS "${SQLite3_INCLUDE_DIR}")
set(SQLite3_LIBRARIES "${SQLite3_LIBRARY}")
set(_qgc_proj_sqlite3_exe "$<TARGET_FILE:sqlite>")
if(CMAKE_CROSSCOMPILING)
    find_program(QGC_HOST_SQLITE3_EXECUTABLE
        NAMES sqlite3 sqlite
        NO_CMAKE_FIND_ROOT_PATH
    )
    if(QGC_HOST_SQLITE3_EXECUTABLE)
        set(_qgc_proj_sqlite3_exe "${QGC_HOST_SQLITE3_EXECUTABLE}")
    else()
        # Fallback for cross-build hosts without sqlite3 CLI: emulate the
        # minimal sqlite3 shell behavior PROJ needs (read SQL from stdin).
        find_package(Python3 COMPONENTS Interpreter REQUIRED)
        set(_qgc_sqlite_wrapper_dir "${CMAKE_CURRENT_BINARY_DIR}")
        set(_qgc_sqlite_wrapper_py "${_qgc_sqlite_wrapper_dir}/qgc_sqlite_wrapper.py")
        file(WRITE "${_qgc_sqlite_wrapper_py}" [=[
#!/usr/bin/env python3
import sqlite3
import sys

if len(sys.argv) < 2:
    print("usage: qgc_sqlite_wrapper.py <database>", file=sys.stderr)
    sys.exit(2)

database = sys.argv[1]
sql_bytes = sys.stdin.buffer.read()
for encoding in ("utf-8", "cp1252", "latin-1"):
    try:
        sql = sql_bytes.decode(encoding)
        break
    except UnicodeDecodeError:
        continue
connection = sqlite3.connect(database)
try:
    connection.executescript(sql)
    connection.commit()
finally:
    connection.close()
]=])

        if(WIN32)
            set(_qgc_sqlite_wrapper_cmd "${_qgc_sqlite_wrapper_dir}/qgc_sqlite_wrapper.cmd")
            file(WRITE "${_qgc_sqlite_wrapper_cmd}" "@echo off\r\n\"${Python3_EXECUTABLE}\" \"${_qgc_sqlite_wrapper_py}\" %*\r\n")
            set(_qgc_proj_sqlite3_exe "${_qgc_sqlite_wrapper_cmd}")
        else()
            file(CHMOD "${_qgc_sqlite_wrapper_py}"
                PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
            )
            set(_qgc_proj_sqlite3_exe "${_qgc_sqlite_wrapper_py}")
        endif()

        message(STATUS "QGC: Using Python-based sqlite3 wrapper for cross-compiled PROJ database generation")
    endif()
endif()
set(EXE_SQLITE3 "${_qgc_proj_sqlite3_exe}")
set(SQLite3_INCLUDE_DIR "${SQLite3_INCLUDE_DIR}" CACHE PATH "Path to SQLite3 include directory" FORCE)
set(SQLite3_LIBRARY "${SQLite3_LIBRARY}" CACHE STRING "SQLite3 library target for PROJ lookup" FORCE)
set(SQLite3_VERSION "${SQLite3_VERSION}" CACHE STRING "SQLite3 version for dependency checks" FORCE)
set(SQLite3_INCLUDE_DIRS "${SQLite3_INCLUDE_DIRS}" CACHE PATH "SQLite3 include dirs for dependency checks" FORCE)
set(SQLite3_LIBRARIES "${SQLite3_LIBRARIES}" CACHE STRING "SQLite3 libraries for dependency checks" FORCE)
set(EXE_SQLITE3 "${EXE_SQLITE3}" CACHE FILEPATH "Path to sqlite3 executable for PROJ database generation" FORCE)
set(SQLite3_HAS_COLUMN_METADATA ON CACHE BOOL "SQLite3 built with column metadata support" FORCE)
set(SQLite3_HAS_MUTEX_ALLOC ON CACHE BOOL "SQLite3 built with mutex support" FORCE)
set(SQLite3_HAS_RTREE ON CACHE BOOL "SQLite3 built with RTree support" FORCE)

qgc_disable_dependency_warnings(sqlite3)
qgc_disable_dependency_warnings(sqlite)

# ============================================================================
# Required PROJ Integration (via CPMAddPackage)
# ============================================================================

set(QGC_PROJ_VERSION 9.7.0)

set(_qgc_prev_skip_install_rules_proj "${CMAKE_SKIP_INSTALL_RULES}")
set(CMAKE_SKIP_INSTALL_RULES ON)

CPMAddPackage(
    NAME PROJ
    VERSION ${QGC_PROJ_VERSION}
    GITHUB_REPOSITORY OSGeo/PROJ
    GIT_TAG ${QGC_PROJ_VERSION}
    PATCHES
        "${CMAKE_SOURCE_DIR}/cmake/patches/proj-guard-uninstall-target.patch"
    OPTIONS
        "BUILD_SHARED_LIBS OFF"
        "BUILD_APPS OFF"
        "BUILD_CCT OFF"
        "BUILD_CS2CS OFF"
        "BUILD_GEOD OFF"
        "BUILD_GIE OFF"
        "BUILD_PROJ OFF"
        "BUILD_PROJINFO OFF"
        "BUILD_PROJSYNC OFF"
        "BUILD_EXAMPLES OFF"
        "BUILD_TESTING OFF"
        "ENABLE_TIFF OFF"
        "ENABLE_CURL OFF"
)

set(CMAKE_SKIP_INSTALL_RULES "${_qgc_prev_skip_install_rules_proj}")
unset(_qgc_prev_skip_install_rules_proj)
if(NOT EXISTS "${PROJ_BINARY_DIR}/cmake_install.cmake")
    file(WRITE "${PROJ_BINARY_DIR}/cmake_install.cmake" "# QGC stub: PROJ install rules are intentionally skipped\n")
endif()

if(TARGET generate_proj_db AND NOT CMAKE_CROSSCOMPILING)
    add_dependencies(generate_proj_db sqlite)
endif()

if(NOT TARGET PROJ::proj AND TARGET proj)
    add_library(PROJ::proj ALIAS proj)
endif()

if(NOT TARGET PROJ::proj)
    message(FATAL_ERROR "QGC: PROJ dependency is required and must provide target PROJ::proj.")
endif()
_qgc_promote_imported_target_global(PROJ::proj)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE PROJ::proj)
if(PROJ_ADDED)
    qgc_disable_dependency_warnings(proj)
endif()

message(STATUS "QGC: PROJ linked for CRS transforms")

# ============================================================================
# Required GDAL/OGR Integration (via CPMAddPackage)
# ============================================================================

# GDAL requires PROJ; force module-mode lookup against our CPM PROJ build.
set(GDAL_FIND_PACKAGE_PROJ_MODE MODULE CACHE STRING "Mode to use for find_package(PROJ)" FORCE)
set(PROJ_INCLUDE_DIR "${PROJ_SOURCE_DIR}/src" CACHE PATH "Path to PROJ include directory" FORCE)
set(PROJ_LIBRARY "${PROJ_BINARY_DIR}/lib/libproj.a" CACHE FILEPATH "Path to PROJ library" FORCE)
set(PROJ_VERSION "${QGC_PROJ_VERSION}" CACHE STRING "PROJ version used by GDAL dependency checks" FORCE)
set(GDAL_USE_HDFS OFF CACHE BOOL "Disable HDFS in GDAL to avoid JNI/HDFS imported target conflicts" FORCE)
set_property(GLOBAL PROPERTY ALLOW_DUPLICATE_CUSTOM_TARGETS ON)
set(_qgc_prev_skip_install_rules "${CMAKE_SKIP_INSTALL_RULES}")
set(CMAKE_SKIP_INSTALL_RULES ON)

CPMAddPackage(
    NAME GDAL
    VERSION 3.12.2
    GITHUB_REPOSITORY OSGeo/gdal
    GIT_TAG v3.12.2
    PATCHES
        "${CMAKE_SOURCE_DIR}/cmake/patches/gdal-guard-export-targets.patch"
        "${CMAKE_SOURCE_DIR}/cmake/patches/gdal-guard-hdfs-jvm-target.patch"
    OPTIONS
        "BUILD_SHARED_LIBS OFF"
        "BUILD_APPS OFF"
        "BUILD_TESTING OFF"
        "BUILD_DOCS OFF"
        "BUILD_PYTHON_BINDINGS OFF"
        "GDAL_BUILD_OPTIONAL_DRIVERS OFF"
        "OGR_BUILD_OPTIONAL_DRIVERS OFF"
        "GDAL_USE_CURL OFF"
        "GDAL_USE_EXPAT OFF"
        "GDAL_USE_EXTERNAL_LIBS OFF"
        "GDAL_USE_HDFS OFF"
        "GDAL_USE_SQLITE3 ON"
        # Disable default-on driver exceptions: keep only GDAL core in this repo.
        "GDAL_ENABLE_DRIVER_GTIFF OFF"
        "GDAL_ENABLE_DRIVER_VRT OFF"
        "OGR_ENABLE_DRIVER_GEOJSON OFF"
        "OGR_ENABLE_DRIVER_SHAPE OFF"
        "EMBED_RESOURCE_FILES OFF"
        "USE_ONLY_EMBEDDED_RESOURCE_FILES OFF"
)
set(CMAKE_SKIP_INSTALL_RULES "${_qgc_prev_skip_install_rules}")
unset(_qgc_prev_skip_install_rules)
if(NOT EXISTS "${GDAL_BINARY_DIR}/cmake_install.cmake")
    file(WRITE "${GDAL_BINARY_DIR}/cmake_install.cmake" "# QGC stub: GDAL install rules are intentionally skipped\n")
endif()

if(DEFINED gdal_SOURCE_DIR)
    # Remove non-Qt *.qml test fixtures that confuse Qt's QML tooling.
    file(REMOVE
        "${gdal_SOURCE_DIR}/autotest/gdrivers/data/ngw/96.qml"
        "${gdal_SOURCE_DIR}/autotest/utilities/data/color_paletted_red_green_0-255.qml"
        "${gdal_SOURCE_DIR}/autotest/utilities/data/color_pseudocolor_spectral_0-100.qml"
        "${gdal_SOURCE_DIR}/autotest/gcore/data/qgis_qml_paletted.qml"
        "${gdal_SOURCE_DIR}/autotest/gcore/data/qgis_qml_singlebandpseudocolor.qml"
    )
endif()

if(NOT TARGET GDAL::GDAL AND TARGET gdal)
    add_library(GDAL::GDAL ALIAS gdal)
endif()

if(NOT TARGET GDAL::GDAL AND TARGET GDAL)
    add_library(GDAL::GDAL ALIAS GDAL)
endif()

if(NOT TARGET GDAL::GDAL)
    message(FATAL_ERROR "QGC: GDAL dependency is required and must provide target GDAL::GDAL (or gdal/GDAL).")
endif()

_qgc_promote_imported_target_global(GDAL::GDAL)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE GDAL::GDAL)

if(TARGET gdal)
    qgc_disable_dependency_warnings(gdal)
endif()

message(STATUS "QGC: GDAL linked for geospatial format support")
