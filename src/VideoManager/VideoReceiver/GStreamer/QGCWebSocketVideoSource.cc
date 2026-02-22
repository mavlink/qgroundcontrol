#include "QGCWebSocketVideoSource.h"
#include "QGCLoggingCategory.h"

#include <QtWebSockets/QWebSocket>
#include <QtNetwork/QSslError>

QGC_LOGGING_CATEGORY(QGCWebSocketVideoSourceLog, "Video.QGCWebSocketVideoSource")

QGCWebSocketVideoSource::QGCWebSocketVideoSource(const QUrl &url, GstElement *appsrc, QObject *parent)
    : QObject(parent)
    , _url(url)
    , _appsrc(appsrc)
{
    qCDebug(QGCWebSocketVideoSourceLog) << "Created for URL:" << _url.toString();

    if (_appsrc) {
        gst_object_ref(_appsrc);
    }
}

QGCWebSocketVideoSource::~QGCWebSocketVideoSource()
{
    stop();

    _appsrcValid.storeRelaxed(0);
    if (_appsrc) {
        gst_object_unref(_appsrc);
        _appsrc = nullptr;
    }
}

void QGCWebSocketVideoSource::start()
{
    if (_running) {
        return;
    }

    _running = true;
    _framesReceived = 0;

    qCDebug(QGCWebSocketVideoSourceLog) << "Starting WebSocket connection to" << _url.toString();

    _webSocket = new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this);

    (void) connect(_webSocket, &QWebSocket::connected, this, &QGCWebSocketVideoSource::_onConnected);
    (void) connect(_webSocket, &QWebSocket::disconnected, this, &QGCWebSocketVideoSource::_onDisconnected);
    (void) connect(_webSocket, &QWebSocket::binaryMessageReceived, this, &QGCWebSocketVideoSource::_onBinaryMessageReceived);
    (void) connect(_webSocket, &QWebSocket::textMessageReceived, this, &QGCWebSocketVideoSource::_onTextMessageReceived);
    (void) connect(_webSocket, &QWebSocket::errorOccurred, this, &QGCWebSocketVideoSource::_onError);
    (void) connect(_webSocket, &QWebSocket::sslErrors, this, &QGCWebSocketVideoSource::_onSslErrors);

    (void) connect(&_heartbeatTimer, &QTimer::timeout, this, &QGCWebSocketVideoSource::_sendHeartbeat);
    (void) connect(&_connectionTimeoutTimer, &QTimer::timeout, this, &QGCWebSocketVideoSource::_checkConnectionTimeout);
    (void) connect(&_reconnectTimer, &QTimer::timeout, this, &QGCWebSocketVideoSource::_reconnect);

    _connectionTimeoutTimer.setSingleShot(true);
    _connectionTimeoutTimer.start(_timeoutSec * 1000);

    _webSocket->open(_url);
}

void QGCWebSocketVideoSource::stop()
{
    if (!_running) {
        return;
    }

    qCDebug(QGCWebSocketVideoSourceLog) << "Stopping. Frames received:" << _framesReceived;

    _running = false;
    _heartbeatTimer.stop();
    _connectionTimeoutTimer.stop();
    _reconnectTimer.stop();

    _cleanupWebSocket();
}

bool QGCWebSocketVideoSource::isConnected() const
{
    return _connected;
}

void QGCWebSocketVideoSource::setTimeout(uint32_t timeoutSec)
{
    _timeoutSec = timeoutSec;
}

void QGCWebSocketVideoSource::setReconnectDelay(uint32_t delayMs)
{
    _reconnectDelayMs = delayMs;
}

void QGCWebSocketVideoSource::setHeartbeatInterval(uint32_t intervalMs)
{
    _heartbeatIntervalMs = intervalMs;
}

void QGCWebSocketVideoSource::_onConnected()
{
    qCDebug(QGCWebSocketVideoSourceLog) << "Connected to" << _url.toString();

    _connected = true;
    _connectionTimeoutTimer.stop();

    _heartbeatTimer.start(_heartbeatIntervalMs);

    emit connected();
}

void QGCWebSocketVideoSource::_onDisconnected()
{
    qCDebug(QGCWebSocketVideoSourceLog) << "Disconnected from" << _url.toString();

    _connected = false;
    _heartbeatTimer.stop();
    _expectingBinaryFrame = false;

    emit disconnected();

    if (_running) {
        qCDebug(QGCWebSocketVideoSourceLog) << "Scheduling reconnect in" << _reconnectDelayMs << "ms";
        _reconnectTimer.setSingleShot(true);
        _reconnectTimer.start(_reconnectDelayMs);
    }
}

void QGCWebSocketVideoSource::_onBinaryMessageReceived(const QByteArray &message)
{
    if (message.isEmpty()) {
        return;
    }

    _pushFrameToAppsrc(message);
    _expectingBinaryFrame = false;
}

void QGCWebSocketVideoSource::_onTextMessageReceived(const QString &message)
{
    // Text messages are JSON metadata from PixEagle-compatible servers
    // Format: {"type":"frame","size":N,"quality":Q}
    if (message.contains(QStringLiteral("frame"))) {
        _expectingBinaryFrame = true;
    }
}

void QGCWebSocketVideoSource::_onError()
{
    if (!_webSocket) {
        return;
    }

    const QString errorString = _webSocket->errorString();
    qCWarning(QGCWebSocketVideoSourceLog) << "WebSocket error:" << errorString;
    emit errorOccurred(errorString);
}

void QGCWebSocketVideoSource::_onSslErrors(const QList<QSslError> &errors)
{
    for (const QSslError &error : errors) {
        qCWarning(QGCWebSocketVideoSourceLog) << "SSL error:" << error.errorString();
    }
}

void QGCWebSocketVideoSource::_sendHeartbeat()
{
    if (_webSocket && _connected) {
        _webSocket->ping();
    }
}

void QGCWebSocketVideoSource::_checkConnectionTimeout()
{
    if (!_connected && _running) {
        qCWarning(QGCWebSocketVideoSourceLog) << "Connection timeout after" << _timeoutSec << "seconds";
        _cleanupWebSocket();

        if (_running) {
            _reconnectTimer.setSingleShot(true);
            _reconnectTimer.start(_reconnectDelayMs);
        }
    }
}

void QGCWebSocketVideoSource::_reconnect()
{
    if (!_running) {
        return;
    }

    qCDebug(QGCWebSocketVideoSourceLog) << "Reconnecting to" << _url.toString();

    _cleanupWebSocket();

    _webSocket = new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this);

    (void) connect(_webSocket, &QWebSocket::connected, this, &QGCWebSocketVideoSource::_onConnected);
    (void) connect(_webSocket, &QWebSocket::disconnected, this, &QGCWebSocketVideoSource::_onDisconnected);
    (void) connect(_webSocket, &QWebSocket::binaryMessageReceived, this, &QGCWebSocketVideoSource::_onBinaryMessageReceived);
    (void) connect(_webSocket, &QWebSocket::textMessageReceived, this, &QGCWebSocketVideoSource::_onTextMessageReceived);
    (void) connect(_webSocket, &QWebSocket::errorOccurred, this, &QGCWebSocketVideoSource::_onError);
    (void) connect(_webSocket, &QWebSocket::sslErrors, this, &QGCWebSocketVideoSource::_onSslErrors);

    _connectionTimeoutTimer.setSingleShot(true);
    _connectionTimeoutTimer.start(_timeoutSec * 1000);

    _webSocket->open(_url);
}

void QGCWebSocketVideoSource::_pushFrameToAppsrc(const QByteArray &jpegData)
{
    if (!_appsrcValid.loadRelaxed() || !_appsrc) {
        return;
    }

    GstBuffer *buffer = gst_buffer_new_allocate(nullptr, jpegData.size(), nullptr);
    if (!buffer) {
        qCWarning(QGCWebSocketVideoSourceLog) << "Failed to allocate GstBuffer";
        return;
    }

    GstMapInfo map;
    if (gst_buffer_map(buffer, &map, GST_MAP_WRITE)) {
        memcpy(map.data, jpegData.constData(), jpegData.size());
        gst_buffer_unmap(buffer, &map);
    } else {
        qCWarning(QGCWebSocketVideoSourceLog) << "Failed to map GstBuffer";
        gst_buffer_unref(buffer);
        return;
    }

    GST_BUFFER_PTS(buffer) = gst_util_uint64_scale(_framesReceived, GST_SECOND, 30);
    GST_BUFFER_DTS(buffer) = GST_BUFFER_PTS(buffer);
    GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale(1, GST_SECOND, 30);

    const GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(_appsrc), buffer);
    if (ret != GST_FLOW_OK) {
        qCWarning(QGCWebSocketVideoSourceLog) << "Failed to push buffer to appsrc:" << ret;
    } else {
        _framesReceived++;
    }
}

void QGCWebSocketVideoSource::_cleanupWebSocket()
{
    _connected = false;
    _expectingBinaryFrame = false;

    if (_webSocket) {
        _webSocket->disconnect(this);
        if (_webSocket->state() != QAbstractSocket::UnconnectedState) {
            _webSocket->close();
        }
        _webSocket->deleteLater();
        _webSocket = nullptr;
    }
}
