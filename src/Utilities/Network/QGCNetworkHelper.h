#pragma once

#include <QtCore/QJsonDocument>
#include <QtCore/QString>
#include <QtCore/QUrl>
#include <QtHttpServer/QHttpServerRequest>
#include <QtHttpServer/QHttpServerResponder>
#include <QtNetwork/QHttpPart>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QSslConfiguration>

class QIODevice;
class QNetworkAccessManager;

/// Network utility functions for HTTP requests, URL handling, and connectivity
/// All functions are stateless and thread-safe
namespace QGCNetworkHelper {

// ============================================================================
// Constants
// ============================================================================

/// Default request timeout in milliseconds
constexpr int kDefaultTimeoutMs = 30000;

/// Default connection timeout for initial connect
constexpr int kDefaultConnectTimeoutMs = 10000;

/// Maximum number of redirects to follow
constexpr int kMaxRedirects = 10;

// ============================================================================
// Content-Type Constants
// ============================================================================

/// Common MIME types for Content-Type headers
inline const QString kContentTypeJson = QStringLiteral("application/json");
inline const QString kContentTypeFormUrlencoded = QStringLiteral("application/x-www-form-urlencoded");
inline const QString kContentTypeOctetStream = QStringLiteral("application/octet-stream");
inline const QString kContentTypeMultipartForm = QStringLiteral("multipart/form-data");
inline const QString kContentTypeXml = QStringLiteral("application/xml");
inline const QString kContentTypeTextPlain = QStringLiteral("text/plain");

// ============================================================================
// HTTP Status Code Helpers
// ============================================================================

/// HTTP status codes - uses Qt's QHttpServerResponder::StatusCode enum
using HttpStatusCode = QHttpServerResponder::StatusCode;

/// HTTP status code ranges
enum class HttpStatusClass
{
    Informational,  ///< 1xx - Informational
    Success,        ///< 2xx - Success
    Redirection,    ///< 3xx - Redirection
    ClientError,    ///< 4xx - Client errors
    ServerError,    ///< 5xx - Server errors
    Unknown         ///< Not a valid HTTP status
};

/// Classify an HTTP status code
HttpStatusClass classifyHttpStatus(int statusCode);

/// Check if HTTP status indicates success (2xx)
inline bool isHttpSuccess(int statusCode)
{
    return statusCode >= 200 && statusCode < 300;
}

/// Check if HTTP status indicates redirect (3xx)
inline bool isHttpRedirect(int statusCode)
{
    return statusCode >= 300 && statusCode < 400;
}

/// Check if HTTP status indicates client error (4xx)
inline bool isHttpClientError(int statusCode)
{
    return statusCode >= 400 && statusCode < 500;
}

/// Check if HTTP status indicates server error (5xx)
inline bool isHttpServerError(int statusCode)
{
    return statusCode >= 500 && statusCode < 600;
}

/// Get human-readable description for HTTP status code
QString httpStatusText(int statusCode);
QString httpStatusText(HttpStatusCode statusCode);

// ============================================================================
// HTTP Methods
// ============================================================================

/// HTTP request methods - uses Qt's QHttpServerRequest::Method enum
using HttpMethod = QHttpServerRequest::Method;

/// Get string name for an HTTP method (e.g., "GET", "POST")
QString httpMethodName(HttpMethod method);

/// Parse HTTP method from string (case-insensitive)
/// Returns HttpMethod::Get if not recognized
HttpMethod parseHttpMethod(const QString& methodStr);

// ============================================================================
// URL Utilities
// ============================================================================

/// Check if URL is valid and has supported scheme
/// Supported: http, https, file, qrc
bool isValidUrl(const QUrl& url);

/// Check if URL uses HTTP or HTTPS scheme
bool isHttpUrl(const QUrl& url);

/// Check if URL uses secure HTTPS scheme
bool isHttpsUrl(const QUrl& url);

/// Normalize URL (lowercase scheme/host, remove default ports, trailing slashes)
QUrl normalizeUrl(const QUrl& url);

/// Ensure URL has scheme, defaulting to https:// if missing
QUrl ensureScheme(const QUrl& url, const QString& defaultScheme = QStringLiteral("https"));

/// Build URL with query parameters from a map
QUrl buildUrl(const QString& baseUrl, const QMap<QString, QString>& params);

/// Build URL with query parameters from a list of pairs
QUrl buildUrl(const QString& baseUrl, const QList<QPair<QString, QString>>& params);

/// Extract filename from URL path (last path segment)
QString urlFileName(const QUrl& url);

/// Get URL without query string and fragment
QUrl urlWithoutQuery(const QUrl& url);

// ============================================================================
// Request Configuration
// ============================================================================

/// Common request configuration options
struct RequestConfig
{
    int timeoutMs = kDefaultTimeoutMs;
    bool allowRedirects = true;
    bool http2Allowed = true;
    bool cacheEnabled = true;
    bool backgroundRequest = false;
    QString userAgent;
    QString accept = QStringLiteral("*/*");
    QString acceptEncoding;
    QString contentType;
};

/// Configure a QNetworkRequest with standard settings
/// @param request Request to configure
/// @param config Configuration options (uses defaults if not specified)
void configureRequest(QNetworkRequest& request, const RequestConfig& config = {});

/// Create a pre-configured QNetworkRequest
/// @param url The URL for the request
/// @param config Configuration options (uses defaults if not specified)
/// @return Configured QNetworkRequest ready for use
QNetworkRequest createRequest(const QUrl& url, const RequestConfig& config = {});

/// Set standard browser-like headers on a request
void setStandardHeaders(QNetworkRequest& request, const QString& userAgent = {});

/// Set JSON content headers (Accept and Content-Type)
void setJsonHeaders(QNetworkRequest& request);

/// Set form data content headers
void setFormHeaders(QNetworkRequest& request);

/// Get the default User-Agent string for QGC
QString defaultUserAgent();

// ============================================================================
// Authentication Helpers
// ============================================================================

/// Set HTTP Basic Authentication header
/// @param request Request to modify
/// @param credentials Base64-encoded "username:password" string
void setBasicAuth(QNetworkRequest& request, const QString& credentials);

/// Set HTTP Basic Authentication header from username and password
/// @param request Request to modify
/// @param username The username
/// @param password The password
void setBasicAuth(QNetworkRequest& request, const QString& username, const QString& password);

/// Set Bearer token authentication header
/// @param request Request to modify
/// @param token The bearer token (without "Bearer " prefix)
void setBearerToken(QNetworkRequest& request, const QString& token);

/// Create Basic Auth credentials string from username and password
/// @return Base64-encoded "username:password" string
QString createBasicAuthCredentials(const QString& username, const QString& password);

// ============================================================================
// Multipart Form Data Helpers
// ============================================================================

/// Create a simple text form field part
/// @param name Field name
/// @param value Field value
/// @return QHttpPart configured as form-data
QHttpPart createFormField(const QString& name, const QString& value);

/// Create a file upload part
/// @param name Field name (e.g., "filearg")
/// @param fileName Display filename
/// @param contentType MIME type of the file (e.g., "application/octet-stream")
/// @param device IO device to read file data from (must stay valid during upload)
/// @return QHttpPart configured as file upload
QHttpPart createFilePart(const QString& name, const QString& fileName, const QString& contentType, QIODevice* device);

/// Create a file upload part with auto-detected content type
/// @param name Field name
/// @param fileName Display filename (used for content-type detection)
/// @param device IO device to read file data from
/// @return QHttpPart configured as file upload
QHttpPart createFilePart(const QString& name, const QString& fileName, QIODevice* device);

// ============================================================================
// SSL/TLS Configuration Builders
// ============================================================================

/// Create SSL configuration with specified protocol
/// @param protocol TLS protocol version (default: TLS 1.2 or later)
/// @return Configured QSslConfiguration
QSslConfiguration createSslConfig(QSsl::SslProtocol protocol = QSsl::TlsV1_2OrLater);

/// Create SSL configuration that disables peer verification (use with caution!)
/// Only for development/testing or known self-signed certificates
/// @return QSslConfiguration with peer verification disabled
QSslConfiguration createInsecureSslConfig();

/// Apply SSL configuration to a request
/// @param request Request to modify
/// @param config SSL configuration to apply
void applySslConfig(QNetworkRequest& request, const QSslConfiguration& config);

// ============================================================================
// JSON Response Helpers
// ============================================================================

/// Parse JSON from network reply data
/// @param data Raw response data
/// @param error Optional pointer to receive parse error details
/// @return Parsed JSON document (null if parsing failed)
QJsonDocument parseJson(const QByteArray& data, QJsonParseError* error = nullptr);

/// Parse JSON from network reply
/// @param reply Network reply to read from
/// @param error Optional pointer to receive parse error details
/// @return Parsed JSON document (null if parsing failed or reply has error)
QJsonDocument parseJsonReply(QNetworkReply* reply, QJsonParseError* error = nullptr);

/// Check if data appears to be valid JSON (quick check)
/// @param data Data to check
/// @return true if data starts with { or [
bool looksLikeJson(const QByteArray& data);

// ============================================================================
// Network Reply Helpers
// ============================================================================

/// Get HTTP status code from a network reply
/// Returns -1 if not an HTTP response
int httpStatusCode(const QNetworkReply* reply);

/// Get redirect URL from a reply, if any
/// Returns empty URL if no redirect
QUrl redirectUrl(const QNetworkReply* reply);

/// Get error message from a network reply
/// Combines network error and HTTP status into readable message
QString errorMessage(const QNetworkReply* reply);

/// Check if reply indicates success (no error and HTTP 2xx)
bool isSuccess(const QNetworkReply* reply);

/// Check if reply indicates a redirect
bool isRedirect(const QNetworkReply* reply);

/// Get Content-Type header from reply
QString contentType(const QNetworkReply* reply);

/// Get Content-Length header from reply (-1 if not present)
qint64 contentLength(const QNetworkReply* reply);

/// Check if response is JSON based on Content-Type
bool isJsonResponse(const QNetworkReply* reply);

// ============================================================================
// Network Availability
// ============================================================================

/// Check if network is available (not disconnected)
bool isNetworkAvailable();

/// Check if internet is reachable (online state, stricter than isNetworkAvailable)
bool isInternetAvailable();

/// Check if current network connection is Ethernet
bool isNetworkEthernet();

/// Check if Bluetooth is available on this device
bool isBluetoothAvailable();

/// Network connection types
enum class ConnectionType
{
    None,       ///< No network connection
    Unknown,    ///< Connection type unknown
    Ethernet,   ///< Wired ethernet
    WiFi,       ///< Wireless LAN
    Cellular,   ///< Mobile/cellular data
    Bluetooth,  ///< Bluetooth connection
};

/// Get current network connection type
ConnectionType connectionType();

/// Get human-readable name for connection type
QString connectionTypeName(ConnectionType type);

// ============================================================================
// SSL/TLS Helpers
// ============================================================================

/// Configure SSL to ignore certificate errors (use with caution!)
/// This should only be used for development/testing or known self-signed certs
void ignoreSslErrors(QNetworkReply* reply);

/// Ignore SSL errors if there's an OpenSSL version mismatch
/// Qt may be built with OpenSSL 1.x but running with OpenSSL 3.x, causing certificate issues
void ignoreSslErrorsIfNeeded(QNetworkReply* reply);

/// Check if SSL is available
bool isSslAvailable();

/// Get SSL library version string
QString sslVersion();

// ============================================================================
// Network Access Manager Helpers
// ============================================================================

/// Initialize network proxy support (call once at application startup)
/// Enables system proxy configuration for all network requests
void initializeProxySupport();

/// Create a network access manager with recommended settings
/// Caller takes ownership of the returned pointer
QNetworkAccessManager* createNetworkManager(QObject* parent = nullptr);

/// Set up default proxy configuration on a network manager
void configureProxy(QNetworkAccessManager* manager);

// ============================================================================
// Compressed Data Helpers
// ============================================================================

/// Check if data appears to be compressed based on magic bytes
/// Detects: gzip, xz, zstd, bzip2, lz4
/// @param data Data to check (needs at least 4 bytes)
/// @return true if data has a recognized compression magic signature
bool looksLikeCompressedData(const QByteArray& data);

/// Parse JSON that may be compressed
/// Automatically detects and decompresses gzip, xz, zstd, bzip2, lz4 data
/// @param data Raw data (compressed or uncompressed JSON)
/// @param error Optional pointer to receive parse error details
/// @return Parsed JSON document (null if parsing failed)
QJsonDocument parseCompressedJson(const QByteArray& data, QJsonParseError* error = nullptr);

}  // namespace QGCNetworkHelper
