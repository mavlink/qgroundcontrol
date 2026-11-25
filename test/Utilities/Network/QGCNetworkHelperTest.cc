#include "QGCNetworkHelperTest.h"

#include <chrono>

#include <QtCore/QCoreApplication>
#include <QtCore/QMap>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

#include "Fixtures/RAIIFixtures.h"
#include "QGCNetworkHelper.h"

// ============================================================================
// HTTP Status Code Helpers Tests
// ============================================================================
void QGCNetworkHelperTest::_testHttpStatusClass_data()
{
    using Class = QGCNetworkHelper::HttpStatusClass;
    QTest::addColumn<int>("status");
    QTest::addColumn<int>("statusClass");

    const auto addRows = [](Class statusClass, std::initializer_list<int> statuses) {
        for (const int status : statuses) {
            QTest::addRow("%d", status) << status << static_cast<int>(statusClass);
        }
    };
    addRows(Class::Informational, {100, 101, 199});
    addRows(Class::Success, {200, 201, 204, 299});
    addRows(Class::Redirection, {300, 301, 302, 304, 399});
    addRows(Class::ClientError, {400, 401, 403, 404, 499});
    addRows(Class::ServerError, {500, 501, 502, 503, 599});
    addRows(Class::Unknown, {-1, 0, 99, 600, 1000});
}

void QGCNetworkHelperTest::_testHttpStatusClass()
{
    using Class = QGCNetworkHelper::HttpStatusClass;
    QFETCH(int, status);
    QFETCH(int, statusClass);
    const Class expected = static_cast<Class>(statusClass);

    QCOMPARE(QGCNetworkHelper::classifyHttpStatus(status), expected);
    QCOMPARE(QGCNetworkHelper::isHttpSuccess(status), expected == Class::Success);
    QCOMPARE(QGCNetworkHelper::isHttpRedirect(status), expected == Class::Redirection);
    QCOMPARE(QGCNetworkHelper::isHttpClientError(status), expected == Class::ClientError);
    QCOMPARE(QGCNetworkHelper::isHttpServerError(status), expected == Class::ServerError);
}

void QGCNetworkHelperTest::_testHttpStatusText_data()
{
    QTest::addColumn<int>("status");
    QTest::addColumn<QString>("text");

    const QList<std::pair<int, const char*>> texts = {
        {100, "Continue"},
        {101, "Switching Protocols"},
        {200, "OK"},
        {201, "Created"},
        {202, "Accepted"},
        {204, "No Content"},
        {301, "Moved Permanently"},
        {302, "Found"},
        {304, "Not Modified"},
        {400, "Bad Request"},
        {401, "Unauthorized"},
        {403, "Forbidden"},
        {404, "Not Found"},
        {405, "Method Not Allowed"},
        {408, "Request Timeout"},
        {429, "Too Many Requests"},
        {500, "Internal Server Error"},
        {502, "Bad Gateway"},
        {503, "Service Unavailable"},
        {504, "Gateway Timeout"},
    };
    for (const auto& [status, text] : texts) {
        QTest::addRow("%d", status) << status << QString::fromLatin1(text);
    }
}

void QGCNetworkHelperTest::_testHttpStatusText()
{
    QFETCH(int, status);
    QFETCH(QString, text);

    QCOMPARE(QGCNetworkHelper::httpStatusText(status), text);
    QCOMPARE(QGCNetworkHelper::httpStatusText(static_cast<QGCNetworkHelper::HttpStatusCode>(status)), text);
}

void QGCNetworkHelperTest::_testHttpStatusTextUnknown()
{
    // Unknown status should return formatted message
    QVERIFY(QGCNetworkHelper::httpStatusText(999).contains("999"));
    // In-range but unnamed codes must still surface the numeric value
    QVERIFY(QGCNetworkHelper::httpStatusText(218).contains("218"));
    QVERIFY(QGCNetworkHelper::httpStatusText(450).contains("450"));
}

// ============================================================================
// HTTP Methods Tests
// ============================================================================
void QGCNetworkHelperTest::_testHttpMethodNames_data()
{
    using Method = QGCNetworkHelper::HttpMethod;
    QTest::addColumn<int>("method");
    QTest::addColumn<QString>("name");

    const QList<std::pair<Method, const char*>> names = {
        {Method::Get, "GET"},       {Method::Post, "POST"},       {Method::Put, "PUT"},
        {Method::Delete, "DELETE"}, {Method::Head, "HEAD"},       {Method::Options, "OPTIONS"},
        {Method::Patch, "PATCH"},   {Method::Connect, "CONNECT"}, {Method::Trace, "TRACE"},
    };
    for (const auto& [method, name] : names) {
        QTest::newRow(name) << static_cast<int>(method) << QString::fromLatin1(name);
    }
}

void QGCNetworkHelperTest::_testHttpMethodNames()
{
    QFETCH(int, method);
    QFETCH(QString, name);
    const auto httpMethod = static_cast<QGCNetworkHelper::HttpMethod>(method);

    QCOMPARE(QGCNetworkHelper::httpMethodName(httpMethod), name);
    QCOMPARE(QGCNetworkHelper::parseHttpMethod(name), httpMethod);
}

void QGCNetworkHelperTest::_testParseHttpMethodCaseInsensitive()
{
    QCOMPARE(QGCNetworkHelper::parseHttpMethod("get"), QGCNetworkHelper::HttpMethod::Get);
    QCOMPARE(QGCNetworkHelper::parseHttpMethod("Get"), QGCNetworkHelper::HttpMethod::Get);
    QCOMPARE(QGCNetworkHelper::parseHttpMethod("gET"), QGCNetworkHelper::HttpMethod::Get);
    QCOMPARE(QGCNetworkHelper::parseHttpMethod("post"), QGCNetworkHelper::HttpMethod::Post);
    QCOMPARE(QGCNetworkHelper::parseHttpMethod("Post"), QGCNetworkHelper::HttpMethod::Post);
}

void QGCNetworkHelperTest::_testParseHttpMethodUnknown()
{
    // Unknown methods default to GET
    QCOMPARE(QGCNetworkHelper::parseHttpMethod("UNKNOWN"), QGCNetworkHelper::HttpMethod::Get);
    QCOMPARE(QGCNetworkHelper::parseHttpMethod(""), QGCNetworkHelper::HttpMethod::Get);
    QCOMPARE(QGCNetworkHelper::parseHttpMethod("INVALID"), QGCNetworkHelper::HttpMethod::Get);
}

// ============================================================================
// URL Utilities Tests
// ============================================================================
void QGCNetworkHelperTest::_testIsValidUrl()
{
    // Valid URLs
    QVERIFY(QGCNetworkHelper::isValidUrl(QUrl("http://example.com")));
    QVERIFY(QGCNetworkHelper::isValidUrl(QUrl("https://example.com")));
    QVERIFY(QGCNetworkHelper::isValidUrl(QUrl("file:///path/to/file")));
    QVERIFY(QGCNetworkHelper::isValidUrl(QUrl("qrc:/resource/path")));
    // Invalid URLs
    QVERIFY(!QGCNetworkHelper::isValidUrl(QUrl()));                     // Empty URL
    QVERIFY(!QGCNetworkHelper::isValidUrl(QUrl("ftp://example.com")));  // Unsupported scheme
}

void QGCNetworkHelperTest::_testIsHttpUrl()
{
    QVERIFY(QGCNetworkHelper::isHttpUrl(QUrl("http://example.com")));
    QVERIFY(QGCNetworkHelper::isHttpUrl(QUrl("https://example.com")));
    QVERIFY(QGCNetworkHelper::isHttpUrl(QUrl("HTTP://EXAMPLE.COM")));
    QVERIFY(QGCNetworkHelper::isHttpUrl(QUrl("HTTPS://EXAMPLE.COM")));
    QVERIFY(!QGCNetworkHelper::isHttpUrl(QUrl("file:///path")));
    QVERIFY(!QGCNetworkHelper::isHttpUrl(QUrl("ftp://example.com")));
    QVERIFY(!QGCNetworkHelper::isHttpUrl(QUrl()));
}

void QGCNetworkHelperTest::_testIsHttpsUrl()
{
    QVERIFY(QGCNetworkHelper::isHttpsUrl(QUrl("https://example.com")));
    QVERIFY(QGCNetworkHelper::isHttpsUrl(QUrl("HTTPS://EXAMPLE.COM")));
    QVERIFY(!QGCNetworkHelper::isHttpsUrl(QUrl("http://example.com")));
    QVERIFY(!QGCNetworkHelper::isHttpsUrl(QUrl("file:///path")));
    QVERIFY(!QGCNetworkHelper::isHttpsUrl(QUrl()));
}

void QGCNetworkHelperTest::_testNormalizeUrl()
{
    // Lowercase scheme and host
    QUrl normalized = QGCNetworkHelper::normalizeUrl(QUrl("HTTP://EXAMPLE.COM/Path"));
    QCOMPARE(normalized.scheme(), QStringLiteral("http"));
    QCOMPARE(normalized.host(), QStringLiteral("example.com"));
    QCOMPARE(normalized.path(), QStringLiteral("/Path"));  // Path case preserved
    // Remove default ports
    QUrl httpWithPort = QGCNetworkHelper::normalizeUrl(QUrl("http://example.com:80/path"));
    QCOMPARE(httpWithPort.port(), -1);
    QUrl httpsWithPort = QGCNetworkHelper::normalizeUrl(QUrl("https://example.com:443/path"));
    QCOMPARE(httpsWithPort.port(), -1);
    // Keep non-default ports
    QUrl customPort = QGCNetworkHelper::normalizeUrl(QUrl("http://example.com:8080/path"));
    QCOMPARE(customPort.port(), 8080);
    // Remove trailing slash (except root)
    QUrl trailingSlash = QGCNetworkHelper::normalizeUrl(QUrl("http://example.com/path/"));
    QCOMPARE(trailingSlash.path(), QStringLiteral("/path"));
    QUrl rootPath = QGCNetworkHelper::normalizeUrl(QUrl("http://example.com/"));
    QCOMPARE(rootPath.path(), QStringLiteral("/"));
}

void QGCNetworkHelperTest::_testEnsureScheme()
{
    // Add default scheme
    QUrl noScheme = QGCNetworkHelper::ensureScheme(QUrl("//example.com/path"));
    QCOMPARE(noScheme.scheme(), QStringLiteral("https"));
    // Custom default scheme
    QUrl customScheme = QGCNetworkHelper::ensureScheme(QUrl("//example.com/path"), "http");
    QCOMPARE(customScheme.scheme(), QStringLiteral("http"));
    // Existing scheme preserved
    QUrl existingScheme = QGCNetworkHelper::ensureScheme(QUrl("http://example.com"), "https");
    QCOMPARE(existingScheme.scheme(), QStringLiteral("http"));
}

void QGCNetworkHelperTest::_testBuildUrlFromMap()
{
    QMap<QString, QString> params;
    params["key1"] = "value1";
    params["key2"] = "value2";
    QUrl url = QGCNetworkHelper::buildUrl("http://example.com/api", params);
    QVERIFY(url.isValid());
    QCOMPARE(url.scheme(), QStringLiteral("http"));
    QCOMPARE(url.host(), QStringLiteral("example.com"));
    QCOMPARE(url.path(), QStringLiteral("/api"));
    QVERIFY(url.hasQuery());
    QString query = url.query();
    QVERIFY(query.contains("key1=value1"));
    QVERIFY(query.contains("key2=value2"));
}

void QGCNetworkHelperTest::_testBuildUrlFromList()
{
    QList<QPair<QString, QString>> params = {
        {"key1", "value1"}, {"key1", "value2"},  // Duplicate key allowed with list
    };
    QUrl url = QGCNetworkHelper::buildUrl("http://example.com/api", params);
    QVERIFY(url.isValid());
    QString query = url.query();
    // List preserves order and allows duplicates
    QVERIFY(query.contains("key1=value1"));
    QVERIFY(query.contains("key1=value2"));
}

void QGCNetworkHelperTest::_testUrlWithoutQuery()
{
    QUrl original("http://example.com/path?key=value#fragment");
    QUrl result = QGCNetworkHelper::urlWithoutQuery(original);
    QVERIFY(result.isValid());
    QCOMPARE(result.scheme(), QStringLiteral("http"));
    QCOMPARE(result.host(), QStringLiteral("example.com"));
    QCOMPARE(result.path(), QStringLiteral("/path"));
    QVERIFY(!result.hasQuery());
    QVERIFY(result.fragment().isEmpty());
}

// ============================================================================
// Request Configuration Tests
// ============================================================================
void QGCNetworkHelperTest::_testDefaultUserAgent()
{
    QString userAgent = QGCNetworkHelper::defaultUserAgent();
    QVERIFY(!userAgent.isEmpty());
    // Should contain app name
    QVERIFY(userAgent.contains(QCoreApplication::applicationName()));
    // Should contain Qt version
    QVERIFY(userAgent.contains("Qt"));
}

void QGCNetworkHelperTest::_testRequestConfigDefaults()
{
    QGCNetworkHelper::RequestConfig config;
    QCOMPARE(config.timeoutMs, QGCNetworkHelper::kDefaultTimeoutMs);
    QVERIFY(config.allowRedirects);
    QVERIFY(config.http2Allowed);
    QVERIFY(config.cacheEnabled);
    QVERIFY(!config.backgroundRequest);
    QVERIFY(config.userAgent.isEmpty());
    QCOMPARE(config.accept, QStringLiteral("*/*"));
}

void QGCNetworkHelperTest::_testRequestConfigAttributes()
{
    QGCNetworkHelper::RequestConfig config;
    config.requestAttributes.append({
        QNetworkRequest::CacheLoadControlAttribute,
        QVariant::fromValue(QNetworkRequest::AlwaysNetwork),
    });

    const QNetworkRequest request =
        QGCNetworkHelper::createRequest(QUrl(QStringLiteral("https://example.com")), config);
    const QVariant attribute = request.attribute(QNetworkRequest::CacheLoadControlAttribute);
    QVERIFY(attribute.isValid());
    QCOMPARE(attribute.toInt(), static_cast<int>(QNetworkRequest::AlwaysNetwork));
}

void QGCNetworkHelperTest::_testRequestConfigKeepAlive()
{
    const QNetworkRequest defaultRequest = QGCNetworkHelper::createRequest(QUrl(QStringLiteral("https://example.com")));
    QVERIFY(!defaultRequest.attribute(QNetworkRequest::ConnectionCacheExpiryTimeoutSecondsAttribute).isValid());
    QCOMPARE(defaultRequest.tcpKeepAliveProbeCount(), 0);

    QGCNetworkHelper::RequestConfig config;
    config.connectionCacheExpirySecs = 300;
    config.tcpKeepAliveIdleSecs = 60;
    config.tcpKeepAliveIntervalSecs = 30;
    config.tcpKeepAliveProbeCount = 3;

    const QNetworkRequest request =
        QGCNetworkHelper::createRequest(QUrl(QStringLiteral("https://example.com")), config);
    QCOMPARE(request.attribute(QNetworkRequest::ConnectionCacheExpiryTimeoutSecondsAttribute).toInt(), 300);
    QCOMPARE(request.tcpKeepAliveIdleTimeBeforeProbes().count(), std::chrono::seconds::rep(60));
    QCOMPARE(request.tcpKeepAliveIntervalBetweenProbes().count(), std::chrono::seconds::rep(30));
    QCOMPARE(request.tcpKeepAliveProbeCount(), 3);
}

void QGCNetworkHelperTest::_testSetJsonHeaders()
{
    QNetworkRequest request(QUrl(QStringLiteral("https://example.com")));
    QGCNetworkHelper::setJsonHeaders(request);

    QCOMPARE(request.rawHeader("Accept"), QByteArray("application/json"));
    QCOMPARE(request.header(QNetworkRequest::ContentTypeHeader).toString(), QStringLiteral("application/json"));
}

void QGCNetworkHelperTest::_testCreateBasicAuthCredentials()
{
    const QString credentials = QGCNetworkHelper::createBasicAuthCredentials("user", "pass");
    QCOMPARE(credentials, QStringLiteral("dXNlcjpwYXNz"));
}

void QGCNetworkHelperTest::_testSetBasicAuthHeader()
{
    QNetworkRequest request(QUrl(QStringLiteral("https://example.com")));
    QGCNetworkHelper::setBasicAuth(request, QStringLiteral("user"), QStringLiteral("pass"));

    QCOMPARE(request.rawHeader("Authorization"), QByteArray("Basic dXNlcjpwYXNz"));
    QGCNetworkHelper::setBasicAuth(request, QStringLiteral("User"), QStringLiteral("PaSs"));
    QCOMPARE(request.rawHeader("Authorization"), QByteArray("Basic VXNlcjpQYVNz"));
}

void QGCNetworkHelperTest::_testSetBearerTokenHeader()
{
    QNetworkRequest request(QUrl(QStringLiteral("https://example.com")));
    QGCNetworkHelper::setBearerToken(request, QStringLiteral("CaseSensitive"));
    QCOMPARE(request.rawHeader("Authorization"), QByteArray("Bearer ") + QByteArray("CaseSensitive"));
    QGCNetworkHelper::setBearerToken(request, QStringLiteral("abc123"));

    QCOMPARE(request.rawHeader("Authorization"), QByteArray("Bearer abc123"));
}

void QGCNetworkHelperTest::_testCreateNetworkManager()
{
    QObject owner;
    auto* manager = QGCNetworkHelper::createNetworkManager(&owner);
    QVERIFY(manager);
    QCOMPARE(manager->parent(), &owner);
}

void QGCNetworkHelperTest::_testLooksLikeJson()
{
    QVERIFY(QGCNetworkHelper::looksLikeJson("{}"));
    QVERIFY(QGCNetworkHelper::looksLikeJson(" [1, 2, 3]"));
    QVERIFY(QGCNetworkHelper::looksLikeJson("\n\t{\"a\":1}"));
    QVERIFY(!QGCNetworkHelper::looksLikeJson(""));
    QVERIFY(!QGCNetworkHelper::looksLikeJson("  \n\t"));
    QVERIFY(!QGCNetworkHelper::looksLikeJson("<xml></xml>"));
}

void QGCNetworkHelperTest::_testErrorMessageNullReply()
{
    QCOMPARE(QGCNetworkHelper::errorMessage(nullptr), QStringLiteral("No reply"));
}

void QGCNetworkHelperTest::_testRedirectUrlResolvesRelativeTarget()
{
    TestFixtures::NetworkReplyFixture reply(QUrl(QStringLiteral("https://example.com/api/v1/source")));
    reply.setRedirectTarget(QUrl(QStringLiteral("../next")));

    const QUrl resolved = QGCNetworkHelper::redirectUrl(&reply);
    QCOMPARE(resolved, QUrl(QStringLiteral("https://example.com/api/next")));
}

void QGCNetworkHelperTest::_testErrorMessagePrefersNetworkErrorOverHttpStatus()
{
    TestFixtures::NetworkReplyFixture reply(QUrl(QStringLiteral("https://example.com/api")));
    reply.setHttpStatus(404);
    reply.setNetworkError(QNetworkReply::ConnectionRefusedError, QStringLiteral("connection refused"));

    QCOMPARE(QGCNetworkHelper::errorMessage(&reply), QStringLiteral("connection refused"));
}

void QGCNetworkHelperTest::_testParseJsonValid()
{
    QJsonParseError error{};
    const QJsonDocument doc = QGCNetworkHelper::parseJson(R"({"value":42})", &error);

    QVERIFY(!doc.isNull());
    QCOMPARE(error.error, QJsonParseError::NoError);
    QCOMPARE(doc.object().value(QStringLiteral("value")).toInt(), 42);
}

void QGCNetworkHelperTest::_testParseJsonInvalid()
{
    QJsonParseError error{};
    expectLogMessage("Utilities.QGCNetworkHelper", QtWarningMsg, QRegularExpression("JSON parse error"));
    const QJsonDocument doc = QGCNetworkHelper::parseJson("{ invalid json", &error);
    verifyExpectedLogMessage();

    QVERIFY(doc.isNull());
    QVERIFY(error.error != QJsonParseError::NoError);
}

void QGCNetworkHelperTest::_testParseJsonReplyNull()
{
    QJsonParseError error{};
    const QJsonDocument doc = QGCNetworkHelper::parseJsonReply(nullptr, &error);

    QVERIFY(doc.isNull());
    QCOMPARE(error.error, QJsonParseError::UnterminatedObject);
    QCOMPARE(error.offset, 0);
}

void QGCNetworkHelperTest::_testReplyHelpersNullReply()
{
    QCOMPARE(QGCNetworkHelper::httpStatusCode(nullptr), -1);
    QVERIFY(QGCNetworkHelper::redirectUrl(nullptr).isEmpty());
    QVERIFY(!QGCNetworkHelper::isSuccess(nullptr));
    QVERIFY(!QGCNetworkHelper::isRedirect(nullptr));
    QVERIFY(QGCNetworkHelper::contentType(nullptr).isEmpty());
    QCOMPARE(QGCNetworkHelper::contentLength(nullptr), -1);
    QVERIFY(!QGCNetworkHelper::isJsonResponse(nullptr));
}

// ============================================================================
// Network Availability Tests
// ============================================================================
void QGCNetworkHelperTest::_testIsNetworkAvailable()
{
    // Smoke test: network availability query completes without crash
    // Result depends on system state, so we only verify it returns a valid bool
    const bool available = QGCNetworkHelper::isNetworkAvailable();
    Q_UNUSED(available);
}

void QGCNetworkHelperTest::_testConnectionTypeName()
{
    QCOMPARE(QGCNetworkHelper::connectionTypeName(QGCNetworkHelper::ConnectionType::None), QStringLiteral("None"));
    QCOMPARE(QGCNetworkHelper::connectionTypeName(QGCNetworkHelper::ConnectionType::Unknown),
             QStringLiteral("Unknown"));
    QCOMPARE(QGCNetworkHelper::connectionTypeName(QGCNetworkHelper::ConnectionType::Ethernet),
             QStringLiteral("Ethernet"));
    QCOMPARE(QGCNetworkHelper::connectionTypeName(QGCNetworkHelper::ConnectionType::WiFi), QStringLiteral("WiFi"));
    QCOMPARE(QGCNetworkHelper::connectionTypeName(QGCNetworkHelper::ConnectionType::Cellular),
             QStringLiteral("Cellular"));
    QCOMPARE(QGCNetworkHelper::connectionTypeName(QGCNetworkHelper::ConnectionType::Bluetooth),
             QStringLiteral("Bluetooth"));
}

// ============================================================================
// SSL Tests
// ============================================================================
void QGCNetworkHelperTest::_testSslVersion()
{
    const QString version = QGCNetworkHelper::sslVersion();
    // Version string might be empty if SSL not available
    // If SSL is available, version should be non-empty
    if (QGCNetworkHelper::isSslAvailable()) {
        QVERIFY(!version.isEmpty());
    }
}

UT_REGISTER_TEST(QGCNetworkHelperTest, TestLabel::Unit, TestLabel::Utilities)
