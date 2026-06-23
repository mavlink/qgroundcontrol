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

if(Shapelib_ADDED)
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
        Qt6::LocationPrivate   # qgeojson_p.h
        shp
        QGCGeoMath
        QGCLogging
        QGCParsing             # JsonParsing::possibleNaNJsonValue / isJsonFile
)

target_include_directories(QGCGeoFormats
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_precompile_headers(QGCGeoFormats PRIVATE
    <QtCore/QObject>
    <QtCore/QString>
    <QtCore/QJsonObject>
    <QtPositioning/QGeoCoordinate>
    <QtQmlIntegration/QtQmlIntegration>
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCGeoFormats)
