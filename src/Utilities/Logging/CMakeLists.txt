# Leaf lib so Compression/FileSystem/Network can PRIVATE-link without cycles.
# The heavier log UI / store / remote sink lives in ../LogManager.

qt_add_library(QGCLogging STATIC
    QGCLoggingCategory.cc
    QGCLoggingCategory.h
    QGCLoggingCategoryManager.cc
    QGCLoggingCategoryManager.h
    LoggingCategoryModel.cc
    LoggingCategoryModel.h
)

qt_add_qml_module(QGCLogging
    URI QGroundControl.Logging
    VERSION 1.0
    RESOURCE_PREFIX /qml
)

target_link_libraries(QGCLogging
    PUBLIC
        Qt6::Core
        Qt6::Qml
        Qt6::QmlIntegration
)

target_include_directories(QGCLogging
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_precompile_headers(QGCLogging PRIVATE
    <QtCore/QObject>
    <QtCore/QString>
    <QtCore/QLoggingCategory>
    <QtQmlIntegration/QtQmlIntegration>
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCLogging)
