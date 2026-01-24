#pragma once

#include "TestFixtures.h"

/// Tests for QGCNetworkHelper (networking utility functions).
/// Uses OfflineTest since it doesn't require a vehicle connection.
class QGCNetworkHelperTest : public OfflineTest
{
    Q_OBJECT

public:
    QGCNetworkHelperTest() = default;

private slots:
    // HTTP status code helpers tests
    void _testClassifyHttpStatusInformational();
    void _testClassifyHttpStatusSuccess();
    void _testClassifyHttpStatusRedirection();
    void _testClassifyHttpStatusClientError();
    void _testClassifyHttpStatusServerError();
    void _testClassifyHttpStatusUnknown();
    void _testIsHttpSuccess();
    void _testIsHttpRedirect();
    void _testIsHttpClientError();
    void _testIsHttpServerError();
    void _testHttpStatusText();
    void _testHttpStatusTextFromEnum();
    void _testHttpStatusCodeEnumRoundTrip();

    // HTTP methods tests
    void _testHttpMethodName();
    void _testParseHttpMethod();
    void _testParseHttpMethodCaseInsensitive();
    void _testParseHttpMethodUnknown();

    // URL utilities tests
    void _testIsValidUrl();
    void _testIsHttpUrl();
    void _testIsHttpsUrl();
    void _testNormalizeUrl();
    void _testEnsureScheme();
    void _testBuildUrlFromMap();
    void _testBuildUrlFromList();
    void _testUrlFileName();
    void _testUrlWithoutQuery();

    // Request configuration tests
    void _testDefaultUserAgent();
    void _testRequestConfigDefaults();

    // Network availability tests
    void _testIsNetworkAvailable();
    void _testConnectionTypeName();

    // SSL tests
    void _testIsSslAvailable();
    void _testSslVersion();
};
