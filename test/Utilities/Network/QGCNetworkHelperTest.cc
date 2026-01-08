#include "QGCNetworkHelperTest.h"
#include "QGCNetworkHelper.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QMap>
#include <QtTest/QTest>

// ============================================================================
// HTTP Status Code Helpers Tests
// ============================================================================

void QGCNetworkHelperTest::_testClassifyHttpStatusInformational()
{
    QCOMPARE(QGCNetworkHelper::classifyHttpStatus(100), QGCNetworkHelper::HttpStatusClass::Informational);
    QCOMPARE(QGCNetworkHelper::classifyHttpStatus(101), QGCNetworkHelper::HttpStatusClass::Informational);
    QCOMPARE(QGCNetworkHelper::classifyHttpStatus(199), QGCNetworkHelper::HttpStatusClass::Informational);
}

void QGCNetworkHelperTest::_testClassifyHttpStatusSuccess()
{
    QCOMPARE(QGCNetworkHelper::classifyHttpStatus(200), QGCNetworkHelper::HttpStatusClass::Success);
    QCOMPARE(QGCNetworkHelper::classifyHttpStatus(201), QGCNetworkHelper::HttpStatusClass::Success);
    QCOMPARE(QGCNetworkHelper::classifyHttpStatus(204), QGCNetworkHelper::HttpStatusClass::Success);
    QCOMPARE(QGCNetworkHelper::classifyHttpStatus(299), QGCNetworkHelper::HttpStatusClass::Success);
}

void QGCNetworkHelperTest::_testClassifyHttpStatusRedirection()
{
    QCOMPARE(QGCNetworkHelper::classifyHttpStatus(300), QGCNetworkHelper::HttpStatusClass::Redirection);
    QCOMPARE(QGCNetworkHelper::classifyHttpStatus(301), QGCNetworkHelper::HttpStatusClass::Redirection);
    QCOMPARE(QGCNetworkHelper::classifyHttpStatus(302), QGCNetworkHelper::HttpStatusClass::Redirection);
    QCOMPARE(QGCNetworkHelper::classifyHttpStatus(304), QGCNetworkHelper::HttpStatusClass::Redirection);
    QCOMPARE(QGCNetworkHelper::classifyHttpStatus(399), QGCNetworkHelper::HttpStatusClass::Redirection);
}

void QGCNetworkHelperTest::_testClassifyHttpStatusClientError()
{
    QCOMPARE(QGCNetworkHelper::classifyHttpStatus(400), QGCNetworkHelper::HttpStatusClass::ClientError);
    QCOMPARE(QGCNetworkHelper::classifyHttpStatus(401), QGCNetworkHelper::HttpStatusClass::ClientError);
    QCOMPARE(QGCNetworkHelper::classifyHttpStatus(403), QGCNetworkHelper::HttpStatusClass::ClientError);
    QCOMPARE(QGCNetworkHelper::classifyHttpStatus(404), QGCNetworkHelper::HttpStatusClass::ClientError);
    QCOMPARE(QGCNetworkHelper::classifyHttpStatus(499), QGCNetworkHelper::HttpStatusClass::ClientError);
}

void QGCNetworkHelperTest::_testClassifyHttpStatusServerError()
{
    QCOMPARE(QGCNetworkHelper::classifyHttpStatus(500), QGCNetworkHelper::HttpStatusClass::ServerError);
    QCOMPARE(QGCNetworkHelper::classifyHttpStatus(501), QGCNetworkHelper::HttpStatusClass::ServerError);
    QCOMPARE(QGCNetworkHelper::classifyHttpStatus(502), QGCNetworkHelper::HttpStatusClass::ServerError);
    QCOMPARE(QGCNetworkHelper::classifyHttpStatus(503), QGCNetworkHelper::HttpStatusClass::ServerError);
    QCOMPARE(QGCNetworkHelper::classifyHttpStatus(599), QGCNetworkHelper::HttpStatusClass::ServerError);
}

void QGCNetworkHelperTest::_testClassifyHttpStatusUnknown()
{
    QCOMPARE(QGCNetworkHelper::classifyHttpStatus(0), QGCNetworkHelper::HttpStatusClass::Unknown);
    QCOMPARE(QGCNetworkHelper::classifyHttpStatus(-1), QGCNetworkHelper::HttpStatusClass::Unknown);
    QCOMPARE(QGCNetworkHelper::classifyHttpStatus(99), QGCNetworkHelper::HttpStatusClass::Unknown);
    QCOMPARE(QGCNetworkHelper::classifyHttpStatus(600), QGCNetworkHelper::HttpStatusClass::Unknown);
    QCOMPARE(QGCNetworkHelper::classifyHttpStatus(1000), QGCNetworkHelper::HttpStatusClass::Unknown);
}

void QGCNetworkHelperTest::_testIsHttpSuccess()
{
    QVERIFY(QGCNetworkHelper::isHttpSuccess(200));
    QVERIFY(QGCNetworkHelper::isHttpSuccess(201));
    QVERIFY(QGCNetworkHelper::isHttpSuccess(204));
    QVERIFY(QGCNetworkHelper::isHttpSuccess(299));

    QVERIFY(!QGCNetworkHelper::isHttpSuccess(100));
    QVERIFY(!QGCNetworkHelper::isHttpSuccess(300));
    QVERIFY(!QGCNetworkHelper::isHttpSuccess(404));
    QVERIFY(!QGCNetworkHelper::isHttpSuccess(500));
}

void QGCNetworkHelperTest::_testIsHttpRedirect()
{
    QVERIFY(QGCNetworkHelper::isHttpRedirect(300));
    QVERIFY(QGCNetworkHelper::isHttpRedirect(301));
    QVERIFY(QGCNetworkHelper::isHttpRedirect(302));
    QVERIFY(QGCNetworkHelper::isHttpRedirect(304));
    QVERIFY(QGCNetworkHelper::isHttpRedirect(399));

    QVERIFY(!QGCNetworkHelper::isHttpRedirect(200));
    QVERIFY(!QGCNetworkHelper::isHttpRedirect(400));
}

void QGCNetworkHelperTest::_testIsHttpClientError()
{
    QVERIFY(QGCNetworkHelper::isHttpClientError(400));
    QVERIFY(QGCNetworkHelper::isHttpClientError(401));
    QVERIFY(QGCNetworkHelper::isHttpClientError(403));
    QVERIFY(QGCNetworkHelper::isHttpClientError(404));
    QVERIFY(QGCNetworkHelper::isHttpClientError(499));

    QVERIFY(!QGCNetworkHelper::isHttpClientError(200));
    QVERIFY(!QGCNetworkHelper::isHttpClientError(500));
}

void QGCNetworkHelperTest::_testIsHttpServerError()
{
    QVERIFY(QGCNetworkHelper::isHttpServerError(500));
    QVERIFY(QGCNetworkHelper::isHttpServerError(501));
    QVERIFY(QGCNetworkHelper::isHttpServerError(502));
    QVERIFY(QGCNetworkHelper::isHttpServerError(503));
    QVERIFY(QGCNetworkHelper::isHttpServerError(599));

    QVERIFY(!QGCNetworkHelper::isHttpServerError(200));
    QVERIFY(!QGCNetworkHelper::isHttpServerError(404));
}

void QGCNetworkHelperTest::_testHttpStatusText()
{
    // Common status codes
    QCOMPARE(QGCNetworkHelper::httpStatusText(200), QStringLiteral("OK"));
    QCOMPARE(QGCNetworkHelper::httpStatusText(201), QStringLiteral("Created"));
    QCOMPARE(QGCNetworkHelper::httpStatusText(204), QStringLiteral("No Content"));
    QCOMPARE(QGCNetworkHelper::httpStatusText(301), QStringLiteral("Moved Permanently"));
    QCOMPARE(QGCNetworkHelper::httpStatusText(302), QStringLiteral("Found"));
    QCOMPARE(QGCNetworkHelper::httpStatusText(304), QStringLiteral("Not Modified"));
    QCOMPARE(QGCNetworkHelper::httpStatusText(400), QStringLiteral("Bad Request"));
    QCOMPARE(QGCNetworkHelper::httpStatusText(401), QStringLiteral("Unauthorized"));
    QCOMPARE(QGCNetworkHelper::httpStatusText(403), QStringLiteral("Forbidden"));
    QCOMPARE(QGCNetworkHelper::httpStatusText(404), QStringLiteral("Not Found"));
    QCOMPARE(QGCNetworkHelper::httpStatusText(500), QStringLiteral("Internal Server Error"));
    QCOMPARE(QGCNetworkHelper::httpStatusText(502), QStringLiteral("Bad Gateway"));
    QCOMPARE(QGCNetworkHelper::httpStatusText(503), QStringLiteral("Service Unavailable"));

    // Unknown status should return formatted message
    QVERIFY(QGCNetworkHelper::httpStatusText(999).contains("999"));
}

void QGCNetworkHelperTest::_testHttpStatusTextFromEnum()
{
    using HttpStatusCode = QGCNetworkHelper::HttpStatusCode;

    // Test Qt HttpStatusCode enum overload
    QCOMPARE(QGCNetworkHelper::httpStatusText(HttpStatusCode::Continue), QStringLiteral("Continue"));
    QCOMPARE(QGCNetworkHelper::httpStatusText(HttpStatusCode::SwitchingProtocols), QStringLiteral("Switching Protocols"));
    QCOMPARE(QGCNetworkHelper::httpStatusText(HttpStatusCode::Ok), QStringLiteral("OK"));
    QCOMPARE(QGCNetworkHelper::httpStatusText(HttpStatusCode::Created), QStringLiteral("Created"));
    QCOMPARE(QGCNetworkHelper::httpStatusText(HttpStatusCode::Accepted), QStringLiteral("Accepted"));
    QCOMPARE(QGCNetworkHelper::httpStatusText(HttpStatusCode::NoContent), QStringLiteral("No Content"));
    QCOMPARE(QGCNetworkHelper::httpStatusText(HttpStatusCode::MovedPermanently), QStringLiteral("Moved Permanently"));
    QCOMPARE(QGCNetworkHelper::httpStatusText(HttpStatusCode::Found), QStringLiteral("Found"));
    QCOMPARE(QGCNetworkHelper::httpStatusText(HttpStatusCode::NotModified), QStringLiteral("Not Modified"));
    QCOMPARE(QGCNetworkHelper::httpStatusText(HttpStatusCode::BadRequest), QStringLiteral("Bad Request"));
    QCOMPARE(QGCNetworkHelper::httpStatusText(HttpStatusCode::Unauthorized), QStringLiteral("Unauthorized"));
    QCOMPARE(QGCNetworkHelper::httpStatusText(HttpStatusCode::Forbidden), QStringLiteral("Forbidden"));
    QCOMPARE(QGCNetworkHelper::httpStatusText(HttpStatusCode::NotFound), QStringLiteral("Not Found"));
    QCOMPARE(QGCNetworkHelper::httpStatusText(HttpStatusCode::MethodNotAllowed), QStringLiteral("Method Not Allowed"));
    QCOMPARE(QGCNetworkHelper::httpStatusText(HttpStatusCode::RequestTimeout), QStringLiteral("Request Timeout"));
    QCOMPARE(QGCNetworkHelper::httpStatusText(HttpStatusCode::TooManyRequests), QStringLiteral("Too Many Requests"));
    QCOMPARE(QGCNetworkHelper::httpStatusText(HttpStatusCode::InternalServerError), QStringLiteral("Internal Server Error"));
    QCOMPARE(QGCNetworkHelper::httpStatusText(HttpStatusCode::BadGateway), QStringLiteral("Bad Gateway"));
    QCOMPARE(QGCNetworkHelper::httpStatusText(HttpStatusCode::ServiceUnavailable), QStringLiteral("Service Unavailable"));
    QCOMPARE(QGCNetworkHelper::httpStatusText(HttpStatusCode::GatewayTimeout), QStringLiteral("Gateway Timeout"));
}

void QGCNetworkHelperTest::_testHttpStatusCodeEnumRoundTrip()
{
    using HttpStatusCode = QGCNetworkHelper::HttpStatusCode;

    // Verify that int and enum overloads produce consistent results
    QCOMPARE(QGCNetworkHelper::httpStatusText(200), QGCNetworkHelper::httpStatusText(HttpStatusCode::Ok));
    QCOMPARE(QGCNetworkHelper::httpStatusText(201), QGCNetworkHelper::httpStatusText(HttpStatusCode::Created));
    QCOMPARE(QGCNetworkHelper::httpStatusText(204), QGCNetworkHelper::httpStatusText(HttpStatusCode::NoContent));
    QCOMPARE(QGCNetworkHelper::httpStatusText(301), QGCNetworkHelper::httpStatusText(HttpStatusCode::MovedPermanently));
    QCOMPARE(QGCNetworkHelper::httpStatusText(302), QGCNetworkHelper::httpStatusText(HttpStatusCode::Found));
    QCOMPARE(QGCNetworkHelper::httpStatusText(304), QGCNetworkHelper::httpStatusText(HttpStatusCode::NotModified));
    QCOMPARE(QGCNetworkHelper::httpStatusText(400), QGCNetworkHelper::httpStatusText(HttpStatusCode::BadRequest));
    QCOMPARE(QGCNetworkHelper::httpStatusText(401), QGCNetworkHelper::httpStatusText(HttpStatusCode::Unauthorized));
    QCOMPARE(QGCNetworkHelper::httpStatusText(403), QGCNetworkHelper::httpStatusText(HttpStatusCode::Forbidden));
    QCOMPARE(QGCNetworkHelper::httpStatusText(404), QGCNetworkHelper::httpStatusText(HttpStatusCode::NotFound));
    QCOMPARE(QGCNetworkHelper::httpStatusText(500), QGCNetworkHelper::httpStatusText(HttpStatusCode::InternalServerError));
    QCOMPARE(QGCNetworkHelper::httpStatusText(502), QGCNetworkHelper::httpStatusText(HttpStatusCode::BadGateway));
    QCOMPARE(QGCNetworkHelper::httpStatusText(503), QGCNetworkHelper::httpStatusText(HttpStatusCode::ServiceUnavailable));

    // Verify static_cast round-trip
    QCOMPARE(static_cast<int>(HttpStatusCode::Ok), 200);
    QCOMPARE(static_cast<int>(HttpStatusCode::NotFound), 404);
    QCOMPARE(static_cast<int>(HttpStatusCode::InternalServerError), 500);
}

// ============================================================================
// HTTP Methods Tests
// ============================================================================

void QGCNetworkHelperTest::_testHttpMethodName()
{
    QCOMPARE(QGCNetworkHelper::httpMethodName(QGCNetworkHelper::HttpMethod::Get), QStringLiteral("GET"));
    QCOMPARE(QGCNetworkHelper::httpMethodName(QGCNetworkHelper::HttpMethod::Post), QStringLiteral("POST"));
    QCOMPARE(QGCNetworkHelper::httpMethodName(QGCNetworkHelper::HttpMethod::Put), QStringLiteral("PUT"));
    QCOMPARE(QGCNetworkHelper::httpMethodName(QGCNetworkHelper::HttpMethod::Delete), QStringLiteral("DELETE"));
    QCOMPARE(QGCNetworkHelper::httpMethodName(QGCNetworkHelper::HttpMethod::Head), QStringLiteral("HEAD"));
    QCOMPARE(QGCNetworkHelper::httpMethodName(QGCNetworkHelper::HttpMethod::Options), QStringLiteral("OPTIONS"));
    QCOMPARE(QGCNetworkHelper::httpMethodName(QGCNetworkHelper::HttpMethod::Patch), QStringLiteral("PATCH"));
    QCOMPARE(QGCNetworkHelper::httpMethodName(QGCNetworkHelper::HttpMethod::Connect), QStringLiteral("CONNECT"));
    QCOMPARE(QGCNetworkHelper::httpMethodName(QGCNetworkHelper::HttpMethod::Trace), QStringLiteral("TRACE"));
}

void QGCNetworkHelperTest::_testParseHttpMethod()
{
    QCOMPARE(QGCNetworkHelper::parseHttpMethod("GET"), QGCNetworkHelper::HttpMethod::Get);
    QCOMPARE(QGCNetworkHelper::parseHttpMethod("POST"), QGCNetworkHelper::HttpMethod::Post);
    QCOMPARE(QGCNetworkHelper::parseHttpMethod("PUT"), QGCNetworkHelper::HttpMethod::Put);
    QCOMPARE(QGCNetworkHelper::parseHttpMethod("DELETE"), QGCNetworkHelper::HttpMethod::Delete);
    QCOMPARE(QGCNetworkHelper::parseHttpMethod("HEAD"), QGCNetworkHelper::HttpMethod::Head);
    QCOMPARE(QGCNetworkHelper::parseHttpMethod("OPTIONS"), QGCNetworkHelper::HttpMethod::Options);
    QCOMPARE(QGCNetworkHelper::parseHttpMethod("PATCH"), QGCNetworkHelper::HttpMethod::Patch);
    QCOMPARE(QGCNetworkHelper::parseHttpMethod("CONNECT"), QGCNetworkHelper::HttpMethod::Connect);
    QCOMPARE(QGCNetworkHelper::parseHttpMethod("TRACE"), QGCNetworkHelper::HttpMethod::Trace);
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
    QVERIFY(!QGCNetworkHelper::isValidUrl(QUrl()));  // Empty URL
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
        {"key1", "value1"},
        {"key1", "value2"},  // Duplicate key allowed with list
    };

    QUrl url = QGCNetworkHelper::buildUrl("http://example.com/api", params);
    QVERIFY(url.isValid());

    QString query = url.query();
    // List preserves order and allows duplicates
    QVERIFY(query.contains("key1=value1"));
    QVERIFY(query.contains("key1=value2"));
}

void QGCNetworkHelperTest::_testUrlFileName()
{
    QCOMPARE(QGCNetworkHelper::urlFileName(QUrl("http://example.com/path/to/file.txt")),
             QStringLiteral("file.txt"));
    QCOMPARE(QGCNetworkHelper::urlFileName(QUrl("http://example.com/file.zip")),
             QStringLiteral("file.zip"));
    QCOMPARE(QGCNetworkHelper::urlFileName(QUrl("http://example.com/")),
             QStringLiteral("/"));
    QCOMPARE(QGCNetworkHelper::urlFileName(QUrl("http://example.com/path/")),
             QStringLiteral("/path/"));
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

// ============================================================================
// Network Availability Tests
// ============================================================================

void QGCNetworkHelperTest::_testIsNetworkAvailable()
{
    // Just verify it returns without crashing
    // Result depends on actual network state
    (void)QGCNetworkHelper::isNetworkAvailable();
    QVERIFY(true);
}

void QGCNetworkHelperTest::_testConnectionTypeName()
{
    QCOMPARE(QGCNetworkHelper::connectionTypeName(QGCNetworkHelper::ConnectionType::None),
             QStringLiteral("None"));
    QCOMPARE(QGCNetworkHelper::connectionTypeName(QGCNetworkHelper::ConnectionType::Unknown),
             QStringLiteral("Unknown"));
    QCOMPARE(QGCNetworkHelper::connectionTypeName(QGCNetworkHelper::ConnectionType::Ethernet),
             QStringLiteral("Ethernet"));
    QCOMPARE(QGCNetworkHelper::connectionTypeName(QGCNetworkHelper::ConnectionType::WiFi),
             QStringLiteral("WiFi"));
    QCOMPARE(QGCNetworkHelper::connectionTypeName(QGCNetworkHelper::ConnectionType::Cellular),
             QStringLiteral("Cellular"));
    QCOMPARE(QGCNetworkHelper::connectionTypeName(QGCNetworkHelper::ConnectionType::Bluetooth),
             QStringLiteral("Bluetooth"));
}

// ============================================================================
// SSL Tests
// ============================================================================

void QGCNetworkHelperTest::_testIsSslAvailable()
{
    // Just verify it returns without crashing
    // SSL availability depends on system configuration
    (void)QGCNetworkHelper::isSslAvailable();
    QVERIFY(true);
}

void QGCNetworkHelperTest::_testSslVersion()
{
    QString version = QGCNetworkHelper::sslVersion();
    // Version string might be empty if SSL not available, but shouldn't crash
    (void)version;
    QVERIFY(true);
}
