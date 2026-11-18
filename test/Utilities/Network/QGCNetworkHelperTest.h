#pragma once

#include "UnitTest.h"

/// Tests for QGCNetworkHelper (networking utility functions)
class QGCNetworkHelperTest : public UnitTest
{
    Q_OBJECT

private slots:
    // HTTP status code helpers tests
    void _testHttpStatusClass_data();
    void _testHttpStatusClass();
    void _testHttpStatusText_data();
    void _testHttpStatusText();
    void _testHttpStatusTextUnknown();

    // HTTP methods tests
    void _testHttpMethodNames_data();
    void _testHttpMethodNames();
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
    void _testUrlWithoutQuery();

    // Request configuration tests
    void _testDefaultUserAgent();
    void _testRequestConfigDefaults();
    void _testRequestConfigAttributes();
    void _testRequestConfigKeepAlive();
    void _testSetJsonHeaders();
    void _testCreateBasicAuthCredentials();
    void _testSetBasicAuthHeader();
    void _testSetBearerTokenHeader();
    void _testCreateNetworkManager();
    void _testLooksLikeJson();
    void _testErrorMessageNullReply();
    void _testRedirectUrlResolvesRelativeTarget();
    void _testErrorMessagePrefersNetworkErrorOverHttpStatus();
    void _testParseJsonValid();
    void _testParseJsonInvalid();
    void _testParseJsonReplyNull();
    void _testReplyHelpersNullReply();

    // Network availability tests
    void _testIsNetworkAvailable();
    void _testConnectionTypeName();

    // SSL tests
    void _testSslVersion();
};
