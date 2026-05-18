qt_add_library(QGCLogManager STATIC
    LogEntry.cc
    LogEntry.h
    LogEntryTableModel.cc
    LogEntryTableModel.h
    LogFormatter.cc
    LogFormatter.h
    LogModel.cc
    LogModel.h
    LogManager.cc
    LogManager.h
)

qt_add_qml_module(QGCLogManager
    URI QGroundControl.LogManager
    VERSION 1.0
    RESOURCE_PREFIX /qml
    DEPENDENCIES
        QGroundControl.Controls
        QGroundControl.Logging
    QML_FILES
        LoggingCategoriesDialog.qml
)

target_include_directories(QGCLogManager
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/Settings
        ${CMAKE_SOURCE_DIR}/src/FactSystem
)

target_link_libraries(QGCLogManager
    PUBLIC
        Qt6::Core
        Qt6::Qml
        Qt6::QmlIntegration
    PRIVATE
        Qt6::Concurrent
        Qt6::Quick
        QGCFileSystem
        QGCLogging
)

target_precompile_headers(QGCLogManager PRIVATE
    <QtCore/QObject>
    <QtCore/QString>
    <QtCore/QDateTime>
    <QtCore/QAbstractTableModel>
    <QtQmlIntegration/QtQmlIntegration>
)

# AppLogging.qml is loaded via dynamic URL (SettingsPages.json), so the QML
# import scanner can't see QGroundControl.LogManager. Force-link the plugin
# so its type registrations end up in the executable.
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCLogManager QGCLogManagerplugin)
