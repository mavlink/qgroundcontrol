#include "QGCNetworkHelper.h"

#include <QtBluetooth/QBluetoothLocalDevice>
#include <QtCore/QFile>
#include <QtCore/QIODevice>
#include <QtCore/QJsonDocument>
#include <QtCore/QUrlQuery>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QHttpHeaders>
#include <QtNetwork/QHttpPart>
#include <QtNetwork/QNetworkInformation>
#include <QtNetwork/QNetworkProxyFactory>
#include <QtNetwork/QSslSocket>
#include <chrono>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(QGCNetworkHelperLog, "Utilities.QGCNetworkHelper")

namespace QGCNetworkHelper {

// ============================================================================
// HTTP Status Code Helpers
// ============================================================================

HttpStatusClass classifyHttpStatus(int statusCode)
{
    const auto code = static_cast<HttpStatusCode>(statusCode);
    if (code >= HttpStatusCode::Continue && code < HttpStatusCode::Ok) {
        return HttpStatusClass::Informational;
    }
    if (code >= HttpStatusCode::Ok && code < HttpStatusCode::MultipleChoices) {
        return HttpStatusClass::Success;
    }
    if (code >= HttpStatusCode::MultipleChoices && code < HttpStatusCode::BadRequest) {
        return HttpStatusClass::Redirection;
    }
    if (code >= HttpStatusCode::BadRequest && code < HttpStatusCode::InternalServerError) {
        return HttpStatusClass::ClientError;
    }
    if (code >= HttpStatusCode::InternalServerError && code <= HttpStatusCode::NetworkConnectTimeoutError) {
        return HttpStatusClass::ServerError;
    }
    return HttpStatusClass::Unknown;
}

namespace {

struct HttpStatusName
{
    HttpStatusCode code;
    const char* text;
};

constexpr HttpStatusName kHttpStatusNames[] = {
    // 1xx Informational
    {HttpStatusCode::Continue, "Continue"},
    {HttpStatusCode::SwitchingProtocols, "Switching Protocols"},
    {HttpStatusCode::Processing, "Processing"},
    // 2xx Success
    {HttpStatusCode::Ok, "OK"},
    {HttpStatusCode::Created, "Created"},
    {HttpStatusCode::Accepted, "Accepted"},
    {HttpStatusCode::NonAuthoritativeInformation, "Non-Authoritative Information"},
    {HttpStatusCode::NoContent, "No Content"},
    {HttpStatusCode::ResetContent, "Reset Content"},
    {HttpStatusCode::PartialContent, "Partial Content"},
    {HttpStatusCode::MultiStatus, "Multi-Status"},
    {HttpStatusCode::AlreadyReported, "Already Reported"},
    {HttpStatusCode::IMUsed, "IM Used"},
    // 3xx Redirection
    {HttpStatusCode::MultipleChoices, "Multiple Choices"},
    {HttpStatusCode::MovedPermanently, "Moved Permanently"},
    {HttpStatusCode::Found, "Found"},
    {HttpStatusCode::SeeOther, "See Other"},
    {HttpStatusCode::NotModified, "Not Modified"},
    {HttpStatusCode::UseProxy, "Use Proxy"},
    {HttpStatusCode::TemporaryRedirect, "Temporary Redirect"},
    {HttpStatusCode::PermanentRedirect, "Permanent Redirect"},
    // 4xx Client Errors
    {HttpStatusCode::BadRequest, "Bad Request"},
    {HttpStatusCode::Unauthorized, "Unauthorized"},
    {HttpStatusCode::PaymentRequired, "Payment Required"},
    {HttpStatusCode::Forbidden, "Forbidden"},
    {HttpStatusCode::NotFound, "Not Found"},
    {HttpStatusCode::MethodNotAllowed, "Method Not Allowed"},
    {HttpStatusCode::NotAcceptable, "Not Acceptable"},
    {HttpStatusCode::ProxyAuthenticationRequired, "Proxy Authentication Required"},
    {HttpStatusCode::RequestTimeout, "Request Timeout"},
    {HttpStatusCode::Conflict, "Conflict"},
    {HttpStatusCode::Gone, "Gone"},
    {HttpStatusCode::LengthRequired, "Length Required"},
    {HttpStatusCode::PreconditionFailed, "Precondition Failed"},
    {HttpStatusCode::PayloadTooLarge, "Payload Too Large"},
    {HttpStatusCode::UriTooLong, "URI Too Long"},
    {HttpStatusCode::UnsupportedMediaType, "Unsupported Media Type"},
    {HttpStatusCode::RequestRangeNotSatisfiable, "Range Not Satisfiable"},
    {HttpStatusCode::ExpectationFailed, "Expectation Failed"},
    {HttpStatusCode::ImATeapot, "I'm a teapot"},
    {HttpStatusCode::MisdirectedRequest, "Misdirected Request"},
    {HttpStatusCode::UnprocessableEntity, "Unprocessable Entity"},
    {HttpStatusCode::Locked, "Locked"},
    {HttpStatusCode::FailedDependency, "Failed Dependency"},
    {HttpStatusCode::UpgradeRequired, "Upgrade Required"},
    {HttpStatusCode::PreconditionRequired, "Precondition Required"},
    {HttpStatusCode::TooManyRequests, "Too Many Requests"},
    {HttpStatusCode::RequestHeaderFieldsTooLarge, "Request Header Fields Too Large"},
    {HttpStatusCode::UnavailableForLegalReasons, "Unavailable For Legal Reasons"},
    // 5xx Server Errors
    {HttpStatusCode::InternalServerError, "Internal Server Error"},
    {HttpStatusCode::NotImplemented, "Not Implemented"},
    {HttpStatusCode::BadGateway, "Bad Gateway"},
    {HttpStatusCode::ServiceUnavailable, "Service Unavailable"},
    {HttpStatusCode::GatewayTimeout, "Gateway Timeout"},
    {HttpStatusCode::HttpVersionNotSupported, "HTTP Version Not Supported"},
    {HttpStatusCode::VariantAlsoNegotiates, "Variant Also Negotiates"},
    {HttpStatusCode::InsufficientStorage, "Insufficient Storage"},
    {HttpStatusCode::LoopDetected, "Loop Detected"},
    {HttpStatusCode::NotExtended, "Not Extended"},
    {HttpStatusCode::NetworkAuthenticationRequired, "Network Authentication Required"},
    {HttpStatusCode::NetworkConnectTimeoutError, "Network Connect Timeout Error"},
};

}  // namespace

QString httpStatusText(HttpStatusCode statusCode)
{
    for (const HttpStatusName& entry : kHttpStatusNames) {
        if (entry.code == statusCode) {
            return QString::fromLatin1(entry.text);
        }
    }
    return QStringLiteral("Unknown Status (%1)").arg(static_cast<int>(statusCode));
}

QString httpStatusText(int statusCode)
{
    return httpStatusText(static_cast<HttpStatusCode>(statusCode));
}

// ============================================================================
// HTTP Methods
// ============================================================================

namespace {

struct HttpMethodEntry
{
    HttpMethod method;
    const char* name;
};

constexpr HttpMethodEntry kHttpMethods[] = {
    {HttpMethod::Get, "GET"},       {HttpMethod::Post, "POST"},       {HttpMethod::Put, "PUT"},
    {HttpMethod::Delete, "DELETE"}, {HttpMethod::Head, "HEAD"},       {HttpMethod::Options, "OPTIONS"},
    {HttpMethod::Patch, "PATCH"},   {HttpMethod::Connect, "CONNECT"}, {HttpMethod::Trace, "TRACE"},
};

}  // namespace

QString httpMethodName(HttpMethod method)
{
    for (const HttpMethodEntry& entry : kHttpMethods) {
        if (entry.method == method) {
            return QString::fromLatin1(entry.name);
        }
    }
    return QStringLiteral("GET");
}

HttpMethod parseHttpMethod(const QString& methodStr)
{
    const QByteArray upper = methodStr.toUpper().toLatin1();
    for (const HttpMethodEntry& entry : kHttpMethods) {
        if (qstrcmp(upper.constData(), entry.name) == 0) {
            return entry.method;
        }
    }
    return HttpMethod::Get;
}

// ============================================================================
// URL Utilities
// ============================================================================

bool isValidUrl(const QUrl& url)
{
    if (!url.isValid()) {
        return false;
    }

    const QString scheme = url.scheme().toLower();
    return scheme == QLatin1String("http") || scheme == QLatin1String("https") || scheme == QLatin1String("file") ||
           scheme == QLatin1String("qrc") || scheme.isEmpty();  // Relative URL
}

bool isHttpUrl(const QUrl& url)
{
    const QString scheme = url.scheme().toLower();
    return scheme == QLatin1String("http") || scheme == QLatin1String("https");
}

bool isHttpsUrl(const QUrl& url)
{
    return url.scheme().toLower() == QLatin1String("https");
}

QUrl normalizeUrl(const QUrl& url)
{
    if (!url.isValid()) {
        return url;
    }

    QUrl normalized = url.adjusted(QUrl::NormalizePathSegments | QUrl::StripTrailingSlash);
    normalized.setScheme(normalized.scheme().toLower());
    normalized.setHost(normalized.host().toLower());

    const int port = normalized.port();
    const QString scheme = normalized.scheme();
    if ((scheme == QLatin1String("http") && port == 80) || (scheme == QLatin1String("https") && port == 443) ||
        (scheme == QLatin1String("ftp") && port == 21)) {
        normalized.setPort(-1);
    }

    return normalized;
}

QUrl ensureScheme(const QUrl& url, const QString& defaultScheme)
{
    if (!url.isValid()) {
        return url;
    }

    if (url.scheme().isEmpty()) {
        QUrl withScheme = url;
        withScheme.setScheme(defaultScheme);
        return withScheme;
    }

    return url;
}

QUrl buildUrl(const QString& baseUrl, const QMap<QString, QString>& params)
{
    QList<QPair<QString, QString>> items;
    for (auto it = params.constBegin(); it != params.constEnd(); ++it) {
        items.append({it.key(), it.value()});
    }
    return buildUrl(baseUrl, items);
}

QUrl buildUrl(const QString& baseUrl, const QList<QPair<QString, QString>>& params)
{
    QUrl url(baseUrl);
    if (!url.isValid()) {
        return url;
    }

    QUrlQuery query;
    for (const auto& [key, value] : params) {
        query.addQueryItem(key, value);
    }
    url.setQuery(query);

    return url;
}

QUrl urlWithoutQuery(const QUrl& url)
{
    return url.adjusted(QUrl::RemoveQuery | QUrl::RemoveFragment);
}

namespace {
enum class HostPortClassification
{
    NotHostPort,
    Valid,
    Invalid,
};

bool isAsciiAlphaNumeric(char character)
{
    return ((character >= 'a') && (character <= 'z')) || ((character >= 'A') && (character <= 'Z')) ||
           ((character >= '0') && (character <= '9'));
}

bool isValidHostname(QString hostname)
{
    if (hostname.endsWith(QLatin1Char('.'))) {
        hostname.chop(1);
    }

    const QByteArray aceHostname = QUrl::toAce(hostname);
    if (aceHostname.isEmpty() || (aceHostname.size() > 253)) {
        return false;
    }

    const QList<QByteArray> labels = aceHostname.split('.');
    for (const QByteArray& label : labels) {
        if (label.isEmpty() || (label.size() > 63) || !isAsciiAlphaNumeric(label.front()) ||
            !isAsciiAlphaNumeric(label.back())) {
            return false;
        }
        for (const char character : label) {
            if (!isAsciiAlphaNumeric(character) && (character != '-')) {
                return false;
            }
        }
    }
    return true;
}

HostPortClassification classifyHostPort(const QString& value)
{
    if (value.contains(QStringLiteral("://"))) {
        return HostPortClassification::NotHostPort;
    }

    const qsizetype separator = value.lastIndexOf(QLatin1Char(':'));
    if ((separator <= 0) || (separator == (value.size() - 1))) {
        return HostPortClassification::NotHostPort;
    }

    const QString portText = value.sliced(separator + 1);
    for (const QChar character : portText) {
        if (!character.isDigit()) {
            return HostPortClassification::NotHostPort;
        }
    }

    bool portOk = false;
    const int port = portText.toInt(&portOk);
    if (!portOk || (port < 1) || (port > 65535)) {
        return HostPortClassification::Invalid;
    }

    const QUrl authority(QStringLiteral("qgc://") + value, QUrl::StrictMode);
    if (!authority.isValid() || authority.host().isEmpty() || !authority.userInfo().isEmpty() ||
        !authority.path().isEmpty() || authority.hasQuery() || authority.hasFragment() ||
        (authority.port(-1) != port)) {
        return HostPortClassification::Invalid;
    }

    QHostAddress address;
    const bool validHost = address.setAddress(authority.host()) || isValidHostname(authority.host());
    return validHost ? HostPortClassification::Valid : HostPortClassification::Invalid;
}
}  // namespace

QString redactedUrlForLogging(const QUrl& url)
{
    if (url.isEmpty()) {
        return QStringLiteral("<empty-url>");
    }

    const QString sourceText = url.isValid() ? url.toString(QUrl::FullyEncoded) : url.path();
    const HostPortClassification hostPort = classifyHostPort(sourceText);
    if (hostPort == HostPortClassification::Valid) {
        return sourceText;
    }
    if (hostPort == HostPortClassification::Invalid) {
        return QStringLiteral("<invalid-url length=%1>").arg(sourceText.size());
    }
    if (!url.isValid()) {
        return QStringLiteral("<invalid-url>");
    }

    QUrl redactedUrl = url.adjusted(QUrl::RemoveUserInfo);
    if (redactedUrl.hasQuery()) {
        const auto queryItems = QUrlQuery(redactedUrl).queryItems(QUrl::FullyDecoded);
        QUrlQuery redactedQuery;
        for (const auto& queryItem : queryItems) {
            redactedQuery.addQueryItem(queryItem.first, QStringLiteral("REDACTED"));
        }
        if (queryItems.isEmpty()) {
            redactedUrl.setQuery(QString());
        } else {
            redactedUrl.setQuery(redactedQuery);
        }
    }
    if (redactedUrl.hasFragment()) {
        redactedUrl.setFragment(QStringLiteral("REDACTED"));
    }

    return redactedUrl.toDisplayString(QUrl::FullyEncoded);
}

QString redactedUrlForLogging(const QString& url)
{
    const HostPortClassification hostPort = classifyHostPort(url);
    if (hostPort == HostPortClassification::Valid) {
        return url;
    }
    if (hostPort == HostPortClassification::Invalid) {
        return QStringLiteral("<invalid-url length=%1>").arg(url.size());
    }
    const QUrl parsedUrl(url);
    if (!url.isEmpty() && !parsedUrl.isValid()) {
        return QStringLiteral("<invalid-url length=%1>").arg(url.size());
    }
    return redactedUrlForLogging(parsedUrl);
}

// ============================================================================
// Request Configuration
// ============================================================================

void configureRequest(QNetworkRequest& request, const RequestConfig& config)
{
    request.setTransferTimeout(config.timeoutMs);

    const auto redirectPolicy =
        config.allowRedirects ? QNetworkRequest::NoLessSafeRedirectPolicy : QNetworkRequest::ManualRedirectPolicy;
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, redirectPolicy);

    request.setAttribute(QNetworkRequest::Http2AllowedAttribute, config.http2Allowed);

    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute,
                         config.cacheEnabled ? QNetworkRequest::PreferCache : QNetworkRequest::AlwaysNetwork);
    request.setAttribute(QNetworkRequest::CacheSaveControlAttribute, config.cacheEnabled);

    request.setAttribute(QNetworkRequest::BackgroundRequestAttribute, config.backgroundRequest);

    using WK = QHttpHeaders::WellKnownHeader;
    QHttpHeaders headers = request.headers();
    headers.replaceOrAppend(WK::UserAgent, config.userAgent.isEmpty() ? defaultUserAgent() : config.userAgent);
    if (!config.accept.isEmpty()) {
        headers.replaceOrAppend(WK::Accept, config.accept);
    }
    if (!config.acceptEncoding.isEmpty()) {
        headers.replaceOrAppend(WK::AcceptEncoding, config.acceptEncoding);
    }
    if (!config.contentType.isEmpty()) {
        headers.replaceOrAppend(WK::ContentType, config.contentType);
    }
    headers.replaceOrAppend(WK::Connection, "keep-alive");
    request.setHeaders(headers);

    for (const auto& [attribute, value] : config.requestAttributes) {
        request.setAttribute(attribute, value);
    }

    if (config.connectionCacheExpirySecs >= 0) {
        request.setAttribute(QNetworkRequest::ConnectionCacheExpiryTimeoutSecondsAttribute,
                             config.connectionCacheExpirySecs);
    }
    if (config.tcpKeepAliveIdleSecs >= 0) {
        request.setTcpKeepAliveIdleTimeBeforeProbes(std::chrono::seconds(config.tcpKeepAliveIdleSecs));
    }
    if (config.tcpKeepAliveIntervalSecs >= 0) {
        request.setTcpKeepAliveIntervalBetweenProbes(std::chrono::seconds(config.tcpKeepAliveIntervalSecs));
    }
    if (config.tcpKeepAliveProbeCount >= 0) {
        request.setTcpKeepAliveProbeCount(config.tcpKeepAliveProbeCount);
    }
}

QNetworkRequest createRequest(const QUrl& url, const RequestConfig& config)
{
    QNetworkRequest request(url);
    configureRequest(request, config);
    return request;
}

void setStandardHeaders(QNetworkRequest& request, const QString& userAgent)
{
    using WK = QHttpHeaders::WellKnownHeader;
    QHttpHeaders headers = request.headers();
    headers.replaceOrAppend(WK::UserAgent, userAgent.isEmpty() ? defaultUserAgent() : userAgent);
    headers.replaceOrAppend(WK::Accept, "*/*");
    headers.replaceOrAppend(WK::AcceptEncoding, "gzip, deflate");
    headers.replaceOrAppend(WK::Connection, "keep-alive");
    request.setHeaders(headers);
}

void setJsonHeaders(QNetworkRequest& request)
{
    using WK = QHttpHeaders::WellKnownHeader;
    QHttpHeaders headers = request.headers();
    headers.replaceOrAppend(WK::Accept, "application/json");
    headers.replaceOrAppend(WK::ContentType, "application/json");
    request.setHeaders(headers);
}

void setFormHeaders(QNetworkRequest& request)
{
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/x-www-form-urlencoded"));
}

// ============================================================================
// Multipart Form Data Helpers
// ============================================================================

QHttpPart createFormField(const QString& name, const QString& value)
{
    QHttpPart part;
    part.setHeader(QNetworkRequest::ContentDispositionHeader, QStringLiteral("form-data; name=\"%1\"").arg(name));
    part.setBody(value.toUtf8());
    return part;
}

QHttpPart createFilePart(const QString& name, const QString& fileName, const QString& contentType, QIODevice* device)
{
    QHttpPart part;
    part.setHeader(QNetworkRequest::ContentTypeHeader, contentType);
    part.setHeader(QNetworkRequest::ContentDispositionHeader,
                   QStringLiteral("form-data; name=\"%1\"; filename=\"%2\"").arg(name, fileName));
    part.setBodyDevice(device);
    return part;
}

QHttpPart createFilePart(const QString& name, const QString& fileName, QIODevice* device)
{
    return createFilePart(name, fileName, kContentTypeOctetStream, device);
}

// ============================================================================
// SSL/TLS Configuration Builders
// ============================================================================

QSslConfiguration createInsecureSslConfig()
{
    QSslConfiguration config = QSslConfiguration::defaultConfiguration();
    config.setPeerVerifyMode(QSslSocket::VerifyNone);
    return config;
}

void applySslConfig(QNetworkRequest& request, const QSslConfiguration& config)
{
    request.setSslConfiguration(config);
}

QList<QSslCertificate> loadCaCertificates(const QString& filePath, QString* errorOut)
{
    const auto certs = QSslCertificate::fromPath(filePath, QSsl::Pem);
    if (certs.isEmpty() && errorOut) {
        *errorOut = QStringLiteral("No certificates found in %1").arg(filePath);
    }
    return certs;
}

bool loadClientCertAndKey(const QString& certPath, const QString& keyPath, QSslCertificate& certOut, QSslKey& keyOut,
                          QString* errorOut)
{
    const auto certs = QSslCertificate::fromPath(certPath, QSsl::Pem);
    if (certs.isEmpty()) {
        if (errorOut)
            *errorOut = QStringLiteral("No certificate found in %1").arg(certPath);
        return false;
    }

    QFile keyFile(keyPath);
    if (!keyFile.open(QIODevice::ReadOnly)) {
        if (errorOut)
            *errorOut = QStringLiteral("Cannot open key file %1").arg(keyPath);
        return false;
    }

    QSslKey key(&keyFile, QSsl::Rsa);
    if (key.isNull()) {
        keyFile.seek(0);
        key = QSslKey(&keyFile, QSsl::Ec);
    }
    if (key.isNull()) {
        if (errorOut)
            *errorOut = QStringLiteral("Invalid key in %1").arg(keyPath);
        return false;
    }

    certOut = certs.first();
    keyOut = key;
    return true;
}

// ============================================================================
// JSON Response Helpers
// ============================================================================

QJsonDocument parseJson(const QByteArray& data, QJsonParseError* error)
{
    QJsonParseError localError;
    QJsonParseError* errorPtr = (error != nullptr) ? error : &localError;

    QJsonDocument doc = QJsonDocument::fromJson(data, errorPtr);

    if (errorPtr->error != QJsonParseError::NoError) {
        qCWarning(QGCNetworkHelperLog) << "JSON parse error:" << errorPtr->errorString() << "at offset"
                                       << errorPtr->offset;
        return {};
    }

    return doc;
}

QJsonDocument parseJsonReply(QNetworkReply* reply, QJsonParseError* error)
{
    if ((reply == nullptr) || (reply->error() != QNetworkReply::NoError)) {
        if (reply != nullptr) {
            qCWarning(QGCNetworkHelperLog) << "Network error before JSON parse:" << reply->errorString();
        }
        if (error != nullptr) {
            error->error = QJsonParseError::UnterminatedObject;
            error->offset = 0;
        }
        return {};
    }

    return parseJson(reply->readAll(), error);
}

bool looksLikeJson(const QByteArray& data)
{
    // Skip leading whitespace
    for (int i = 0; i < data.size(); ++i) {
        const char c = data.at(i);
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            continue;
        }
        return c == '{' || c == '[';
    }

    return false;
}

// ============================================================================
// Network Reply Helpers
// ============================================================================

int httpStatusCode(const QNetworkReply* reply)
{
    if (!reply) {
        return -1;
    }
    const QVariant statusAttr = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    return statusAttr.isValid() ? statusAttr.toInt() : -1;
}

QUrl redirectUrl(const QNetworkReply* reply)
{
    if (!reply) {
        return {};
    }

    const QVariant redirectAttr = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    if (redirectAttr.isNull()) {
        return {};
    }

    const QUrl redirectTarget = redirectAttr.toUrl();
    return reply->url().resolved(redirectTarget);
}

QString errorMessage(const QNetworkReply* reply)
{
    if (!reply) {
        return QStringLiteral("No reply");
    }

    if (reply->error() != QNetworkReply::NoError) {
        return reply->errorString();
    }

    const int status = httpStatusCode(reply);
    if (status >= 400) {
        return QStringLiteral("HTTP %1: %2").arg(status).arg(httpStatusText(status));
    }

    return {};
}

bool isSuccess(const QNetworkReply* reply)
{
    if (!reply) {
        return false;
    }

    if (reply->error() != QNetworkReply::NoError) {
        return false;
    }

    const int status = httpStatusCode(reply);
    return status == -1 || isHttpSuccess(status);  // -1 means non-HTTP (e.g., file://)
}

bool isRedirect(const QNetworkReply* reply)
{
    if (!reply) {
        return false;
    }

    const int status = httpStatusCode(reply);
    return isHttpRedirect(status) || !redirectUrl(reply).isEmpty();
}

QString contentType(const QNetworkReply* reply)
{
    if (!reply) {
        return {};
    }
    return reply->header(QNetworkRequest::ContentTypeHeader).toString();
}

qint64 contentLength(const QNetworkReply* reply)
{
    if (!reply) {
        return -1;
    }
    const QVariant lenAttr = reply->header(QNetworkRequest::ContentLengthHeader);
    return lenAttr.isValid() ? lenAttr.toLongLong() : -1;
}

bool isJsonResponse(const QNetworkReply* reply)
{
    const QString type = contentType(reply);
    return type.contains(QLatin1String("application/json"), Qt::CaseInsensitive) ||
           type.contains(QLatin1String("+json"), Qt::CaseInsensitive);
}

// ============================================================================
// Network Availability
// ============================================================================

QNetworkInformation* networkInformation()
{
    if (!QNetworkInformation::loadDefaultBackend()) {
        qCDebug(QGCNetworkHelperLog) << "Failed to load network information backend";
    }
    if (!QNetworkInformation::loadBackendByFeatures(QNetworkInformation::Feature::Reachability)) {
        qCDebug(QGCNetworkHelperLog) << "Network information backend does not provide reachability";
    }
    return QNetworkInformation::instance();
}

bool isNetworkAvailable()
{
    const QNetworkInformation* netInfo = networkInformation();
    return !netInfo || netInfo->reachability() != QNetworkInformation::Reachability::Disconnected;
}

bool isInternetAvailable()
{
    const QNetworkInformation* netInfo = networkInformation();
    return netInfo && netInfo->reachability() == QNetworkInformation::Reachability::Online;
}

bool isNetworkEthernet()
{
    const QNetworkInformation* netInfo = networkInformation();
    return netInfo && netInfo->transportMedium() == QNetworkInformation::TransportMedium::Ethernet;
}

bool isBluetoothAvailable()
{
    const QList<QBluetoothHostInfo> devices = QBluetoothLocalDevice::allDevices();
    return !devices.isEmpty();
}

ConnectionType connectionType()
{
    const QNetworkInformation* netInfo = networkInformation();
    if (!netInfo) {
        return ConnectionType::Unknown;
    }

    if (netInfo->reachability() == QNetworkInformation::Reachability::Disconnected) {
        return ConnectionType::None;
    }

    switch (netInfo->transportMedium()) {
        case QNetworkInformation::TransportMedium::Ethernet:
            return ConnectionType::Ethernet;
        case QNetworkInformation::TransportMedium::WiFi:
            return ConnectionType::WiFi;
        case QNetworkInformation::TransportMedium::Cellular:
            return ConnectionType::Cellular;
        case QNetworkInformation::TransportMedium::Bluetooth:
            return ConnectionType::Bluetooth;
        default:
            return ConnectionType::Unknown;
    }
}

QString connectionTypeName(ConnectionType type)
{
    switch (type) {
        case ConnectionType::None:
            return QStringLiteral("None");
        case ConnectionType::Ethernet:
            return QStringLiteral("Ethernet");
        case ConnectionType::WiFi:
            return QStringLiteral("WiFi");
        case ConnectionType::Cellular:
            return QStringLiteral("Cellular");
        case ConnectionType::Bluetooth:
            return QStringLiteral("Bluetooth");
        case ConnectionType::Unknown:
        default:
            return QStringLiteral("Unknown");
    }
}

// ============================================================================
// SSL/TLS Helpers
// ============================================================================

void ignoreSslErrors(QNetworkReply* reply)
{
    if (!reply) {
        return;
    }

    QObject::connect(reply, &QNetworkReply::sslErrors, reply, [reply](const QList<QSslError>& errors) {
        qCWarning(QGCNetworkHelperLog) << "Ignoring SSL errors for" << reply->url();
        for (const QSslError& error : errors) {
            qCDebug(QGCNetworkHelperLog) << "  -" << error.errorString();
        }
        reply->ignoreSslErrors();
    });
}

void ignoreSslErrorsIfNeeded(QNetworkReply* reply)
{
    if (!reply) {
        return;
    }

    // Check for OpenSSL version mismatch: Qt built with 1.x but running with 3.x
    const bool sslLibraryBuildIs1x = ((QSslSocket::sslLibraryBuildVersionNumber() & 0xf0000000) == 0x10000000);
    const bool sslLibraryIs3x = ((QSslSocket::sslLibraryVersionNumber() & 0xf0000000) == 0x30000000);
    if (sslLibraryBuildIs1x && sslLibraryIs3x) {
        qCWarning(QGCNetworkHelperLog) << "Ignoring SSL certificate errors due to OpenSSL version mismatch";
        QList<QSslError> errorsThatCanBeIgnored;
        errorsThatCanBeIgnored << QSslError(QSslError::NoPeerCertificate);
        reply->ignoreSslErrors(errorsThatCanBeIgnored);
    }
}

bool isSslAvailable()
{
    return QSslSocket::supportsSsl();
}

QString sslVersion()
{
    return QSslSocket::sslLibraryVersionString();
}

// ============================================================================
// Network Access Manager Helpers
// ============================================================================

void initializeProxySupport()
{
    QNetworkProxyFactory::setUseSystemConfiguration(true);
}

}  // namespace QGCNetworkHelper
