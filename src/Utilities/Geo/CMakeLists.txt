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
        GeoValidation.cc
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

        # GeoPackage format
        GeoPackage/GeoPackageHelper.cc
        GeoPackage/GeoPackageHelper.h

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

        # WKB format
        WKB/WKBHelper.cc
        WKB/WKBHelper.h

        # WKT format
        WKT/WKTHelper.cc
        WKT/WKTHelper.h
)

target_include_directories(${CMAKE_PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/CSV
        ${CMAKE_CURRENT_SOURCE_DIR}/GeoJSON
        ${CMAKE_CURRENT_SOURCE_DIR}/GeoPackage
        ${CMAKE_CURRENT_SOURCE_DIR}/GPX
        ${CMAKE_CURRENT_SOURCE_DIR}/KML
        ${CMAKE_CURRENT_SOURCE_DIR}/OpenAir
        ${CMAKE_CURRENT_SOURCE_DIR}/SHP
        ${CMAKE_CURRENT_SOURCE_DIR}/WKB
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
    PATCHES geographiclib.patch
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE GeographicLib::GeographicLib)

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

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE shp)
