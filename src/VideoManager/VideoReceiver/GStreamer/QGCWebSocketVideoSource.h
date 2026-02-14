#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtCore/QUrl>
#include <QtCore/QAtomicInt>

#include <gst/gst.h>
#include <gst/app/gstappsrc.h>

Q_DECLARE_LOGGING_CATEGORY(QGCWebSocketVideoSourceLog)

class QWebSocket;

class QGCWebSocketVideoSource : public QObject
{
    Q_OBJECT

public:
    explicit QGCWebSocketVideoSource(const QUrl &url, GstElement *appsrc, QObject *parent = nullptr);
    ~QGCWebSocketVideoSource();

    void start();
    void stop();
    bool isConnected() const;

    void setTimeout(uint32_t timeoutSec);
    void setReconnectDelay(uint32_t delayMs);
    void setHeartbeatInterval(uint32_t intervalMs);

signals:
    void connected();
    void disconnected();
    void errorOccurred(const QString &error);

private slots:
    void _onConnected();
    void _onDisconnected();
    void _onBinaryMessageReceived(const QByteArray &message);
    void _onTextMessageReceived(const QString &message);
    void _onError();
    void _onSslErrors(const QList<QSslError> &errors);
    void _sendHeartbeat();
    void _checkConnectionTimeout();
    void _reconnect();

private:
    void _pushFrameToAppsrc(const QByteArray &jpegData);
    void _cleanupWebSocket();

    QUrl _url;
    QWebSocket *_webSocket = nullptr;
    GstElement *_appsrc = nullptr;
    QAtomicInt _appsrcValid{1};

    QTimer _heartbeatTimer;
    QTimer _connectionTimeoutTimer;
    QTimer _reconnectTimer;

    uint32_t _timeoutSec = 10;
    uint32_t _reconnectDelayMs = 2000;
    uint32_t _heartbeatIntervalMs = 5000;

    bool _running = false;
    bool _connected = false;
    uint64_t _framesReceived = 0;

    bool _expectingBinaryFrame = false;
    int _expectedFrameSize = 0;
};
