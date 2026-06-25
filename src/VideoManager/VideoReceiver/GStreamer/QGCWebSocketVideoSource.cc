#include "QGCWebSocketVideoSource.h"

#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QSslError>
#include <QtWebSockets/QWebSocket>

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
    if (_config.hasAuthentication()) {
        request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    }
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
    _webSocket->setMaxAllowedIncomingFrameSize(kMaximumJpegBytes);
    _webSocket->setMaxAllowedIncomingMessageSize(kMaximumJpegBytes);
    _webSocket->setReadBufferSize(static_cast<qint64>(kMaximumJpegBytes));

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
    if (_webSocket) {
        _webSocket->disconnect(this);
        _webSocket->abort();
        delete _webSocket;
        _webSocket = nullptr;
    }
}

bool QGCWebSocketVideoSource::isCompleteJpeg(const QByteArray& message)
{
    return message.size() >= 4 && static_cast<quint8>(message.front()) == 0xFF &&
           static_cast<quint8>(message.at(1)) == 0xD8 && static_cast<quint8>(message.at(message.size() - 2)) == 0xFF &&
           static_cast<quint8>(message.back()) == 0xD9;
}

void QGCWebSocketVideoSource::_onConnected()
{
    qCDebug(QGCWebSocketVideoSourceLog) << "Connected" << QGCNetworkHelper::redactedUrlForLogging(_url);
    emit connected();
}

void QGCWebSocketVideoSource::_onDisconnected()
{
    qCDebug(QGCWebSocketVideoSourceLog) << "Disconnected" << QGCNetworkHelper::redactedUrlForLogging(_url);
    if (_running && _appsrc) {
        (void) gst_app_src_end_of_stream(GST_APP_SRC(_appsrc));
    }
    emit disconnected();
}

void QGCWebSocketVideoSource::_onBinaryMessageReceived(const QByteArray& message)
{
    if (!isCompleteJpeg(message)) {
        qCWarning(QGCWebSocketVideoSourceLog) << "Ignoring invalid JPEG message of" << message.size() << "bytes";
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
    if (_webSocket) {
        qCWarning(QGCWebSocketVideoSourceLog) << "WebSocket error code" << static_cast<int>(_webSocket->error());
    }
}

void QGCWebSocketVideoSource::_onSslErrors(const QList<QSslError>& errors)
{
    for (const QSslError& error : errors) {
        qCWarning(QGCWebSocketVideoSourceLog) << "TLS verification error code" << static_cast<int>(error.error());
    }
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
    if (result == GST_FLOW_OK) {
        emit jpegFramePushed(jpegData.size());
    } else if (result != GST_FLOW_FLUSHING) {
        qCWarning(QGCWebSocketVideoSourceLog) << "Failed to push JPEG buffer:" << result;
    }
}
