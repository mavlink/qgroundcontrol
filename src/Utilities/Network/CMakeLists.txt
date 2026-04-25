qt_add_library(QGCNetwork STATIC
    QGCCachedFileDownload.cc
    QGCCachedFileDownload.h
    QGCFileDownload.cc
    QGCFileDownload.h
    QGCNetworkHelper.cc
    QGCNetworkHelper.h
    RemoteTransport.cc
    RemoteTransport.h
    TransportStrategy.cc
    TransportStrategy.h
)

target_include_directories(QGCNetwork
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(QGCNetwork
    PUBLIC
        Qt6::Core
        Qt6::Network
    PRIVATE
        Qt6::HttpServer
        QGCCompression
        QGCLogging
)

target_precompile_headers(QGCNetwork PRIVATE
    <QtCore/QObject>
    <QtCore/QString>
    <QtCore/QByteArray>
    <QtCore/QUrl>
    <QtNetwork/QNetworkReply>
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCNetwork)
