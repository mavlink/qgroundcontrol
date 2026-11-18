#include "TerrainTileManagerTest.h"

#include <QtCore/QRegularExpression>
#include <QtPositioning/QGeoCoordinate>
#include <QtTest/QSignalSpy>

#include "Fixtures/RAIIFixtures.h"
#include "FlightMapSettings.h"
#include "SettingsManager.h"
#include "TerrainQueryInterface.h"
#include "TerrainTileManager.h"
#include "TileReplyNetworkMock.h"

// The setting can name a provider that this build lacks: the query fails, it is neither queued for ever nor a crash
void TerrainTileManagerTest::_testUnknownElevationProviderFailsTheQuery()
{
    TestFixtures::SettingsFixture saved;
    saved.setFactValue(SettingsManager::instance()->flightMapSettings()->elevationMapProvider(),
                       QStringLiteral("No Such Provider"));

    TileReplyMockNam nam;  // must outlive the manager, which borrows it
    TerrainTileManager manager(nullptr, &nam);
    TerrainQueryInterface query;
    QSignalSpy heightsSpy(&query, &TerrainQueryInterface::coordinateHeightsReceived);

    expectLogMessage("MapTiles.UrlFactory", QtWarningMsg, QRegularExpression(QStringLiteral("type not found:")));
    expectLogMessage("Terrain.TerrainTileManager", QtWarningMsg,
                     QRegularExpression(QStringLiteral("No elevation provider is named")));
    expectLogMessage("Terrain.TerrainTileManager", QtWarningMsg,
                     QRegularExpression(QStringLiteral("signalling failure due to internal error")));
    manager.addCoordinateQuery(&query, {QGeoCoordinate(-61.235, 97.415)});
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();

    QCOMPARE(heightsSpy.count(), 1);
    QVERIFY(!heightsSpy.constFirst().at(0).toBool());
    QCOMPARE(nam.requestCount, 0);
}

UT_REGISTER_TEST(TerrainTileManagerTest, TestLabel::Unit, TestLabel::Terrain)
