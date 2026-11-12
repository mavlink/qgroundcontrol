#include "MapAttributionUITest.h"

#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>
#include <QtTest/QTest>

#include "Fact.h"
#include "FlightMapSettings.h"
#include "FlyViewSettings.h"
#include "GeoMapCamera.h"
#include "QGCMapEngineManager.h"
#include "SettingsManager.h"
#include "VideoSettings.h"

UT_REGISTER_TEST(MapAttributionUITest, TestLabel::Integration)

QRectF MapAttributionUITest::_sceneRect(const QQuickItem* item)
{
    return item->mapRectToScene(QRectF(0, 0, item->width(), item->height()));
}

void MapAttributionUITest::_flyViewMapCreditsItsSources_data()
{
    QTest::addColumn<bool>("geoMapEngine");

    QTest::newRow("QtLocation") << false;
    QTest::newRow("GeoMap") << true;
}

void MapAttributionUITest::_flyViewMapCreditsItsSources()
{
    QFETCH(bool, geoMapEngine);

    SettingsManager* const settings = SettingsManager::instance();
    Fact* const geoEngineFact = settings->flyViewSettings()->useGeoMapEngine();
    Fact* const mapProviderFact = settings->flightMapSettings()->mapProvider();
    Fact* const mapTypeFact = settings->flightMapSettings()->mapType();
    Fact* const videoSourceFact = settings->videoSettings()->videoSource();
    const auto restoreSettings =
        qScopeGuard([geoEngineFact, mapProviderFact, mapTypeFact, videoSourceFact,
                     savedEngine = geoEngineFact->rawValue(), savedMapProvider = mapProviderFact->rawValue(),
                     savedMapType = mapTypeFact->rawValue(), savedVideoSource = videoSourceFact->rawValue()] {
            geoEngineFact->setRawValue(savedEngine);
            mapProviderFact->setRawValue(savedMapProvider);
            mapTypeFact->setRawValue(savedMapType);
            videoSourceFact->setRawValue(savedVideoSource);
        });
    ignoreLogMessage("API.QGCApplication.AppMessage", QtDebugMsg,
                     QRegularExpression(QStringLiteral("Restart application for changes to take effect")));
    geoEngineFact->setRawValue(geoMapEngine);
    mapProviderFact->setRawValue(QStringLiteral("Bing"));
    mapTypeFact->setRawValue(QStringLiteral("Hybrid"));
    // A configured video stream makes the Fly View show its PiP window, in the corner of the credit
    videoSourceFact->setRawValue(QString::fromLatin1(VideoSettings::videoSourceUDPH264));

    startUI();
    if (QTest::currentTestFailed()) {
        return;
    }
    if (geoMapEngine) {
        QVERIFY2(expectSoftwareBackendWarnings(/*strict*/ false).has_value(),
                 "No renderer interface on the main window");
    }

    QQuickItem* const map =
        findVisibleItem(_rootItem, geoMapEngine ? QStringLiteral("flyViewGeoMapAdapter") : QStringLiteral("flyViewMap"),
                        TestTimeout::mediumMs());
    QVERIFY2(map, "Fly View map not visible");
    QQuickItem* const credit = findVisibleItem(map, QStringLiteral("mapAttribution"), TestTimeout::mediumMs());
    QVERIFY2(credit, "the map shows no credit");
    const auto creditText = [credit] { return credit->property("text").toString(); };
    QCOMPARE(creditText(), QGCMapEngineManager::mapAttribution(QStringLiteral("Bing Hybrid")));

    // The credit is on the map, clear of what the Fly View puts in that corner
    QQuickItem* const toolStrip = findVisibleItem(_rootItem, QStringLiteral("flyViewToolStrip"));
    QVERIFY2(toolStrip, "tool strip not visible");
    QQuickItem* const pipView = findVisibleItem(_rootItem, QStringLiteral("flyViewPipView"), TestTimeout::mediumMs());
    QVERIFY2(pipView, "PiP window not visible");
    const auto pipRect = [pipView] { return pipView->mapRectToScene(pipView->property("occupiedRect").toRectF()); };
    QTRY_VERIFY_WITH_TIMEOUT(!pipRect().isEmpty(), TestTimeout::mediumMs());
    QTRY_VERIFY_WITH_TIMEOUT(!_sceneRect(credit).intersects(pipRect()), TestTimeout::shortMs());
    QVERIFY(!_sceneRect(credit).intersects(_sceneRect(toolStrip)));
    QVERIFY(_sceneRect(map).contains(_sceneRect(credit)));

    // Swapped into the PiP window, the map is too small for a credit
    const QString pipName = QStringLiteral("flyViewPipView");
    QVERIFY(_clickItemAt(pipView, 0.5, 0.5, pipName));
    QTRY_VERIFY_WITH_TIMEOUT(!credit->isVisible(), TestTimeout::shortMs());

    // Popped out into a window of its own, the map has the corner to itself: the credit keeps none of the room it
    // leaves for the tool strip and the PiP window of the main window
    QObject* const pipState = map->property("pipState").value<QObject*>();
    QVERIFY(pipState);
    QVERIFY(pipState->setProperty("state", pipState->property("windowState")));
    QTRY_VERIFY_WITH_TIMEOUT(credit->isVisible(), TestTimeout::shortMs());
    const auto creditInMap = [credit, map] {
        return credit->mapRectToItem(map, QRectF(0, 0, credit->width(), credit->height()));
    };
    QTRY_COMPARE_LT_WITH_TIMEOUT(creditInMap().left(), toolStrip->width(), TestTimeout::shortMs());
    QTRY_COMPARE_LT_WITH_TIMEOUT(map->height() - creditInMap().bottom(), 2 * credit->height(), TestTimeout::shortMs());

    // Closing that window puts the map back into the PiP window, and a click there back into the main window
    QQuickWindow* const mapWindow = map->window();
    QVERIFY(mapWindow && (mapWindow != _window));
    mapWindow->close();
    QTRY_COMPARE_WITH_TIMEOUT(pipState->property("state"), pipState->property("pipState"), TestTimeout::shortMs());
    QVERIFY(_clickItemAt(pipView, 0.5, 0.5, pipName));
    QTRY_COMPARE_WITH_TIMEOUT(pipState->property("state"), pipState->property("fullState"), TestTimeout::shortMs());
    QTRY_VERIFY_WITH_TIMEOUT(credit->isVisible(), TestTimeout::shortMs());

    // The credit follows the map that the user chooses
    mapProviderFact->setRawValue(QStringLiteral("Mapbox"));
    const QString mapboxCredit = QGCMapEngineManager::mapAttribution(QStringLiteral("Mapbox Hybrid"));
    QTRY_COMPARE_WITH_TIMEOUT(creditText(), mapboxCredit, TestTimeout::shortMs());

    if (geoMapEngine) {
        // 3D draws the relief, whose source is then credited as well
        auto* const camera = map->findChild<GeoMapCamera*>(QStringLiteral("geoMapCamera"));
        QVERIFY2(camera, "GeoMapCamera not found");
        QCOMPARE(camera->mode(), GeoMapCamera::Mode::Mode2D);
        camera->setMode(GeoMapCamera::Mode::Mode3D);
        QTRY_COMPARE_WITH_TIMEOUT(creditText(),
                                  mapboxCredit + QStringLiteral(" | ") + QGCMapEngineManager::terrainAttribution(),
                                  TestTimeout::shortMs());
        camera->setMode(GeoMapCamera::Mode::Mode2D);
        QTRY_COMPARE_WITH_TIMEOUT(creditText(), mapboxCredit, TestTimeout::shortMs());
    }

    // The map of OpenAIP has no type: its provider selects it, whatever type is left of the provider before
    mapProviderFact->setRawValue(QStringLiteral("OpenAIP"));
    const QString openAipCredit = QGCMapEngineManager::mapAttribution(QStringLiteral("OpenAIP"));
    QTRY_COMPARE_WITH_TIMEOUT(creditText(), openAipCredit, TestTimeout::shortMs());

    // The settings page sets the provider, then the type. The two name no map in between: the map stays
    mapProviderFact->setRawValue(QStringLiteral("CustomURL"));
    QCOMPARE(creditText(), openAipCredit);
    // The source that the user sets has no one to credit
    mapTypeFact->setRawValue(QStringLiteral("Custom"));
    QTRY_VERIFY_WITH_TIMEOUT(!credit->isVisible(), TestTimeout::shortMs());

    stopUI();
}
