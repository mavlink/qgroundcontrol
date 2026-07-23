#include "QGCWebSocketVideoSource.h"

#include <QtCore/QTimer>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QSslCertificate>
#include <QtNetwork/QSslConfiguration>
#include <QtNetwork/QSslError>
#include <QtWebSockets/QWebSocket>

#include "QGCJpegStreamGuard.h"
#include "QGCLoggingCategory.h"
#include "QGCNetworkHelper.h"
#include "SecureMemory.h"

QGC_LOGGING_CATEGORY(QGCWebSocketVideoSourceLog, "Video.GStreamer.WebSocketVideoSource")

QGCWebSocketVideoSource::QGCWebSocketVideoSource(const QUrl& url, const VideoReceiver::NetworkSourceConfig& config,
                                                 GstElement* appsrc, QObject* parent)
    : QObject(parent), _url(url), _config(config), _appsrc(appsrc)
{
    if (_appsrc) {
        gst_object_ref(_appsrc);
    }
}

QGCWebSocketVideoSource::~QGCWebSocketVideoSource()
{
    stop();
    _config.clearSecret();
    gst_clear_object(&_appsrc);
}

bool QGCWebSocketVideoSource::start(QString& error)
{
    error.clear();
    if (_running) {
        return true;
    }
    _protocolViolationReported = false;
    _transportErrorReported = false;
    _pendingError = PendingError::None;
    _pendingErrorDetail.clear();
    _pendingTransportErrorCode = 0;
    _pendingErrorPosted = false;
    _pendingErrorRetryScheduled = false;
    if (!_appsrc || !_url.isValid() ||
        (_url.scheme() != QStringLiteral("ws") && _url.scheme() != QStringLiteral("wss"))) {
        error = tr("Invalid WebSocket JPEG source.");
        return false;
    }
    if (_config.hasAuthentication() && _url.scheme() != QStringLiteral("wss")) {
        error = tr("Authenticated WebSocket video requires WSS.");
        return false;
    }
    if (!_config.caCertificateFile.isEmpty() && _url.scheme() != QStringLiteral("wss")) {
        error = tr("A custom CA certificate requires WSS.");
        return false;
    }

    QNetworkRequest request(_url);
    request.setRawHeader("User-Agent", QGCNetworkHelper::defaultUserAgent().toUtf8());
    switch (_config.authentication) {
        case VideoReceiver::NetworkSourceConfig::Authentication::Basic: {
            QByteArray credentials = _config.username.toUtf8() + QByteArrayLiteral(":") + _config.secret;
            QByteArray authorization = QByteArrayLiteral("Basic ") + credentials.toBase64();
            request.setRawHeader("Authorization", authorization);
            QGC::secureZero(authorization);
            QGC::secureZero(credentials);
            break;
        }
        case VideoReceiver::NetworkSourceConfig::Authentication::Bearer: {
            QByteArray authorization = QByteArrayLiteral("Bearer ") + _config.secret;
            request.setRawHeader("Authorization", authorization);
            QGC::secureZero(authorization);
            break;
        }
        case VideoReceiver::NetworkSourceConfig::Authentication::None:
            break;
    }

    _webSocket = new QWebSocket(_config.origin, QWebSocketProtocol::VersionLatest, this);
    _webSocket->setMaxAllowedIncomingFrameSize(static_cast<quint64>(QGCJpegStreamGuard::kMaximumEncodedBytes));
    _webSocket->setMaxAllowedIncomingMessageSize(static_cast<quint64>(QGCJpegStreamGuard::kMaximumEncodedBytes));
    _webSocket->setReadBufferSize(static_cast<qint64>(QGCJpegStreamGuard::kMaximumEncodedBytes));

    if (_url.scheme() == QStringLiteral("wss")) {
        QSslConfiguration sslConfiguration = QGCNetworkHelper::createSslConfig();
        if (!_config.caCertificateFile.isEmpty()) {
            const QList<QSslCertificate> certificates =
                QGCNetworkHelper::loadCaCertificates(_config.caCertificateFile, &error);
            if (certificates.isEmpty()) {
                delete _webSocket;
                _webSocket = nullptr;
                return false;
            }
            sslConfiguration.addCaCertificates(certificates);
        }
        _webSocket->setSslConfiguration(sslConfiguration);
    }

    (void) connect(_webSocket, &QWebSocket::connected, this, &QGCWebSocketVideoSource::_onConnected);
    (void) connect(_webSocket, &QWebSocket::disconnected, this, &QGCWebSocketVideoSource::_onDisconnected);
    (void) connect(_webSocket, &QWebSocket::binaryMessageReceived, this,
                   &QGCWebSocketVideoSource::_onBinaryMessageReceived);
    (void) connect(_webSocket, &QWebSocket::textMessageReceived, this,
                   &QGCWebSocketVideoSource::_onTextMessageReceived);
    (void) connect(_webSocket, &QWebSocket::errorOccurred, this, &QGCWebSocketVideoSource::_onError);
    (void) connect(_webSocket, &QWebSocket::sslErrors, this, &QGCWebSocketVideoSource::_onSslErrors);

    _running = true;
    qCDebug(QGCWebSocketVideoSourceLog) << "Opening" << QGCNetworkHelper::redactedUrlForLogging(_url);
    _webSocket->open(request);
    return true;
}

void QGCWebSocketVideoSource::stop()
{
    if (!_running && !_webSocket) {
        return;
    }

    _running = false;
    _pendingError = PendingError::None;
    _pendingErrorDetail.clear();
    _pendingErrorRetryScheduled = false;
    if (_webSocket) {
        _webSocket->disconnect(this);
        _webSocket->abort();
        delete _webSocket;
        _webSocket = nullptr;
    }
}

bool QGCWebSocketVideoSource::isCompleteJpeg(const QByteArray& message)
{
    return QGCJpegStreamGuard::validateJpeg(message);
}

void QGCWebSocketVideoSource::_onConnected()
{
    qCDebug(QGCWebSocketVideoSourceLog) << "Connected" << QGCNetworkHelper::redactedUrlForLogging(_url);
    emit connected();
}

void QGCWebSocketVideoSource::_onDisconnected()
{
    qCDebug(QGCWebSocketVideoSourceLog) << "Disconnected" << QGCNetworkHelper::redactedUrlForLogging(_url);
    const bool terminalErrorOwnsDisconnect =
        _transportErrorReported || _pendingError != PendingError::None || _pendingErrorPosted;
    if (_running && _appsrc && !terminalErrorOwnsDisconnect) {
        (void) gst_app_src_end_of_stream(GST_APP_SRC(_appsrc));
    }
    emit disconnected();
}

void QGCWebSocketVideoSource::_onBinaryMessageReceived(const QByteArray& message)
{
    if (_protocolViolationReported) {
        return;
    }

    QString validationError;
    if (!QGCJpegStreamGuard::validateJpeg(message, &validationError)) {
        _protocolViolationReported = true;
        _transportErrorReported = true;
        qCWarning(QGCWebSocketVideoSourceLog)
            << "Rejecting WebSocket JPEG stream after an invalid" << message.size() << "byte frame:" << validationError;
        _pendingError = PendingError::ProtocolViolation;
        _pendingErrorDetail = validationError;
        _postPendingErrorWhenAttached();
        if (_webSocket) {
            _webSocket->close(QWebSocketProtocol::CloseCodeProtocolError, tr("Invalid JPEG frame"));
        }
        return;
    }
    _pushFrameToAppsrc(message);
}

void QGCWebSocketVideoSource::_onTextMessageReceived(const QString& message)
{
    qCDebug(QGCWebSocketVideoSourceLog) << "Ignoring WebSocket text message of" << message.size() << "characters";
}

void QGCWebSocketVideoSource::_onError()
{
    if (!_webSocket || _transportErrorReported) {
        return;
    }
    _transportErrorReported = true;
    const int errorCode = static_cast<int>(_webSocket->error());
    qCWarning(QGCWebSocketVideoSourceLog) << "WebSocket error code" << errorCode;
    _pendingError = PendingError::Transport;
    _pendingTransportErrorCode = errorCode;
    _postPendingErrorWhenAttached();
}

void QGCWebSocketVideoSource::_onSslErrors(const QList<QSslError>& errors)
{
    if (!errors.isEmpty()) {
        qCWarning(QGCWebSocketVideoSourceLog)
            << "TLS verification failed with" << errors.size() << "error(s); first code"
            << static_cast<int>(errors.constFirst().error());
    }
}

bool QGCWebSocketVideoSource::_appsrcHasReadyPipelineAncestor() const
{
    if (!_appsrc) {
        return false;
    }

    GstObject* current = GST_OBJECT(gst_object_ref(_appsrc));
    bool foundPipeline = false;
    while (current) {
        if (GST_IS_PIPELINE(current)) {
            GstState state = GST_STATE_NULL;
            GstState pending = GST_STATE_VOID_PENDING;
            (void) gst_element_get_state(GST_ELEMENT(current), &state, &pending, 0);
            foundPipeline = state >= GST_STATE_READY || pending >= GST_STATE_READY;
            gst_object_unref(current);
            break;
        }
        GstObject* parent = gst_object_get_parent(current);
        gst_object_unref(current);
        current = parent;
    }
    return foundPipeline;
}

void QGCWebSocketVideoSource::_postPendingErrorWhenAttached()
{
    if (!_running || !_appsrc || _pendingError == PendingError::None || _pendingErrorPosted) {
        return;
    }
    if (!_appsrcHasReadyPipelineAncestor()) {
        if (!_pendingErrorRetryScheduled) {
            _pendingErrorRetryScheduled = true;
            QTimer::singleShot(10, this, [this]() {
                _pendingErrorRetryScheduled = false;
                _postPendingErrorWhenAttached();
            });
        }
        return;
    }

    _pendingErrorPosted = true;
    switch (_pendingError) {
        case PendingError::ProtocolViolation: {
            const QByteArray detail = _pendingErrorDetail.toUtf8();
            GST_ELEMENT_ERROR(_appsrc, STREAM, DECODE, ("WebSocket JPEG stream was rejected"),
                              ("%s", detail.constData()));
            break;
        }
        case PendingError::Transport:
            GST_ELEMENT_ERROR(_appsrc, RESOURCE, OPEN_READ, ("WebSocket video connection failed"),
                              ("Socket error code %d", _pendingTransportErrorCode));
            break;
        case PendingError::None:
            break;
    }
    _pendingError = PendingError::None;
    _pendingErrorDetail.clear();
}

void QGCWebSocketVideoSource::_pushFrameToAppsrc(const QByteArray& jpegData)
{
    if (!_running || !_appsrc) {
        return;
    }

    GstBuffer* buffer = gst_buffer_new_allocate(nullptr, static_cast<gsize>(jpegData.size()), nullptr);
    if (!buffer) {
        qCWarning(QGCWebSocketVideoSourceLog) << "Failed to allocate JPEG buffer";
        return;
    }

    gst_buffer_fill(buffer, 0, jpegData.constData(), static_cast<gsize>(jpegData.size()));
    GST_BUFFER_PTS(buffer) = GST_CLOCK_TIME_NONE;
    GST_BUFFER_DTS(buffer) = GST_CLOCK_TIME_NONE;
    GST_BUFFER_DURATION(buffer) = GST_CLOCK_TIME_NONE;

    const GstFlowReturn result = gst_app_src_push_buffer(GST_APP_SRC(_appsrc), buffer);
    if (result != GST_FLOW_OK && result != GST_FLOW_FLUSHING) {
        qCWarning(QGCWebSocketVideoSourceLog) << "Failed to push JPEG buffer:" << result;
    }
}
