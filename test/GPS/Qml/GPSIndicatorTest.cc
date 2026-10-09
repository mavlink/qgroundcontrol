#include "GPSIndicatorTest.h"

#include <initializer_list>
#include <memory>
#include <tuple>

#include <QtGui/QColor>
#include <QtQuick/QQuickItem>
#include <QtQuickTest/quicktest.h>

#include "Fact.h"
#include "Fixtures/RAIIFixtures.h"
#include "GPSCorrectionManager.h"
#include "GPSManager.h"
#include "GPSReceiver.h"
#include "GPSReceiverFactGroup.h"
#include "MAVLinkLib.h"
#include "NTRIPSettings.h"
#include "QGCPalette.h"
#include "Qml/Support/GPSPanelFixtures.h"
#include "RTKSettings.h"
#include "Receiver/Support/GPSReceiverTestSupport.h"
#include "Receiver/Support/ScriptedReceiverWorker.h"
#include "SettingsManager.h"
#include "Support/GPSQmlTestHelpers.h"
#include "Support/GPSTestHelpers.h"
#include "UnitsSettings.h"
#include "Vehicle.h"
#include "VehicleGPSFactGroup.h"
#include "development/mavlink_msg_gnss_integrity.h"

using namespace GPSTest;

namespace {
/// The toolbar indicator in @a window without a vehicle, with @a inputs replacing the application state it shows; null
/// on failure, with the reason in the engine's lastError().
std::unique_ptr<QObject> createIndicator(GPSTest::QmlEngine& engine, const GPSTest::QmlWindowFixture& window,
                                         QVariantMap inputs = {})
{
    inputs.insert(QStringLiteral("parent"), QVariant::fromValue(window.contentItem()));
    inputs.insert(QStringLiteral("_activeVehicle"), QVariant::fromValue(static_cast<QObject*>(nullptr)));
    return engine.create(GPSTest::sourceQmlUrl(QStringLiteral("Toolbar/GPSIndicator.qml")), inputs);
}

/// The indicator's drawer page in @a window for @a vehicle, laid out for a 640 pixel drawer unless @a inputs give an
/// availableWidth; null on failure, with the reason in the engine's lastError().
std::unique_ptr<QObject> createIndicatorPage(GPSTest::QmlEngine& engine, const GPSTest::QmlWindowFixture& window,
                                             Vehicle* vehicle, QVariantMap inputs = {})
{
    inputs.insert(QStringLiteral("parent"), QVariant::fromValue(window.contentItem()));
    inputs.insert(QStringLiteral("activeVehicle"), QVariant::fromValue(vehicle));
    if (!inputs.contains(QStringLiteral("availableWidth"))) {
        inputs.insert(QStringLiteral("availableWidth"), 640);
    }
    return engine.create(GPSTest::sourceQmlUrl(QStringLiteral("Toolbar/GPSIndicatorPage.qml")), inputs);
}

QColor colorOf(const QObject* item)
{
    return item->property("color").value<QColor>();
}
}  // namespace

void GPSIndicatorTest::_pagePanelUsesApplicationReceiver()
{
    // GPSReceiverSettingsTest covers the panel; the drawer shows it for the application's receiver and settings.
    RTKSettingsFixture settings(gpsReceiverManufacturerForType(GPSType::quectel));
    GPSReceiver* const receiver = GPSManager::instance()->receiver();
    GPSTest::QmlWindowFixture window;
    GPSTest::QmlEngine engine;
    std::unique_ptr<QObject> page = createIndicatorPage(engine, window, nullptr, {{QStringLiteral("expanded"), true}});
    QVERIFY2(page, qPrintable(engine.lastError()));
    auto* consent = page->findChild<QObject*>(QStringLiteral("rtkPersistentChangesCheckBox"));
    QVERIFY(consent);
    if (!consent->property("visible").toBool()) {
        QSKIP("Persistent receiver settings require serial support");
    }
    QVERIFY(!consent->property("checked").toBool());
    QVERIFY(QMetaObject::invokeMethod(consent, "click"));
    QVERIFY(receiver->persistentChangesAllowed());
    receiver->setPersistentChangesAllowed(false);
    QVERIFY(!consent->property("checked").toBool());
}

void GPSIndicatorTest::_pageFitsWidth_data()
{
    QTest::addColumn<int>("width");
    QTest::newRow("mobile") << 320;
    QTest::newRow("narrow-desktop") << 640;
    QTest::newRow("desktop") << 1200;
}

void GPSIndicatorTest::_pageFitsWidth()
{
    QFETCH(int, width);
    RTKSettingsFixture settings(gpsReceiverManufacturerForType(GPSType::quectel));
    QVERIFY(!GPSManager::instance()->receiver()->hasReceiver());
    AppReceiver base;
    GPSReceiverFactGroup* const facts = base.facts();

    GPSTest::QmlWindowFixture window(width, 800);
    GPSTest::QmlEngine engine;
    std::unique_ptr<QObject> page =
        createIndicatorPage(engine, window, nullptr, {{QStringLiteral("availableWidth"), width}});
    QVERIFY2(page, qPrintable(engine.lastError()));
    auto* item = qobject_cast<QQuickItem*>(page.get());
    QVERIFY(item);
    QVERIFY(window.show());
    QVERIFY(!page->property("expanded").toBool());
    QTRY_VERIFY_WITH_TIMEOUT(item->height() > 0 && item->implicitHeight() > 0, TestTimeout::shortMs());
    QVERIFY(item->width() <= width + 1);

    auto* status = page->findChild<QQuickItem*>(QStringLiteral("rtkReceiverStatus"));
    auto* satellites = page->findChild<QQuickItem*>(QStringLiteral("rtkSatellitesInView"));
    auto* usage = page->findChild<QQuickItem*>(QStringLiteral("rtkSatellitesUsed"));
    QVERIFY(status && satellites && usage);
    QVERIFY(status->isVisible());
    const QString disconnectedText = status->property("text").toString();
    QVERIFY(!disconnectedText.isEmpty());
    QVERIFY(!satellites->isVisible());
    QVERIFY(!usage->isVisible());
    QVERIFY(status->mapRectToItem(item, status->boundingRect()).right() <= width + 1);

    QVERIFY(base.connect());
    settings.saved.setFactValue(facts->numSatellites(), 12);
    settings.saved.setFactValue(facts->numSatellitesUsed(), 7);
    QVERIFY(satellites->isVisible() && usage->isVisible());
    QVERIFY(status->property("text").toString() != disconnectedText);
    base.disconnect();
    // Values left from the receiver are hidden while it is disconnected.
    facts->numSatellites()->setRawValue(12);
    facts->numSatellitesUsed()->setRawValue(7);
    QVERIFY(status->isVisible() && !satellites->isVisible() && !usage->isVisible());
    QCOMPARE(status->property("text").toString(), disconnectedText);
    QVERIFY(item->height() > 0);

    // Expanded, the page shows the receiver settings within its width.
    QVERIFY(page->setProperty("expanded", true));
    QTRY_VERIFY_WITH_TIMEOUT(page->findChild<QQuickItem*>(QStringLiteral("gpsReceiverSettings")),
                             TestTimeout::shortMs());
    auto* panel = page->findChild<QQuickItem*>(QStringLiteral("gpsReceiverSettings"));
    auto* connect = page->findChild<QQuickItem*>(QStringLiteral("rtkConnectButton"));
    QVERIFY(connect);
    // TCP connections are available even without serial support.
    QVERIFY(connect->isVisible());
    QVERIFY(connect->isEnabled());
    QTRY_VERIFY_WITH_TIMEOUT(panel->width() > 0, TestTimeout::mediumMs());
    // Once laid out, the panel has the width the page reserves for it, not the much smaller width its wrapped notes
    // report.
    QVERIFY(QQuickTest::qWaitForPolish(window.window(), TestTimeout::mediumMs()));
    QVERIFY2(qAbs(panel->width() - page->property("_settingsWidth").toReal()) <= 1,
             qPrintable(QStringLiteral("panel width %1").arg(panel->width())));
    QTRY_VERIFY_WITH_TIMEOUT(item->width() <= width + 1, TestTimeout::mediumMs());
    QVERIFY(panel->mapRectToItem(item, panel->boundingRect()).right() <= width + 1);
}

void GPSIndicatorTest::_pageOffersNtripConnect()
{
    TestFixtures::SettingsFixture saved;
    NTRIPSettings* ntrip = SettingsManager::instance()->ntripSettings();
    saved.setFactValue(ntrip->ntripServerConnectEnabled(), false);
    saved.setFactValue(ntrip->ntripServerHostAddress(), QString());
    GPSTest::QmlWindowFixture window;
    GPSTest::QmlEngine engine;
    std::unique_ptr<QObject> page = createIndicatorPage(engine, window, nullptr);
    QVERIFY2(page, qPrintable(engine.lastError()));
    auto* section = page->findChild<QObject*>(QStringLiteral("gpsIndicatorNtrip"));
    QVERIFY(section);
    QVERIFY(!section->property("visible").toBool());

    ntrip->ntripServerHostAddress()->setRawValue(QStringLiteral("caster.example"));
    QTRY_VERIFY_WITH_TIMEOUT(section->property("visible").toBool(), TestTimeout::shortMs());
    auto* connect = section->findChild<QObject*>(QStringLiteral("ntripConnectButton"));
    QVERIFY(connect);
    QVERIFY(connect->property("enabled").toBool());
}

void GPSIndicatorTest::_pageVehicleMeasurements()
{
    TestFixtures::SettingsFixture saved;
    UnitsSettings* const units = SettingsManager::instance()->unitsSettings();
    saved.setFactValue(units->horizontalDistanceUnits(), UnitsSettings::HorizontalDistanceUnitsMeters);
    saved.setFactValue(units->verticalDistanceUnits(), UnitsSettings::VerticalDistanceUnitsFeet);
    Vehicle vehicle(MAV_AUTOPILOT_PX4, MAV_TYPE_QUADROTOR);
    auto* gps = vehicle.gpsFactGroup();
    QVERIFY(gps);
    gps->setLiveUpdates(true);
    // The page shows vehicle GPS status only once the vehicle reports GPS telemetry.
    gps->handleMessage(&vehicle, GPSTest::gpsRawMessage({.fixType = GPS_FIX_TYPE_3D_FIX,
                                                         .eph = UINT16_MAX,
                                                         .epv = UINT16_MAX,
                                                         .cog = UINT16_MAX,
                                                         .satellitesVisible = 10}));
    QVERIFY(gps->telemetryAvailable());
    gps->hdop()->setRawValue(0.8);
    gps->vdop()->setRawValue(1.2);
    gps->horizontalAccuracy()->setRawValue(2.5);
    gps->verticalAccuracy()->setRawValue(4.5);
    GPSTest::QmlWindowFixture window;
    GPSTest::QmlEngine engine;
    std::unique_ptr<QObject> page = createIndicatorPage(engine, window, &vehicle);
    QVERIFY2(page, qPrintable(engine.lastError()));
    QVERIFY(window.show());
    auto* hdop = page->findChild<QObject*>(QStringLiteral("vehicleGpsHdop"));
    auto* vdop = page->findChild<QObject*>(QStringLiteral("vehicleGpsVdop"));
    auto* horizontal = page->findChild<QObject*>(QStringLiteral("vehicleGpsHorizontalAccuracy"));
    auto* vertical = page->findChild<QObject*>(QStringLiteral("vehicleGpsVerticalAccuracy"));
    QVERIFY(hdop && vdop && horizontal && vertical);
    QCOMPARE(hdop->property("labelText").toString(), gps->hdop()->cookedValueString());
    QCOMPARE(vdop->property("labelText").toString(), gps->vdop()->cookedValueString());
    // Each accuracy follows its application distance units.
    QCOMPARE(horizontal->property("labelText").toString(), QStringLiteral("2.5 m"));
    QCOMPARE(vertical->property("labelText").toString(), QStringLiteral("14.8 ft"));

    // Values the vehicle does not report are hidden.
    gps->horizontalAccuracy()->setRawValue(qQNaN());
    gps->hdop()->setRawValue(qQNaN());
    gps->vdop()->setRawValue(qQNaN());
    QTRY_VERIFY_WITH_TIMEOUT(!horizontal->property("visible").toBool(), TestTimeout::shortMs());
    QVERIFY(!hdop->property("visible").toBool());
    QVERIFY(!vdop->property("visible").toBool());
    QVERIFY(vertical->property("visible").toBool());
    gps->hdop()->setRawValue(0.9);
    QTRY_VERIFY_WITH_TIMEOUT(hdop->property("visible").toBool(), TestTimeout::shortMs());
    QCOMPARE(hdop->property("labelText").toString(), gps->hdop()->cookedValueString());

    QVERIFY(page->setProperty("activeVehicle", QVariant::fromValue(static_cast<Vehicle*>(nullptr))));
    QVERIFY(!hdop->property("visible").toBool());
    QVERIFY(!vertical->property("visible").toBool());
}

void GPSIndicatorTest::_pageResilienceGroups()
{
    Vehicle vehicle(MAV_AUTOPILOT_PX4, MAV_TYPE_QUADROTOR);
    auto* gps1 = vehicle.gpsFactGroup();
    auto* gps2 = vehicle.gps2FactGroup();
    QVERIFY(gps1 && gps2);
    for (FactGroup* group : std::initializer_list<FactGroup*>{gps1, gps2}) {
        group->setLiveUpdates(true);
    }
    const auto integrity = [&vehicle](VehicleGPSFactGroup* gps, uint8_t id, uint8_t spoofing, uint8_t jamming) {
        mavlink_gnss_integrity_t report{};
        report.id = id;
        report.spoofing_state = spoofing;
        report.jamming_state = jamming;
        report.authentication_state = 0;
        mavlink_message_t message{};
        mavlink_msg_gnss_integrity_encode(1, 1, &message, &report);
        gps->handleMessage(&vehicle, message);
    };
    GPSTest::QmlWindowFixture window;
    GPSTest::QmlEngine engine;
    std::unique_ptr<QObject> page = createIndicatorPage(engine, window, &vehicle);
    QVERIFY2(page, qPrintable(engine.lastError()));
    auto* first = page->findChild<QObject*>(QStringLiteral("gps1Resilience"));
    auto* second = page->findChild<QObject*>(QStringLiteral("gps2Resilience"));
    QVERIFY(first && second);
    QVERIFY(!first->property("visible").toBool());
    QVERIFY(!second->property("visible").toBool());

    // One reporting receiver is the vehicle's resilience status; with two, each is named.
    integrity(gps1, 0, 1, 3);
    QTRY_VERIFY_WITH_TIMEOUT(first->property("visible").toBool(), TestTimeout::shortMs());
    QVERIFY(!second->property("visible").toBool());
    QCOMPARE(first->property("heading").toString(), QStringLiteral("GPS Resilience Status"));

    integrity(gps2, 1, 2, 1);
    QTRY_VERIFY_WITH_TIMEOUT(second->property("visible").toBool(), TestTimeout::shortMs());
    QVERIFY(first->property("visible").toBool());
    QCOMPARE(first->property("heading").toString(), QStringLiteral("GPS 1 Resilience"));
    QCOMPARE(second->property("heading").toString(), QStringLiteral("GPS 2 Resilience"));
}

void GPSIndicatorTest::_resilienceIcon_data()
{
    using Auth = VehicleGPSFactGroup::AuthenticationState;
    constexpr int NONE = VehicleGPSFactGroup::NOT_REPORTED;
    QTest::addColumn<int>("jamming1");
    QTest::addColumn<int>("jamming2");
    QTest::addColumn<int>("authentication1");
    QTest::addColumn<int>("authentication2");
    QTest::addColumn<QByteArray>("interferenceColor");
    QTest::addColumn<QByteArray>("authenticationColor");
    // Each icon shows the worse of the two receivers; VehicleGPSFactGroupTest covers how one receiver ranks.
    QTest::newRow("unknown-hidden") << NONE << NONE << NONE << NONE << QByteArray() << QByteArray();
    QTest::newRow("second-receiver-only") << NONE << 2 << NONE << int(Auth::Initializing)
                                          << QByteArrayLiteral("colorOrange") << QByteArrayLiteral("colorYellow");
    QTest::newRow("worse-first") << 3 << 1 << int(Auth::Error) << int(Auth::Ok) << QByteArrayLiteral("colorRed")
                                 << QByteArrayLiteral("colorRed");
    QTest::newRow("worse-second") << 1 << 2 << int(Auth::Ok) << int(Auth::Error) << QByteArrayLiteral("colorOrange")
                                  << QByteArrayLiteral("colorRed");
}

void GPSIndicatorTest::_resilienceIcon()
{
    QFETCH(int, jamming1);
    QFETCH(int, jamming2);
    QFETCH(int, authentication1);
    QFETCH(int, authentication2);
    QFETCH(QByteArray, interferenceColor);
    QFETCH(QByteArray, authenticationColor);
    VehicleGPSFactGroup gps1;
    VehicleGPSFactGroup gps2(nullptr, nullptr, VehicleGPSFactGroup::ReceiverIndex::Secondary);
    gps1.jammingState()->setRawValue(jamming1);
    gps2.jammingState()->setRawValue(jamming2);
    gps1.authenticationState()->setRawValue(authentication1);
    gps2.authenticationState()->setRawValue(authentication2);
    GPSTest::QmlWindowFixture window;
    GPSTest::QmlEngine engine;
    std::unique_ptr<QObject> indicator = createIndicator(
        engine, window,
        {{QStringLiteral("_gps"), QVariant::fromValue(&gps1)}, {QStringLiteral("_gps2"), QVariant::fromValue(&gps2)}});
    QVERIFY2(indicator, qPrintable(engine.lastError()));
    const QGCPalette palette;
    const auto verifyIcon = [&](const char* objectName, const QByteArray& color) {
        auto* icon = indicator->findChild<QObject*>(QLatin1String(objectName));
        QVERIFY(icon);
        QCOMPARE(icon->property("visible").toBool(), !color.isEmpty());
        if (!color.isEmpty()) {
            QCOMPARE(colorOf(icon), palette.property(color.constData()).value<QColor>());
        }
    };
    verifyIcon("gpsInterferenceIcon", interferenceColor);
    verifyIcon("gpsAuthenticationIcon", authenticationColor);
    for (VehicleGPSFactGroup* gps : {&gps1, &gps2}) {
        gps->jammingState()->setRawValue(VehicleGPSFactGroup::NOT_REPORTED);
        gps->authenticationState()->setRawValue(VehicleGPSFactGroup::NOT_REPORTED);
    }
    verifyIcon("gpsInterferenceIcon", QByteArray());
    verifyIcon("gpsAuthenticationIcon", QByteArray());
}

void GPSIndicatorTest::_showsReceiverWithoutVehicleGPS_data()
{
    using State = GPSCorrectionManager::State;
    QTest::addColumn<int>("role");
    QTest::addColumn<int>("fixType");
    QTest::addColumn<bool>("surveying");
    QTest::addColumn<int>("correctionState");
    QTest::addColumn<QString>("label");
    QTest::addColumn<QString>("detail");
    QTest::newRow("passive-position-only") << int(RTKSettings::Passive) << 3 << false << int(State::Inactive) << "GNSS"
                                           << "3D";
    QTest::newRow("passive-float") << int(RTKSettings::Passive) << 5 << false << int(State::Waiting) << "RTK"
                                   << "Float";
    QTest::newRow("base-surveying") << int(RTKSettings::ConfiguredBase) << 0 << true << int(State::Waiting) << "RTK"
                                    << "Survey";
    QTest::newRow("base-fixed") << int(RTKSettings::ConfiguredBase) << 0 << false << int(State::Fresh) << "RTK"
                                << "Base";
}

void GPSIndicatorTest::_showsReceiverWithoutVehicleGPS()
{
    QFETCH(int, role);
    QFETCH(int, fixType);
    QFETCH(bool, surveying);
    QFETCH(int, correctionState);
    QFETCH(QString, label);
    QFETCH(QString, detail);
    ScriptedReceiverWorkerFactory workers;
    GPSReceiver receiver;
    receiver.setWorkerFactory(workers.workerFactory());
    GPSReceiver::Configuration configuration = receiverConfiguration();
    configuration.receiverRole = static_cast<RTKSettings::ReceiverRole>(role);
    useTcp(configuration);
    receiver.setConfiguration(configuration);
    QVERIFY(receiver.connectReceiver());
    QCOMPARE(int(receiver.activeRole()), role);
    GPSReceiverFactGroup& facts = *receiver.facts();
    facts.setLiveUpdates(true);
    GPSTest::QmlWindowFixture window;
    GPSTest::QmlEngine engine;
    std::unique_ptr<QObject> indicator = createIndicator(engine, window,
                                                         {{QStringLiteral("_receiver"), QVariant::fromValue(&receiver)},
                                                          {QStringLiteral("_correctionState"), correctionState}});
    QVERIFY2(indicator, qPrintable(engine.lastError()));
    QVERIFY(indicator->property("showIndicator").toBool());
    auto* rtkLabel = indicator->findChild<QObject*>(QStringLiteral("gpsCorrectionsLabel"));
    auto* satellites = indicator->findChild<QObject*>(QStringLiteral("gpsSatelliteCount"));
    auto* detailLabel = indicator->findChild<QObject*>(QStringLiteral("gpsDetail"));
    QVERIFY(rtkLabel && satellites && detailLabel);
    QCOMPARE(rtkLabel->property("visible").toBool(), correctionState != int(GPSCorrectionManager::State::Inactive));
    QVERIFY(!satellites->property("visible").toBool());

    workers.current()->ready();
    facts.fixType()->setRawValue(fixType);
    facts.numSatellitesUsed()->setRawValue(9);
    facts.active()->setRawValue(surveying);
    QVERIFY(rtkLabel->property("visible").toBool());
    QCOMPARE(rtkLabel->property("text").toString(), label);
    QVERIFY(satellites->property("visible").toBool());
    QCOMPARE(satellites->property("text").toString(), QStringLiteral("9"));
    QCOMPARE(detailLabel->property("text").toString(), detail);
}

void GPSIndicatorTest::_receiverAntennaWarning()
{
    ScriptedGPSReceiver harness;
    GPSReceiver& receiver = harness.receiver;
    GPSReceiverFactGroup& facts = *receiver.facts();
    facts.setLiveUpdates(true);
    GPSTest::QmlWindowFixture window;
    GPSTest::QmlEngine engine;
    auto status = engine.create(GPSTest::sourceQmlUrl(QStringLiteral("GPS/Qml/GPSReceiverStatus.qml")),
                                {{QStringLiteral("parent"), QVariant::fromValue(window.contentItem())},
                                 {QStringLiteral("receiver"), QVariant::fromValue(&receiver)},
                                 {QStringLiteral("facts"), QVariant::fromValue(&facts)}});
    QVERIFY2(status, qPrintable(engine.lastError()));
    std::unique_ptr<QObject> indicator =
        createIndicator(engine, window,
                        {{QStringLiteral("_receiver"), QVariant::fromValue(&receiver)},
                         {QStringLiteral("_correctionState"), int(GPSCorrectionManager::State::Inactive)}});
    QVERIFY2(indicator, qPrintable(engine.lastError()));
    auto* antenna = status->findChild<QObject*>(QStringLiteral("rtkAntenna"));
    auto* label = indicator->findChild<QObject*>(QStringLiteral("gpsCorrectionsLabel"));
    QVERIFY(antenna && label);
    const QGCPalette palette;
    QVERIFY(connectOverTcp(receiver));
    harness.workers.current()->ready();
    // A receiver without antenna supervision shows no row.
    QVERIFY(!antenna->property("visible").toBool());
    QCOMPARE(colorOf(label), palette.text());
    for (const auto& [state, text, warning] :
         {std::tuple{GPSIntegrityReport::AntennaState::Ok, QStringLiteral("OK"), false},
          std::tuple{GPSIntegrityReport::AntennaState::Open, QStringLiteral("Open"), true},
          std::tuple{GPSIntegrityReport::AntennaState::Short, QStringLiteral("Short"), true}}) {
        facts.antennaState()->setRawValue(static_cast<int>(state));
        QVERIFY(antenna->property("visible").toBool());
        QCOMPARE(antenna->property("labelText").toString(), text);
        // A fault colours the indicator as jamming does.
        QCOMPARE(colorOf(label), warning ? palette.colorOrange() : palette.text());
    }
    receiver.disconnectReceiver();
    QVERIFY(!antenna->property("visible").toBool());
    QCOMPARE(colorOf(label), palette.text());
}

UT_REGISTER_TEST(GPSIndicatorTest, TestLabel::Unit)
