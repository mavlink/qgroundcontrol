#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QUrl>
#include <QtNetwork/QSslError>
#include <gst/app/gstappsrc.h>

#include "VideoReceiver.h"

class QWebSocket;

class QGCWebSocketVideoSource : public QObject
{
    Q_OBJECT

public:
    explicit QGCWebSocketVideoSource(const QUrl& url, const VideoReceiver::NetworkSourceConfig& config,
                                     GstElement* appsrc, QObject* parent = nullptr);
    ~QGCWebSocketVideoSource() override;

    bool start(QString& error);
    void stop();

    static bool isCompleteJpeg(const QByteArray& message);

signals:
    void connected();
    void disconnected();

private slots:
    void _onConnected();
    void _onDisconnected();
    void _onBinaryMessageReceived(const QByteArray& message);
    void _onTextMessageReceived(const QString& message);
    void _onError();
    void _onSslErrors(const QList<QSslError>& errors);

private:
    void _pushFrameToAppsrc(const QByteArray& jpegData);

    static constexpr quint64 kMaximumJpegBytes = 16U * 1024U * 1024U;

    QUrl _url;
    VideoReceiver::NetworkSourceConfig _config;
    GstElement* _appsrc = nullptr;
    QWebSocket* _webSocket = nullptr;
    bool _running = false;
};
