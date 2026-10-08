qt_add_library(QGCGeoFormats STATIC
    GeoJsonHelper.cc
    GeoJsonHelper.h
    KMLDomDocument.cc
    KMLDomDocument.h
    KMLHelper.cc
    KMLHelper.h
    KMLSchemaValidator.cc
    KMLSchemaValidator.h
    ShapeFileHelper.cc
    ShapeFileHelper.h
    SHPFileHelper.cc
    SHPFileHelper.h
)

qt_add_qml_module(QGCGeoFormats
    URI QGroundControl.Geo
    VERSION 1.0
    RESOURCE_PREFIX /qml
)

qt_add_resources(QGCGeoFormats "kml_schema"
    PREFIX "/kml"
    FILES
        ogckml22.xsd
)

if(QGC_USE_SYSTEM_LIBS OR QGC_SYSTEM_LIBS_ONLY)
    # System shapelib (ALT: libshape-devel) ships libshp.so + shapefil.h without a CMake config.
    find_library(SHAPELIB_LIBRARY NAMES shp)
    find_path(SHAPELIB_INCLUDE_DIR NAMES shapefil.h)
    if(NOT SHAPELIB_LIBRARY OR NOT SHAPELIB_INCLUDE_DIR)
        message(FATAL_ERROR "QGC: Shapelib (required dependency) was not resolved from system")
    endif()
    add_library(shp SHARED IMPORTED)
    set_target_properties(shp PROPERTIES
        IMPORTED_LOCATION "${SHAPELIB_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${SHAPELIB_INCLUDE_DIR}")
else()
    CPMAddPackage(
        NAME Shapelib
        VERSION 1.6.3
        GITHUB_REPOSITORY OSGeo/shapelib
        GIT_TAG v1.6.3
        OPTIONS
            "BUILD_SHAPELIB_CONTRIB OFF"
            "BUILD_APPS OFF"
            "BUILD_TESTING OFF"
    )

    qgc_require_cpm_added(Shapelib)

    qgc_disable_dependency_warnings(shp)
endif()

target_link_libraries(QGCGeoFormats
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Positioning
        Qt6::Qml
        Qt6::QmlIntegration
        Qt6::Xml
    PRIVATE
        Qt6::Location
        Qt6::LocationPrivate
        shp
        QGCGeoMath
        QGCLogging
        QGCParsing
)

target_include_directories(QGCGeoFormats PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_precompile_headers(QGCGeoFormats PRIVATE
    <QtCore/QObject>
    <QtCore/QString>
    <QtCore/QJsonObject>
    <QtPositioning/QGeoCoordinate>
    <QtQmlIntegration/QtQmlIntegration>
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCGeoFormats)
