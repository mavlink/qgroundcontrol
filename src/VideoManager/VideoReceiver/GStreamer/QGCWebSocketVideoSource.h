/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtCore/QQueue>
#include <QtCore/QPair>
#include <QtCore/QDateTime>
#include <QtWebSockets/QWebSocket>
#include <QtNetwork/QAbstractSocket>

#include <gst/gst.h>
#include <gst/app/gstappsrc.h>

/**
 * @brief WebSocket video source for QGroundControl
 *
 * Provides WebSocket-based video streaming with GStreamer appsrc integration.
 * Implements bidirectional communication for adaptive quality control and
 * automatic reconnection for robust video streaming from drones.
 *
 * Protocol (compatible with PixEagle):
 * - Server sends JSON metadata followed by binary JPEG frame
 * - Client can request quality adjustments and send heartbeats
 */
class QGCWebSocketVideoSource : public QObject
{
    Q_OBJECT

public:
    explicit QGCWebSocketVideoSource(
        const QString &url,
        int timeout = 10,
        int reconnectDelay = 2000,
        int heartbeatInterval = 5000,
        int minQuality = 60,
        int maxQuality = 95,
        bool adaptiveQuality = true,
        QObject *parent = nullptr
    );
    ~QGCWebSocketVideoSource();

    GstElement* appsrcElement() { return _appsrc; }
    bool isConnected() const { return _connected; }
    int currentQuality() const { return _currentQuality; }
    qreal bandwidthEstimate() const { return _bandwidthBytesPerSecond; }

public slots:
    void start();
    void stop();
    void setQuality(int quality);

signals:
    void connected();
    void disconnected();
    void error(const QString &errorString);
    void frameReceived(int size);
    void qualityChanged(int quality);
    void bandwidthUpdated(qreal bytesPerSecond);
    void stateChanged(const QString &state);

private slots:
    void onConnected();
    void onDisconnected();
    void onTextMessageReceived(const QString &message);
    void onBinaryMessageReceived(const QByteArray &message);
    void onError(QAbstractSocket::SocketError socketError);
    void onSslErrors(const QList<QSslError> &errors);
    void onHeartbeatTimer();
    void onReconnectTimer();

private:
    void createAppsrcElement();
    void pushFrameToAppsrc(const QByteArray &frameData);
    void sendQualityRequest(int quality);
    void sendHeartbeat();
    void updateBandwidthEstimate(int frameSize);
    void scheduleReconnect();
    void cleanupAppsrc();

    // WebSocket connection
    QWebSocket *_webSocket = nullptr;
    QString _url;
    bool _connected = false;
    bool _shouldReconnect = true;

    // GStreamer appsrc element
    GstElement *_appsrc = nullptr;

    // Timers
    QTimer *_heartbeatTimer = nullptr;
    QTimer *_reconnectTimer = nullptr;

    // Frame state tracking
    bool _expectingBinaryFrame = false;
    int _expectedFrameSize = 0;
    qint64 _lastFrameTime = 0;
    quint64 _frameCount = 0;

    // Quality control
    int _currentQuality = 85;
    int _minQuality = 60;
    int _maxQuality = 95;
    bool _adaptiveQuality = true;

    // Bandwidth tracking for adaptive quality
    QQueue<QPair<qint64, int>> _frameHistory;  // <timestamp, size>
    qreal _bandwidthBytesPerSecond = 0;
    static constexpr int MAX_FRAME_HISTORY = 30;  // Track last 30 frames

    // Configuration
    int _timeout = 10;
    int _reconnectDelay = 2000;
    int _heartbeatInterval = 5000;

    // Statistics
    quint64 _totalBytesReceived = 0;
    quint64 _framesDropped = 0;
    qint64 _connectionStartTime = 0;
};
