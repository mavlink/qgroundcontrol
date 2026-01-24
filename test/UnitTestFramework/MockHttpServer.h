#pragma once

/// @file MockHttpServer.h
/// @brief Mock HTTP server for testing network-dependent features.
///
/// Provides a lightweight HTTP server for testing features that require
/// network access without external dependencies:
/// - Terrain tile queries
/// - Firmware downloads
/// - Map tile fetching
/// - REST API interactions
///
/// Usage:
/// @code
/// void MyTest::testTerrainQuery()
/// {
///     MockHttpServer server;
///     server.addResponse("/api/terrain", 200, R"({"elevation": 100.5})");
///     server.start();
///
///     // Configure your component to use server.baseUrl()
///     TerrainQuery query;
///     query.setBaseUrl(server.baseUrl());
///
///     // Make the request
///     auto result = query.getElevation(37.7749, -122.4194);
///
///     // Verify request was made
///     QCOMPARE(server.requestCount("/api/terrain"), 1);
///     QCOMPARE(result, 100.5);
/// }
/// @endcode

#include <QtCore/QByteArray>
#include <QtCore/QHash>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QMutex>
#include <QtCore/QObject>
#include <QtCore/QRegularExpression>
#include <QtCore/QString>
#include <QtCore/QThread>
#include <QtCore/QUrl>
#include <QtCore/QUrlQuery>
#include <QtCore/QWaitCondition>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>

#include <functional>
#include <memory>
#include <vector>

/// HTTP request received by the mock server
struct MockHttpRequest {
    QString method;             ///< HTTP method (GET, POST, etc.)
    QString path;               ///< Request path
    QString query;              ///< Query string
    QHash<QString, QString> headers;  ///< Request headers
    QByteArray body;            ///< Request body
    QUrl fullUrl;               ///< Full URL including query parameters

    /// Get a query parameter value
    QString queryParam(const QString& name) const {
        QUrlQuery urlQuery(query);
        return urlQuery.queryItemValue(name);
    }

    /// Check if request has a specific header
    bool hasHeader(const QString& name) const {
        return headers.contains(name.toLower());
    }

    /// Get header value (case-insensitive)
    QString header(const QString& name) const {
        return headers.value(name.toLower());
    }
};

/// HTTP response to be sent by the mock server
struct MockHttpResponse {
    int statusCode = 200;
    QString statusText = QStringLiteral("OK");
    QHash<QString, QString> headers;
    QByteArray body;
    int delayMs = 0;            ///< Delay before sending response

    MockHttpResponse() {
        headers[QStringLiteral("Content-Type")] = QStringLiteral("text/plain");
    }

    /// Create a JSON response
    static MockHttpResponse json(const QJsonObject& obj, int status = 200) {
        MockHttpResponse response;
        response.statusCode = status;
        response.statusText = (status == 200) ? QStringLiteral("OK") : QStringLiteral("Error");
        response.headers[QStringLiteral("Content-Type")] = QStringLiteral("application/json");
        response.body = QJsonDocument(obj).toJson(QJsonDocument::Compact);
        return response;
    }

    /// Create a JSON response from string
    static MockHttpResponse json(const QString& jsonStr, int status = 200) {
        MockHttpResponse response;
        response.statusCode = status;
        response.statusText = (status == 200) ? QStringLiteral("OK") : QStringLiteral("Error");
        response.headers[QStringLiteral("Content-Type")] = QStringLiteral("application/json");
        response.body = jsonStr.toUtf8();
        return response;
    }

    /// Create a plain text response
    static MockHttpResponse text(const QString& text, int status = 200) {
        MockHttpResponse response;
        response.statusCode = status;
        response.statusText = (status == 200) ? QStringLiteral("OK") : QStringLiteral("Error");
        response.headers[QStringLiteral("Content-Type")] = QStringLiteral("text/plain");
        response.body = text.toUtf8();
        return response;
    }

    /// Create a binary response
    static MockHttpResponse binary(const QByteArray& data,
                                   const QString& contentType = QStringLiteral("application/octet-stream"),
                                   int status = 200) {
        MockHttpResponse response;
        response.statusCode = status;
        response.statusText = (status == 200) ? QStringLiteral("OK") : QStringLiteral("Error");
        response.headers[QStringLiteral("Content-Type")] = contentType;
        response.body = data;
        return response;
    }

    /// Create an error response
    static MockHttpResponse error(int status, const QString& message = QString()) {
        MockHttpResponse response;
        response.statusCode = status;
        switch (status) {
            case 400: response.statusText = QStringLiteral("Bad Request"); break;
            case 401: response.statusText = QStringLiteral("Unauthorized"); break;
            case 403: response.statusText = QStringLiteral("Forbidden"); break;
            case 404: response.statusText = QStringLiteral("Not Found"); break;
            case 500: response.statusText = QStringLiteral("Internal Server Error"); break;
            case 503: response.statusText = QStringLiteral("Service Unavailable"); break;
            default: response.statusText = QStringLiteral("Error"); break;
        }
        response.body = message.isEmpty() ? response.statusText.toUtf8() : message.toUtf8();
        return response;
    }

    /// Create a redirect response
    static MockHttpResponse redirect(const QString& location, int status = 302) {
        MockHttpResponse response;
        response.statusCode = status;
        response.statusText = (status == 301) ? QStringLiteral("Moved Permanently")
                                              : QStringLiteral("Found");
        response.headers[QStringLiteral("Location")] = location;
        return response;
    }

    /// Add delay before response
    MockHttpResponse& withDelay(int ms) {
        delayMs = ms;
        return *this;
    }

    /// Add a header
    MockHttpResponse& withHeader(const QString& name, const QString& value) {
        headers[name] = value;
        return *this;
    }
};

/// Request handler function type
using MockHttpHandler = std::function<MockHttpResponse(const MockHttpRequest&)>;

/// Route definition for pattern matching
struct MockHttpRoute {
    QString method;             ///< HTTP method or "*" for any
    QRegularExpression pathPattern;  ///< Regex pattern for path
    MockHttpHandler handler;
    int callCount = 0;
    std::vector<MockHttpRequest> requests;  ///< Recorded requests
};

/// Mock HTTP server for testing
class MockHttpServer : public QObject
{
    Q_OBJECT

public:
    explicit MockHttpServer(QObject* parent = nullptr)
        : QObject(parent)
        , _server(new QTcpServer(this))
    {
        connect(_server, &QTcpServer::newConnection, this, &MockHttpServer::_handleConnection);
    }

    ~MockHttpServer() override {
        stop();
    }

    /// Start the server on a random available port
    bool start(quint16 port = 0) {
        if (_server->isListening()) {
            return true;
        }

        if (!_server->listen(QHostAddress::LocalHost, port)) {
            qWarning() << "MockHttpServer: Failed to start:" << _server->errorString();
            return false;
        }

        _port = _server->serverPort();
        qDebug() << "MockHttpServer: Listening on port" << _port;
        return true;
    }

    /// Stop the server
    void stop() {
        if (_server->isListening()) {
            _server->close();
            qDebug() << "MockHttpServer: Stopped";
        }
    }

    /// Get the server port
    quint16 port() const { return _port; }

    /// Get the base URL for the server
    QString baseUrl() const {
        return QStringLiteral("http://localhost:%1").arg(_port);
    }

    /// Add a static response for a path
    void addResponse(const QString& path, int statusCode, const QString& body,
                     const QString& contentType = QStringLiteral("application/json")) {
        MockHttpResponse response;
        response.statusCode = statusCode;
        response.body = body.toUtf8();
        response.headers[QStringLiteral("Content-Type")] = contentType;

        addRoute("*", path, [response](const MockHttpRequest&) {
            return response;
        });
    }

    /// Add a response object for a path
    void addResponse(const QString& path, const MockHttpResponse& response) {
        addRoute("*", path, [response](const MockHttpRequest&) {
            return response;
        });
    }

    /// Add a route with a handler function
    void addRoute(const QString& method, const QString& pathPattern, MockHttpHandler handler) {
        QMutexLocker locker(&_mutex);

        MockHttpRoute route;
        route.method = method.toUpper();

        // Convert simple path patterns to regex
        QString pattern = pathPattern;
        pattern.replace(QStringLiteral("*"), QStringLiteral(".*"));
        if (!pattern.startsWith('^')) {
            pattern = '^' + pattern;
        }
        if (!pattern.endsWith('$')) {
            pattern += '$';
        }
        route.pathPattern = QRegularExpression(pattern);
        route.handler = std::move(handler);

        _routes.append(route);
    }

    /// Add a GET route
    void get(const QString& path, MockHttpHandler handler) {
        addRoute(QStringLiteral("GET"), path, std::move(handler));
    }

    /// Add a POST route
    void post(const QString& path, MockHttpHandler handler) {
        addRoute(QStringLiteral("POST"), path, std::move(handler));
    }

    /// Get request count for a path
    int requestCount(const QString& path = QString()) const {
        QMutexLocker locker(&_mutex);

        if (path.isEmpty()) {
            return _totalRequests;
        }

        for (const auto& route : _routes) {
            if (route.pathPattern.match(path).hasMatch()) {
                return route.callCount;
            }
        }
        return 0;
    }

    /// Get all recorded requests
    QList<MockHttpRequest> requests() const {
        QMutexLocker locker(&_mutex);
        return _allRequests;
    }

    /// Get requests for a specific path
    std::vector<MockHttpRequest> requestsForPath(const QString& path) const {
        QMutexLocker locker(&_mutex);

        for (const auto& route : _routes) {
            if (route.pathPattern.match(path).hasMatch()) {
                return route.requests;
            }
        }
        return {};
    }

    /// Clear all recorded requests
    void clearRequests() {
        QMutexLocker locker(&_mutex);
        _totalRequests = 0;
        _allRequests.clear();
        for (auto& route : _routes) {
            route.callCount = 0;
            route.requests.clear();
        }
    }

    /// Clear all routes
    void clearRoutes() {
        QMutexLocker locker(&_mutex);
        _routes.clear();
    }

    /// Reset the server (clear routes and requests)
    void reset() {
        clearRoutes();
        clearRequests();
    }

    /// Set default response for unmatched routes
    void setDefaultResponse(const MockHttpResponse& response) {
        _defaultResponse = response;
    }

    /// Wait for a specific number of requests
    bool waitForRequests(int count, int timeoutMs = 5000) {
        QMutexLocker locker(&_mutex);
        if (_totalRequests >= count) {
            return true;
        }

        return _requestCondition.wait(&_mutex, timeoutMs);
    }

signals:
    void requestReceived(const MockHttpRequest& request);

private slots:
    void _handleConnection() {
        while (_server->hasPendingConnections()) {
            QTcpSocket* socket = _server->nextPendingConnection();
            connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
                _handleRequest(socket);
            });
            connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
        }
    }

private:
    void _handleRequest(QTcpSocket* socket) {
        QByteArray requestData = socket->readAll();
        QString requestStr = QString::fromUtf8(requestData);

        // Parse HTTP request
        MockHttpRequest request = _parseRequest(requestStr);

        // Record request
        {
            QMutexLocker locker(&_mutex);
            _totalRequests++;
            _allRequests.append(request);
        }

        emit requestReceived(request);

        // Find matching route
        MockHttpResponse response = _findResponse(request);

        // Apply delay if specified
        if (response.delayMs > 0) {
            QThread::msleep(response.delayMs);
        }

        // Send response
        _sendResponse(socket, response);
    }

    MockHttpRequest _parseRequest(const QString& requestStr) {
        MockHttpRequest request;

        QStringList lines = requestStr.split(QStringLiteral("\r\n"));
        if (lines.isEmpty()) {
            return request;
        }

        // Parse request line
        QStringList requestLine = lines[0].split(' ');
        if (requestLine.size() >= 2) {
            request.method = requestLine[0];
            QString fullPath = requestLine[1];

            int queryIndex = fullPath.indexOf('?');
            if (queryIndex >= 0) {
                request.path = fullPath.left(queryIndex);
                request.query = fullPath.mid(queryIndex + 1);
            } else {
                request.path = fullPath;
            }

            request.fullUrl = QUrl(baseUrl() + fullPath);
        }

        // Parse headers
        int i = 1;
        for (; i < lines.size(); ++i) {
            if (lines[i].isEmpty()) {
                break;
            }
            int colonIndex = lines[i].indexOf(':');
            if (colonIndex > 0) {
                QString name = lines[i].left(colonIndex).trimmed().toLower();
                QString value = lines[i].mid(colonIndex + 1).trimmed();
                request.headers[name] = value;
            }
        }

        // Parse body
        if (i + 1 < lines.size()) {
            request.body = lines.mid(i + 1).join(QStringLiteral("\r\n")).toUtf8();
        }

        return request;
    }

    MockHttpResponse _findResponse(const MockHttpRequest& request) {
        QMutexLocker locker(&_mutex);

        for (auto& route : _routes) {
            if ((route.method == "*" || route.method == request.method) &&
                route.pathPattern.match(request.path).hasMatch()) {

                route.callCount++;
                route.requests.push_back(request);

                _requestCondition.wakeAll();

                return route.handler(request);
            }
        }

        _requestCondition.wakeAll();
        return _defaultResponse;
    }

    void _sendResponse(QTcpSocket* socket, const MockHttpResponse& response) {
        QString statusLine = QStringLiteral("HTTP/1.1 %1 %2\r\n")
                            .arg(response.statusCode)
                            .arg(response.statusText);

        QString headers;
        for (auto it = response.headers.begin(); it != response.headers.end(); ++it) {
            headers += QStringLiteral("%1: %2\r\n").arg(it.key(), it.value());
        }
        headers += QStringLiteral("Content-Length: %1\r\n").arg(response.body.size());
        headers += QStringLiteral("Connection: close\r\n");
        headers += QStringLiteral("\r\n");

        socket->write(statusLine.toUtf8());
        socket->write(headers.toUtf8());
        socket->write(response.body);
        socket->flush();
        socket->disconnectFromHost();
    }

    QTcpServer* _server;
    quint16 _port = 0;
    QList<MockHttpRoute> _routes;
    MockHttpResponse _defaultResponse = MockHttpResponse::error(404);

    mutable QMutex _mutex;
    QWaitCondition _requestCondition;
    int _totalRequests = 0;
    QList<MockHttpRequest> _allRequests;
};

/// Pre-configured mock responses for common QGC use cases
namespace MockResponses {

/// Terrain elevation response
inline MockHttpResponse terrainElevation(double elevation) {
    QJsonObject obj;
    obj[QStringLiteral("elevation")] = elevation;
    obj[QStringLiteral("resolution")] = 30;
    return MockHttpResponse::json(obj);
}

/// Terrain elevation batch response
inline MockHttpResponse terrainElevationBatch(const QList<double>& elevations) {
    QJsonArray arr;
    for (double e : elevations) {
        arr.append(e);
    }
    QJsonObject obj;
    obj[QStringLiteral("elevations")] = arr;
    return MockHttpResponse::json(obj);
}

/// Firmware manifest response
inline MockHttpResponse firmwareManifest(const QString& version,
                                         const QString& url,
                                         const QString& checksum = QString()) {
    QJsonObject obj;
    obj[QStringLiteral("version")] = version;
    obj[QStringLiteral("url")] = url;
    if (!checksum.isEmpty()) {
        obj[QStringLiteral("checksum")] = checksum;
    }
    return MockHttpResponse::json(obj);
}

/// Simple image tile (1x1 PNG)
inline MockHttpResponse imageTile() {
    // Minimal valid PNG (1x1 transparent pixel)
    static const unsigned char pngData[] = {
        0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A,
        0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52,
        0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01,
        0x08, 0x06, 0x00, 0x00, 0x00, 0x1F, 0x15, 0xC4,
        0x89, 0x00, 0x00, 0x00, 0x0A, 0x49, 0x44, 0x41,
        0x54, 0x78, 0x9C, 0x63, 0x00, 0x01, 0x00, 0x00,
        0x05, 0x00, 0x01, 0x0D, 0x0A, 0x2D, 0xB4, 0x00,
        0x00, 0x00, 0x00, 0x49, 0x45, 0x4E, 0x44, 0xAE,
        0x42, 0x60, 0x82
    };

    return MockHttpResponse::binary(
        QByteArray(reinterpret_cast<const char*>(pngData), sizeof(pngData)),
        QStringLiteral("image/png")
    );
}

} // namespace MockResponses
