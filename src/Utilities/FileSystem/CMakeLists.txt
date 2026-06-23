qt_add_library(QGCFileSystem STATIC
    QGCFileHelper.cc
    QGCFileHelper.h
    QGCFileWatcher.cc
    QGCFileWatcher.h
    QGCFileWriter.cc
    QGCFileWriter.h
)

target_include_directories(QGCFileSystem
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(QGCFileSystem
    PUBLIC
        Qt6::Core
    PRIVATE
        QGCLogging
)

target_precompile_headers(QGCFileSystem PRIVATE
    <QtCore/QObject>
    <QtCore/QString>
    <QtCore/QByteArray>
    <QtCore/QFile>
    <QtCore/QDir>
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCFileSystem)
