/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "QGCWebSocketVideoSource.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QUrl>
#include <QtCore/QThread>

QGC_LOGGING_CATEGORY(WebSocketVideoLog, "qgc.videomanager.websocket")

QGCWebSocketVideoSource::QGCWebSocketVideoSource(
    const QString &url,
    int timeout,
    int reconnectDelay,
    int heartbeatInterval,
    int minQuality,
    int maxQuality,
    bool adaptiveQuality,
    QObject *parent)
    : QObject(parent)
    , _url(url)
    , _timeout(timeout)
    , _reconnectDelay(reconnectDelay)
    , _heartbeatInterval(heartbeatInterval)
    , _minQuality(minQuality)
    , _maxQuality(maxQuality)
    , _adaptiveQuality(adaptiveQuality)
{
    qCDebug(WebSocketVideoLog) << "Creating WebSocket video source:" << url;

    // Create WebSocket
    _webSocket = new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this);

    // Connect WebSocket signals
    connect(_webSocket, &QWebSocket::connected, this, &QGCWebSocketVideoSource::onConnected);
    connect(_webSocket, &QWebSocket::disconnected, this, &QGCWebSocketVideoSource::onDisconnected);
    connect(_webSocket, &QWebSocket::textMessageReceived, this, &QGCWebSocketVideoSource::onTextMessageReceived);
    connect(_webSocket, &QWebSocket::binaryMessageReceived, this, &QGCWebSocketVideoSource::onBinaryMessageReceived);
    connect(_webSocket, &QWebSocket::errorOccurred, this, &QGCWebSocketVideoSource::onError);
    connect(_webSocket, &QWebSocket::sslErrors, this, &QGCWebSocketVideoSource::onSslErrors);

    // Create heartbeat timer
    _heartbeatTimer = new QTimer(this);
    _heartbeatTimer->setInterval(_heartbeatInterval);
    connect(_heartbeatTimer, &QTimer::timeout, this, &QGCWebSocketVideoSource::onHeartbeatTimer);

    // Create reconnect timer
    _reconnectTimer = new QTimer(this);
    _reconnectTimer->setSingleShot(true);
    connect(_reconnectTimer, &QTimer::timeout, this, &QGCWebSocketVideoSource::onReconnectTimer);

    // Create GStreamer appsrc element
    createAppsrcElement();
}

QGCWebSocketVideoSource::~QGCWebSocketVideoSource()
{
    qCDebug(WebSocketVideoLog) << "Destroying WebSocket video source";
    stop();
    cleanupAppsrc();
}

void QGCWebSocketVideoSource::createAppsrcElement()
{
    _appsrc = gst_element_factory_make("appsrc", "websocket_appsrc");
    if (!_appsrc) {
        qCCritical(WebSocketVideoLog) << "Failed to create appsrc element";
        return;
    }

    // Configure appsrc for live streaming
    g_object_set(_appsrc,
                 "is-live", TRUE,
                 "format", GST_FORMAT_TIME,
                 "do-timestamp", TRUE,
                 "min-latency", (gint64)0,
                 "max-bytes", (guint64)(1024 * 1024),  // 1MB max queue
                 "block", FALSE,
                 "stream-type", GST_APP_STREAM_TYPE_STREAM,
                 nullptr);

    // Set caps for JPEG frames
    GstCaps *caps = gst_caps_new_simple("image/jpeg",
                                        "framerate", GST_TYPE_FRACTION, 30, 1,
                                        nullptr);
    g_object_set(_appsrc, "caps", caps, nullptr);
    gst_caps_unref(caps);

    // Increase reference count so it's not destroyed when removed from bin
    gst_object_ref(_appsrc);

    qCDebug(WebSocketVideoLog) << "appsrc element created successfully";
}

void QGCWebSocketVideoSource::start()
{
    if (_connected) {
        qCDebug(WebSocketVideoLog) << "Already connected";
        return;
    }

    qCDebug(WebSocketVideoLog) << "Starting WebSocket connection to:" << _url;
    _shouldReconnect = true;
    _connectionStartTime = QDateTime::currentMSecsSinceEpoch();

    emit stateChanged("Connecting");
    _webSocket->open(QUrl(_url));
}

void QGCWebSocketVideoSource::stop()
{
    qCDebug(WebSocketVideoLog) << "Stopping WebSocket connection";
    _shouldReconnect = false;

    _heartbeatTimer->stop();
    _reconnectTimer->stop();

    if (_webSocket->state() == QAbstractSocket::ConnectedState) {
        _webSocket->close();
    }

    // Send EOS to appsrc
    if (_appsrc) {
        gst_app_src_end_of_stream(GST_APP_SRC(_appsrc));
    }

    emit stateChanged("Stopped");
}

void QGCWebSocketVideoSource::setQuality(int quality)
{
    if (quality < _minQuality || quality > _maxQuality) {
        qCWarning(WebSocketVideoLog) << "Quality out of range:" << quality;
        return;
    }

    if (_currentQuality != quality) {
        sendQualityRequest(quality);
    }
}

void QGCWebSocketVideoSource::onConnected()
{
    qCDebug(WebSocketVideoLog) << "WebSocket connected successfully";
    _connected = true;
    _frameCount = 0;
    _totalBytesReceived = 0;
    _framesDropped = 0;

    emit connected();
    emit stateChanged("Connected");

    // Start heartbeat
    _heartbeatTimer->start();

    // Stop reconnect attempts
    _reconnectTimer->stop();
}

void QGCWebSocketVideoSource::onDisconnected()
{
    qCDebug(WebSocketVideoLog) << "WebSocket disconnected";
    _connected = false;
    _expectingBinaryFrame = false;

    emit disconnected();
    emit stateChanged("Disconnected");

    _heartbeatTimer->stop();

    // Schedule reconnection if needed
    if (_shouldReconnect) {
        scheduleReconnect();
    }
}

void QGCWebSocketVideoSource::onTextMessageReceived(const QString &message)
{
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (doc.isNull() || !doc.isObject()) {
        qCWarning(WebSocketVideoLog) << "Invalid JSON message:" << message;
        return;
    }

    QJsonObject obj = doc.object();
    QString type = obj["type"].toString();

    if (type == "frame") {
        // Frame metadata - binary frame will follow
        _expectingBinaryFrame = true;
        _expectedFrameSize = obj["size"].toInt();

        int quality = obj["quality"].toInt();
        if (quality > 0 && quality != _currentQuality) {
            _currentQuality = quality;
            emit qualityChanged(_currentQuality);
        }

        qCDebug(WebSocketVideoLog) << "Frame metadata: size=" << _expectedFrameSize
                                    << "quality=" << _currentQuality;

    } else if (type == "pong") {
        // Heartbeat response
        qCDebug(WebSocketVideoLog) << "Heartbeat acknowledged";

    } else if (type == "error") {
        QString errorMsg = obj["message"].toString();
        qCWarning(WebSocketVideoLog) << "Server error:" << errorMsg;
        emit error(errorMsg);

    } else {
        qCDebug(WebSocketVideoLog) << "Unknown message type:" << type;
    }
}

void QGCWebSocketVideoSource::onBinaryMessageReceived(const QByteArray &message)
{
    if (!_expectingBinaryFrame) {
        qCWarning(WebSocketVideoLog) << "Unexpected binary message, size:" << message.size();
        _framesDropped++;
        return;
    }

    _expectingBinaryFrame = false;
    _lastFrameTime = QDateTime::currentMSecsSinceEpoch();
    _frameCount++;
    _totalBytesReceived += message.size();

    qCDebug(WebSocketVideoLog) << "Frame received: size=" << message.size()
                                << "frame#" << _frameCount;

    // Push frame to GStreamer
    pushFrameToAppsrc(message);

    // Update statistics
    emit frameReceived(message.size());
    updateBandwidthEstimate(message.size());
}

void QGCWebSocketVideoSource::onError(QAbstractSocket::SocketError socketError)
{
    QString errorString = _webSocket->errorString();
    qCWarning(WebSocketVideoLog) << "WebSocket error:" << socketError << errorString;

    emit error(errorString);
    emit stateChanged("Error");

    // Trigger reconnection
    if (_shouldReconnect && !_reconnectTimer->isActive()) {
        scheduleReconnect();
    }
}

void QGCWebSocketVideoSource::onSslErrors(const QList<QSslError> &errors)
{
    for (const QSslError &sslError : errors) {
        qCWarning(WebSocketVideoLog) << "SSL Error:" << sslError.errorString();
    }

    // For development/testing, you might want to ignore SSL errors
    // In production, proper certificate validation should be enforced
    // _webSocket->ignoreSslErrors();
}

void QGCWebSocketVideoSource::onHeartbeatTimer()
{
    if (_connected) {
        sendHeartbeat();

        // Check for stale connection (no frames in 3x heartbeat interval)
        qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
        if (_lastFrameTime > 0 && (currentTime - _lastFrameTime) > _heartbeatInterval * 3) {
            qCWarning(WebSocketVideoLog) << "Connection stale, no frames in"
                                         << (currentTime - _lastFrameTime) << "ms";
            _webSocket->close();
        }
    }
}

void QGCWebSocketVideoSource::onReconnectTimer()
{
    if (_shouldReconnect && !_connected) {
        qCDebug(WebSocketVideoLog) << "Attempting reconnection...";
        emit stateChanged("Reconnecting");
        _webSocket->open(QUrl(_url));
    }
}

void QGCWebSocketVideoSource::pushFrameToAppsrc(const QByteArray &frameData)
{
    if (!_appsrc) {
        qCWarning(WebSocketVideoLog) << "appsrc is null, cannot push frame";
        return;
    }

    // Create GStreamer buffer
    GstBuffer *buffer = gst_buffer_new_allocate(nullptr, frameData.size(), nullptr);
    if (!buffer) {
        qCWarning(WebSocketVideoLog) << "Failed to allocate GstBuffer";
        _framesDropped++;
        return;
    }

    // Copy frame data into buffer
    GstMapInfo map;
    if (!gst_buffer_map(buffer, &map, GST_MAP_WRITE)) {
        qCWarning(WebSocketVideoLog) << "Failed to map GstBuffer";
        gst_buffer_unref(buffer);
        _framesDropped++;
        return;
    }

    memcpy(map.data, frameData.constData(), frameData.size());
    gst_buffer_unmap(buffer, &map);

    // Let appsrc handle timestamps automatically (do-timestamp=TRUE)
    // GStreamer will generate proper relative timestamps for live stream
    GST_BUFFER_PTS(buffer) = GST_CLOCK_TIME_NONE;
    GST_BUFFER_DURATION(buffer) = GST_CLOCK_TIME_NONE;

    // Push to appsrc
    GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(_appsrc), buffer);
    if (ret != GST_FLOW_OK) {
        qCWarning(WebSocketVideoLog) << "Failed to push buffer to appsrc, ret:" << ret;
        _framesDropped++;
    }
}

void QGCWebSocketVideoSource::sendQualityRequest(int quality)
{
    if (!_connected) return;

    QJsonObject obj;
    obj["type"] = "quality";
    obj["quality"] = quality;

    QJsonDocument doc(obj);
    _webSocket->sendTextMessage(doc.toJson(QJsonDocument::Compact));

    qCDebug(WebSocketVideoLog) << "Quality request sent:" << quality;
}

void QGCWebSocketVideoSource::sendHeartbeat()
{
    if (!_connected) return;

    QJsonObject obj;
    obj["type"] = "ping";
    obj["timestamp"] = QDateTime::currentMSecsSinceEpoch();

    QJsonDocument doc(obj);
    _webSocket->sendTextMessage(doc.toJson(QJsonDocument::Compact));

    qCDebug(WebSocketVideoLog) << "Heartbeat sent";
}

void QGCWebSocketVideoSource::updateBandwidthEstimate(int frameSize)
{
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();

    // Add to history
    _frameHistory.enqueue(qMakePair(currentTime, frameSize));

    // Keep only recent frames
    while (_frameHistory.size() > MAX_FRAME_HISTORY) {
        _frameHistory.dequeue();
    }

    // Calculate bandwidth over the frame history window
    if (_frameHistory.size() >= 10) {
        qint64 oldestTime = _frameHistory.first().first;
        qint64 timeSpan = currentTime - oldestTime;

        if (timeSpan > 0) {
            int totalBytes = 0;
            for (const auto &frame : _frameHistory) {
                totalBytes += frame.second;
            }

            _bandwidthBytesPerSecond = (totalBytes * 1000.0) / timeSpan;
            emit bandwidthUpdated(_bandwidthBytesPerSecond);

            qCDebug(WebSocketVideoLog) << "Bandwidth:" << (_bandwidthBytesPerSecond / 1024.0)
                                        << "KB/s, Quality:" << _currentQuality;

            // Adaptive quality adjustment
            if (_adaptiveQuality) {
                qreal targetBytesPerFrame = _bandwidthBytesPerSecond / 30.0;  // Assume 30fps

                if (targetBytesPerFrame < 10000) {  // < 10KB per frame
                    int newQuality = qMax(_minQuality, _currentQuality - 5);
                    if (newQuality != _currentQuality) {
                        qCDebug(WebSocketVideoLog) << "Reducing quality due to low bandwidth:"
                                                    << _currentQuality << "→" << newQuality;
                        sendQualityRequest(newQuality);
                    }
                } else if (targetBytesPerFrame > 50000) {  // > 50KB per frame
                    int newQuality = qMin(_maxQuality, _currentQuality + 5);
                    if (newQuality != _currentQuality) {
                        qCDebug(WebSocketVideoLog) << "Increasing quality due to high bandwidth:"
                                                    << _currentQuality << "→" << newQuality;
                        sendQualityRequest(newQuality);
                    }
                }
            }
        }
    }
}

void QGCWebSocketVideoSource::scheduleReconnect()
{
    if (!_shouldReconnect) return;

    qCDebug(WebSocketVideoLog) << "Scheduling reconnection in" << _reconnectDelay << "ms";
    emit stateChanged("Waiting to reconnect");
    _reconnectTimer->start(_reconnectDelay);
}

void QGCWebSocketVideoSource::cleanupAppsrc()
{
    if (_appsrc) {
        gst_app_src_end_of_stream(GST_APP_SRC(_appsrc));
        gst_object_unref(_appsrc);
        _appsrc = nullptr;
    }
}
