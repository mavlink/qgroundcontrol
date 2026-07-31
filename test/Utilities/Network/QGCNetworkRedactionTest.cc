#include "QGCNetworkRedactionTest.h"

#include "QGCNetworkHelper.h"

#include <QtCore/QUrl>
#include <QtCore/QUrlQuery>

void QGCNetworkRedactionTest::_testPreservesStreamIdentity()
{
    QCOMPARE(QGCNetworkHelper::redactedUrlForLogging(QStringLiteral("rtsp://camera.example:554/axis-media/media.amp")),
             QStringLiteral("rtsp://camera.example:554/axis-media/media.amp"));
    QCOMPARE(QGCNetworkHelper::redactedUrlForLogging(QStringLiteral("udp://0.0.0.0:5600")),
             QStringLiteral("udp://0.0.0.0:5600"));
}

void QGCNetworkRedactionTest::_testRemovesUserInfo()
{
    const QString result = QGCNetworkHelper::redactedUrlForLogging(
        QStringLiteral("https://pilot%40ops:secret%2Fvalue@example.com:8443/video%20feed"));

    QCOMPARE(result, QStringLiteral("https://example.com:8443/video%20feed"));
    QVERIFY(!result.contains(QStringLiteral("pilot")));
    QVERIFY(!result.contains(QStringLiteral("secret")));
}

void QGCNetworkRedactionTest::_testRedactsQueryValues()
{
    const QString result = QGCNetworkHelper::redactedUrlForLogging(
        QStringLiteral("https://example.com/video?token=abc123&mode=low-latency#session"));
    const QUrl resultUrl(result);
    const QUrlQuery resultQuery(resultUrl);

    QCOMPARE(resultUrl.path(), QStringLiteral("/video"));
    QCOMPARE(resultQuery.queryItemValue(QStringLiteral("token")), QStringLiteral("REDACTED"));
    QCOMPARE(resultQuery.queryItemValue(QStringLiteral("mode")), QStringLiteral("REDACTED"));
    QCOMPARE(resultUrl.fragment(), QStringLiteral("REDACTED"));
    QVERIFY(!result.contains(QStringLiteral("abc123")));
    QVERIFY(!result.contains(QStringLiteral("low-latency")));
    QVERIFY(!result.contains(QStringLiteral("session")));
}

void QGCNetworkRedactionTest::_testHandlesRelativeAndInvalidInput()
{
    QCOMPARE(QGCNetworkHelper::redactedUrlForLogging(QStringLiteral("5600")), QStringLiteral("5600"));
    QCOMPARE(QGCNetworkHelper::redactedUrlForLogging(QStringLiteral("camera.local:5600")),
             QStringLiteral("camera.local:5600"));
    QCOMPARE(QGCNetworkHelper::redactedUrlForLogging(QString()), QStringLiteral("<empty-url>"));
    QCOMPARE(QGCNetworkHelper::redactedUrlForLogging(QStringLiteral("http://[invalid")),
             QStringLiteral("<invalid-url>"));
}

void QGCNetworkRedactionTest::_testQUrlOverload()
{
    const QUrl sourceUrl(QStringLiteral("rtsp://pilot:secret@camera.example:8554/live?token=abc123"));
    const QString result = QGCNetworkHelper::redactedUrlForLogging(sourceUrl);

    QCOMPARE(QUrl(result).path(), QStringLiteral("/live"));
    QVERIFY(!result.contains(QStringLiteral("pilot")));
    QVERIFY(!result.contains(QStringLiteral("secret")));
    QVERIFY(!result.contains(QStringLiteral("abc123")));
}

UT_REGISTER_TEST(QGCNetworkRedactionTest, TestLabel::Unit, TestLabel::Utilities)
