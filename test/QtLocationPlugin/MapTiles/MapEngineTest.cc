#include "MapEngineTest.h"

#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "AppSettings.h"
#include "Fixtures/RAIIFixtures.h"
#include "MapEngine.h"
#include "MapProviderRegistry.h"
#include "SettingsManager.h"

void MapEngineTest::_testReportsSourceChange_data()
{
    QTest::addColumn<QString>("settingName");
    QTest::addColumn<QString>("providerType");
    QTest::newRow("Custom URL") << QStringLiteral("customURL") << QStringLiteral("CustomURL Custom");
    QTest::newRow("Mapbox Custom account") << QStringLiteral("mapboxAccount") << QStringLiteral("Mapbox Custom");
    QTest::newRow("Mapbox Custom style") << QStringLiteral("mapboxStyle") << QStringLiteral("Mapbox Custom");
}

void MapEngineTest::_testReportsSourceChange()
{
    QFETCH(QString, settingName);
    QFETCH(QString, providerType);

    Fact* const setting = SettingsManager::instance()->appSettings()->property(qPrintable(settingName)).value<Fact*>();
    QVERIFY(setting);
    TestFixtures::SettingsFixture saved;
    saved.setFactValue(setting, QStringLiteral("source A"));

    // A map that is open holds tiles of the source that was configured, so it has to hear of another one
    MapEngine engine;
    QSignalSpy sourceSpy(&engine, &MapEngine::tileSourceChanged);
    saved.setFactValue(setting, QStringLiteral("source B"));

    QCOMPARE(sourceSpy.count(), 1);
    QCOMPARE(sourceSpy.first().first().toInt(), MapProviderRegistry::getMapIdFromProviderType(providerType));
}

UT_REGISTER_TEST(MapEngineTest, TestLabel::Unit)
