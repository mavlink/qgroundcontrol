#include "QGCNetworkRedactionTest.h"

#include "QGCNetworkHelper.h"

void QGCNetworkRedactionTest::_testRedactedUrlForLogging()
{
    const QString sensitiveUrl =
        QStringLiteral("https://pilot:secret@example.com:8443/video%20feed?token=abc123#session");
    const QString redacted = QGCNetworkHelper::redactedUrlForLogging(sensitiveUrl);

    QCOMPARE(redacted, QStringLiteral("https://example.com:8443/<redacted>"));
    QVERIFY(!redacted.contains(QStringLiteral("pilot")));
    QVERIFY(!redacted.contains(QStringLiteral("secret")));
    QVERIFY(!redacted.contains(QStringLiteral("abc123")));
    QVERIFY(!redacted.contains(QStringLiteral("video")));
    QCOMPARE(QGCNetworkHelper::redactedUrlForLogging(QStringLiteral("udp://0.0.0.0:5600")),
             QStringLiteral("udp://0.0.0.0:5600"));
    QCOMPARE(QGCNetworkHelper::redactedUrlForLogging(QStringLiteral("https://example.com/?token=abc123")),
             QStringLiteral("https://example.com/"));
    QCOMPARE(QGCNetworkHelper::redactedUrlForLogging(QStringLiteral("https://[2001:db8::1]:8443/video?token=abc123")),
             QStringLiteral("https://[2001:db8::1]:8443/<redacted>"));

    const QString encodedUserInfo = QGCNetworkHelper::redactedUrlForLogging(
        QStringLiteral("https://pilot%40ops:secret%2Fvalue@example.com/video?token=abc123"));
    QCOMPARE(encodedUserInfo, QStringLiteral("https://example.com/<redacted>"));
    QVERIFY(!encodedUserInfo.contains(QStringLiteral("pilot")));
    QVERIFY(!encodedUserInfo.contains(QStringLiteral("secret")));
    QCOMPARE(QGCNetworkHelper::redactedUrlForLogging(QStringLiteral("token-only-value")),
             QStringLiteral("<invalid-url>"));
}
