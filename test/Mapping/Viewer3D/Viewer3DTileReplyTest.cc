/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "Viewer3DTileReplyTest.h"

#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>

#include "QGCMapUrlEngine.h"
#include "TileReplyNetworkMock.h"
#include "UnitTestTileGenerator.h"
#include "Viewer3DTileReply.h"

namespace {

/// Any map provider works: under unit tests a cache miss is served by UnitTestTileGenerator unless a miss is forced
QString viewer3DMapType()
{
    const QStringList elevationTypes = UrlFactory::getElevationProviderTypes();
    for (const QString& type : UrlFactory::getProviderTypes()) {
        if (!elevationTypes.contains(type)) {
            return type;
        }
    }
    return QString();
}

}  // namespace

void Viewer3DTileReplyTest::initTestCase()
{
    UnitTest::initTestCase();
    // The tests fetch tiles that the tile cache of the process must not have yet
    QVERIFY(UnitTestTileGenerator::resetTileCache());
}

void Viewer3DTileReplyTest::_testOutcome_data()
{
    QTest::addColumn<QString>("providerType");
    QTest::addColumn<int>("tileX");  // a tile of its own for every row
    QTest::addColumn<bool>("cached");
    QTest::addColumn<QByteArray>("serverBody");
    QTest::addColumn<QString>("expectedSignal");

    QTest::newRow("cached tile") << viewer3DMapType() << 1 << true << QByteArray() << QStringLiteral("tileDone");
    QTest::newRow("server tile") << viewer3DMapType() << 2 << false << tileReplyPngBody() << QStringLiteral("tileDone");
    // Viewer3DTileQuery reloads the map one zoom level out when a provider has no tile
    QTest::newRow("no tile at this zoom")
        << QStringLiteral("Bing Hybrid") << 3 << false << tileReplyBingPlaceholder() << QStringLiteral("tileEmpty");
    QTest::newRow("empty server answer") << viewer3DMapType() << 4 << false << QByteArray()
                                         << QStringLiteral("tileGiveUp");
}

void Viewer3DTileReplyTest::_testOutcome()
{
    QFETCH(QString, providerType);
    QFETCH(int, tileX);
    QFETCH(bool, cached);
    QFETCH(QByteArray, serverBody);
    QFETCH(QString, expectedSignal);

    TileReplyMockNam nam;
    nam.body = serverBody;
    UnitTestTileGenerator::setForcedMissCount(cached ? 0 : 1);
    const auto guard = qScopeGuard([] { UnitTestTileGenerator::setForcedMissCount(0); });

    if (expectedSignal == QStringLiteral("tileGiveUp")) {
        expectLogMessage("Viewer3d.Viewer3DTileReply", QtWarningMsg,
                         QRegularExpression(QStringLiteral("Request error for tile")));
    }

    Viewer3DTileReply tileReply(10, tileX, 2, UrlFactory::getMapIdFromProviderType(providerType), &nam);
    QStringList emitted;
    Viewer3DTileInfo tile;
    connect(&tileReply, &Viewer3DTileReply::tileDone, this, [&](const Viewer3DTileInfo& info) {
        emitted.append(QStringLiteral("tileDone"));
        tile = info;
    });
    connect(&tileReply, &Viewer3DTileReply::tileEmpty, this, [&](const Viewer3DTileInfo& info) {
        emitted.append(QStringLiteral("tileEmpty"));
        tile = info;
    });
    connect(&tileReply, &Viewer3DTileReply::tileGiveUp, this, [&](const Viewer3DTileInfo& info) {
        emitted.append(QStringLiteral("tileGiveUp"));
        tile = info;
    });

    QTRY_COMPARE_WITH_TIMEOUT(emitted, QStringList{expectedSignal}, TestTimeout::mediumMs());
    QCOMPARE(tile.x, tileX);
    QCOMPARE(tile.zoomLevel, 10);
    QCOMPARE(tile.data.isEmpty(), expectedSignal != QStringLiteral("tileDone"));
    QCOMPARE(nam.requestCount, cached ? 0 : 1);
    if (expectedSignal == QStringLiteral("tileGiveUp")) {
        verifyExpectedLogMessage();
    }
}

// Regression test for a destruction-order crash: Viewer3DTileQuery creates its
// QNetworkAccessManager child before its Viewer3DTileReply children. QObject
// deletes children in creation order, so the manager (and the QNetworkReply
// objects it owns) dies first, and the tile reply must not touch the network
// reply it was waiting for.
void Viewer3DTileReplyTest::_testDestructionOrderNoCrash()
{
    UnitTestTileGenerator::setForcedMissCount(1);
    const auto guard = qScopeGuard([] { UnitTestTileGenerator::setForcedMissCount(0); });

    QObject* parentObj = new QObject();

    // Manager created first: it is destroyed before the tile reply below
    auto* networkManager = new TileReplyMockNam(parentObj);
    networkManager->holdReplies = true;

    (void) new Viewer3DTileReply(10, 5, 2, UrlFactory::getMapIdFromProviderType(viewer3DMapType()), networkManager,
                                 parentObj);
    QTRY_COMPARE_WITH_TIMEOUT(networkManager->requestCount, 1, TestTimeout::mediumMs());

    // Deletes the manager (and its QNetworkReply children) first, then the tile reply
    delete parentObj;
}

UT_REGISTER_TEST(Viewer3DTileReplyTest, TestLabel::Unit)
