#include "QGeoMapReplyQGCTest.h"

#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtLocation/private/qgeotilespec_p.h>
#include <QtNetwork/QNetworkAccessManager>
#include <QtTest/QSignalSpy>

#include "QGeoMapReplyQGC.h"
#include "UnitTestTileGenerator.h"

namespace {

/// A tile of the plugin's first map type. Under unit tests a cache miss is served by UnitTestTileGenerator unless a
/// miss is forced.
QGeoTileSpec mapReplyTileSpec(int x)
{
    return QGeoTileSpec(QStringLiteral("QGroundControl"), 1, 13, x, 2867);
}

}  // namespace

void QGeoMapReplyQGCTest::_testTileDelivered()
{
    QNetworkAccessManager networkManager;
    QGeoTiledMapReplyQGC mapReply(&networkManager, mapReplyTileSpec(4400));
    QSignalSpy finishedSpy(&mapReply, &QGeoTiledMapReply::finished);

    mapReply.init();
    QVERIFY_SIGNAL_COUNT_WAIT(finishedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(mapReply.error(), QGeoTiledMapReply::NoError);
    QVERIFY(!mapReply.mapImageData().isEmpty());
    QCOMPARE(mapReply.mapImageFormat(), QStringLiteral("png"));
    QVERIFY(mapReply.isCached());
}

void QGeoMapReplyQGCTest::_testErrorReported()
{
    const UnitTestTileGenerator::ForcedMisses forcedMisses(1);

    // Without a network manager the cache miss cannot be fetched
    QGeoTiledMapReplyQGC mapReply(nullptr, mapReplyTileSpec(4401));
    QSignalSpy finishedSpy(&mapReply, &QGeoTiledMapReply::finished);

    expectLogMessage("QtLocationPlugin.QGeoTiledMapReplyQGC", QtWarningMsg,
                     QRegularExpression(QStringLiteral("NetworkError")));
    mapReply.init();
    QVERIFY_SIGNAL_COUNT_WAIT(finishedSpy, 1, TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    QCOMPARE(mapReply.error(), QGeoTiledMapReply::CommunicationError);
    QVERIFY(!mapReply.errorString().isEmpty());
}

void QGeoMapReplyQGCTest::_testAbortFinishesOnce()
{
    QNetworkAccessManager networkManager;
    QGeoTiledMapReplyQGC aborted(&networkManager, mapReplyTileSpec(4402));
    QSignalSpy abortedSpy(&aborted, &QGeoTiledMapReply::finished);
    aborted.init();
    aborted.abort();
    QCOMPARE(abortedSpy.count(), 1);

    // The cache worker runs lookups in order: once this one is answered, the aborted reply's tile has arrived too
    QGeoTiledMapReplyQGC sentinel(&networkManager, mapReplyTileSpec(4403));
    QSignalSpy sentinelSpy(&sentinel, &QGeoTiledMapReply::finished);
    sentinel.init();
    QVERIFY_SIGNAL_COUNT_WAIT(sentinelSpy, 1, TestTimeout::mediumMs());

    QCOMPARE(abortedSpy.count(), 1);
    QVERIFY(aborted.mapImageData().isEmpty());
}

UT_REGISTER_TEST(QGeoMapReplyQGCTest, TestLabel::Unit)
