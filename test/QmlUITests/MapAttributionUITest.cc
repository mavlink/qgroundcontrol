#include "MapAttributionUITest.h"

#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtGui/QColor>
#include <QtPositioning/QGeoCoordinate>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>
#include <QtTest/QTest>

#include "Fact.h"
#include "FlightMapSettings.h"
#include "FlyViewSettings.h"
#include "GeoMapCamera.h"
#include "MissionController.h"
#include "PlanViewSettings.h"
#include "QGCMapEngineManager.h"
#include "QGCMapPalette.h"
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

    // The style of a map tells that it is one of satellite pictures, whatever its name: what lies on it is light
    const auto creditColor = [credit] { return credit->property("color").value<QColor>(); };
    QGCMapPalette mapPalette;
    const QColor darkText = mapPalette.text();
    mapPalette.setLightColors(true);
    const QColor lightText = mapPalette.text();
    mapProviderFact->setRawValue(QStringLiteral("MapQuest"));
    mapTypeFact->setRawValue(QStringLiteral("Sat"));
    QTRY_COMPARE_WITH_TIMEOUT(creditText(), QGCMapEngineManager::mapAttribution(QStringLiteral("MapQuest Sat")),
                              TestTimeout::shortMs());
    QTRY_COMPARE_WITH_TIMEOUT(creditColor(), lightText, TestTimeout::shortMs());

    // The map of OpenAIP has no type: its provider selects it, whatever type is left of the provider before
    mapProviderFact->setRawValue(QStringLiteral("OpenAIP"));
    const QString openAipCredit = QGCMapEngineManager::mapAttribution(QStringLiteral("OpenAIP"));
    QTRY_COMPARE_WITH_TIMEOUT(creditText(), openAipCredit, TestTimeout::shortMs());
    QTRY_COMPARE_WITH_TIMEOUT(creditColor(), darkText, TestTimeout::shortMs());

    // The settings page sets the provider, then the type. The two name no map in between: the map stays
    mapProviderFact->setRawValue(QStringLiteral("CustomURL"));
    QCOMPARE(creditText(), openAipCredit);
    // The source that the user sets has no one to credit
    mapTypeFact->setRawValue(QStringLiteral("Custom"));
    QTRY_VERIFY_WITH_TIMEOUT(!credit->isVisible(), TestTimeout::shortMs());

    stopUI();
}

void MapAttributionUITest::_planViewMapCreditsItsSources_data()
{
    QTest::addColumn<bool>("geoMapEngine");

    QTest::newRow("QtLocation") << false;
    QTest::newRow("GeoMap") << true;
}

void MapAttributionUITest::_planViewMapCreditsItsSources()
{
    QFETCH(bool, geoMapEngine);

    SettingsManager* const settings = SettingsManager::instance();
    Fact* const mapProviderFact = settings->flightMapSettings()->mapProvider();
    Fact* const mapTypeFact = settings->flightMapSettings()->mapType();
    Fact* const missionStatusFact = settings->planViewSettings()->showMissionItemStatus();
    const auto restoreSettings =
        qScopeGuard([mapProviderFact, mapTypeFact, missionStatusFact, savedMapProvider = mapProviderFact->rawValue(),
                     savedMapType = mapTypeFact->rawValue(), savedMissionStatus = missionStatusFact->rawValue()] {
            mapProviderFact->setRawValue(savedMapProvider);
            mapTypeFact->setRawValue(savedMapType);
            missionStatusFact->setRawValue(savedMissionStatus);
        });
    mapProviderFact->setRawValue(QStringLiteral("Bing"));
    mapTypeFact->setRawValue(QStringLiteral("Hybrid"));
    missionStatusFact->setRawValue(true);
    // The Plan View has a GeoMap beside its tabbed panel only
    setUseTabbedPlanPanel(true);
    setUseGeoMapPlanEngine(geoMapEngine);

    startUI();
    if (QTest::currentTestFailed()) {
        return;
    }
    if (geoMapEngine) {
        QVERIFY2(expectSoftwareBackendWarnings(/*strict*/ false).has_value(),
                 "No renderer interface on the main window");
    }

    QVERIFY(clickToolSelectDropdownButton(QStringLiteral("toolbar_viewPlan")));
    QQuickItem* const planView = findVisibleItem(_rootItem, QStringLiteral("mainView_plan"), TestTimeout::mediumMs());
    QVERIFY2(planView, "Plan View not visible");
    QQuickItem* const map = findVisibleItem(planView, QStringLiteral("planView_map"), TestTimeout::mediumMs());
    QVERIFY2(map, "Plan View map not visible");
    // Only a GeoMap has a camera
    QCOMPARE(map->property("camera").isValid(), geoMapEngine);
    QQuickItem* const credit = findVisibleItem(map, QStringLiteral("mapAttribution"), TestTimeout::mediumMs());
    QVERIFY2(credit, "the map shows no credit");
    const auto creditText = [credit] { return credit->property("text").toString(); };
    const QString imageryCredit = QGCMapEngineManager::mapAttribution(QStringLiteral("Bing Hybrid"));
    QTRY_COMPARE_WITH_TIMEOUT(creditText(), imageryCredit, TestTimeout::shortMs());

    // The tabbed panel shows the tool strip and the mission status on its Mission tab, which a plan without a home
    // position does not open. A click on the map sets that position: the test sets it as the click does.
    auto* const missionController =
        qobject_cast<MissionController*>(planView->property("_missionController").value<QObject*>());
    QVERIFY2(missionController, "the Plan View has no mission controller");
    missionController->setHomePosition(QGeoCoordinate(47.397742, 8.545594));
    const QString missionTab = QStringLiteral("planPanel_tab_mission");
    QVERIFY(verifyEnabled(missionTab, true, QStringLiteral("plan with a home position")));
    QVERIFY(clickButton(missionTab));
    QQuickItem* const toolStrip =
        findVisibleItem(planView, QStringLiteral("planView_toolStrip"), TestTimeout::shortMs());
    QVERIFY2(toolStrip, "tool strip not visible");
    QQuickItem* const missionStatus =
        findVisibleItem(planView, QStringLiteral("planView_missionStatus"), TestTimeout::shortMs());
    QVERIFY2(missionStatus, "mission status not visible");

    // The mission status draws the heights of the elevation provider, as the terrain profile of the mission: while
    // it does, that provider is credited after the imagery
    const QString elevationCredit = QGCMapEngineManager::mapAttribution(
        settings->flightMapSettings()->elevationMapProvider()->rawValue().toString());
    QVERIFY2(!elevationCredit.isEmpty(), "the elevation provider has no credit");
    QTRY_COMPARE_WITH_TIMEOUT(creditText(), imageryCredit + QStringLiteral(" | ") + elevationCredit,
                              TestTimeout::shortMs());

    // The credit is on the map, clear of what the Plan View puts in that corner, and under the scale
    QTRY_COMPARE_GE_WITH_TIMEOUT(_sceneRect(credit).left(), _sceneRect(toolStrip).right(), TestTimeout::shortMs());
    QTRY_COMPARE_LE_WITH_TIMEOUT(_sceneRect(credit).bottom(), _sceneRect(missionStatus).top(), TestTimeout::shortMs());
    QVERIFY(_sceneRect(map).contains(_sceneRect(credit)));
    QQuickItem* const scale =
        findVisibleItem(planView, geoMapEngine ? QStringLiteral("geoMapScale") : QStringLiteral("planView_mapScale"));
    QVERIFY2(scale, "map scale not visible");
    QCOMPARE_LE(_sceneRect(scale).bottom(), _sceneRect(credit).top());

    // Without the terrain profile, the Plan View draws nothing of the elevation provider
    missionStatusFact->setRawValue(false);
    QTRY_COMPARE_WITH_TIMEOUT(creditText(), imageryCredit, TestTimeout::shortMs());

    stopUI();
}
