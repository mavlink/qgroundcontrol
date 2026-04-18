qt_add_library(QGCLogging STATIC
    LogEntry.cc
    LogEntry.h
    LogEntryTableModel.cc
    LogEntryTableModel.h
    LogFormatter.cc
    LogFormatter.h
    LoggingCategoryModel.cc
    LoggingCategoryModel.h
    LogModel.cc
    LogModel.h
    LogStore.cc
    LogStore.h
    LogStoreQueryModel.cc
    LogStoreQueryModel.h
    LogRemoteSink.cc
    LogRemoteSink.h
    LogManager.cc
    LogManager.h
    QGCLoggingCategory.cc
    QGCLoggingCategory.h
    QGCLoggingCategoryManager.cc
    QGCLoggingCategoryManager.h
)

qt_add_qml_module(QGCLogging
    URI QGroundControl.Logging
    VERSION 1.0
    RESOURCE_PREFIX /qml
    DEPENDENCIES
        QGroundControl.Controls
    QML_FILES
        AppLogging.qml
        LoggingCategoriesDialog.qml
        LoggingSettingsDialog.qml
)

target_link_libraries(QGCLogging
    PUBLIC
        Qt6::Core
        Qt6::Qml
        Qt6::QmlIntegration
    PRIVATE
        Qt6::Concurrent
        Qt6::Network
        Qt6::Quick
        Qt6::Sql
)

target_sources(QGCLogging
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../Database/QGCSqlHelper.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/../Database/QGCSqlHelper.h
)

target_include_directories(QGCLogging
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../Compression
        ${CMAKE_CURRENT_SOURCE_DIR}/../Database
        ${CMAKE_CURRENT_SOURCE_DIR}/../FileSystem
        ${CMAKE_CURRENT_SOURCE_DIR}/../Network
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCLogging)
