#include "GPSIndicatorTest.h"

#include <chrono>
#include <initializer_list>
#include <memory>
#include <tuple>

#include <QtGui/QColor>
#include <QtPositioning/QGeoCoordinate>
#include <QtQuick/QQuickItem>
#include <QtQuickTest/quicktest.h>

#include "Fact.h"
#include "Fixtures/RAIIFixtures.h"
#include "GPSCorrectionManager.h"
#include "GPSManager.h"
#include "GPSReceiver.h"
#include "GPSReceiverFactGroup.h"
#include "MAVLinkLib.h"
#include "ManualScheduler.h"
#include "NTRIP/Support/MockNTRIPTransport.h"
#include "NTRIPSettings.h"
#include "PositionManager.h"
#include "Protocols/Support/ProtocolTestPackets.h"
#include "QGCPalette.h"
#include "Qml/Support/GPSPanelFixtures.h"
#include "QmlObjectListModel.h"
#include "QmlTesting/QmlFixtures.h"
#include "RTKSettings.h"
#include "Receiver/Support/GPSReceiverTestSupport.h"
#include "Receiver/Support/ScriptedReceiverWorker.h"
#include "SettingsManager.h"
#include "Support/GPSTestHelpers.h"
#include "UnitsSettings.h"
#include "Vehicle.h"
#include "VehicleGPSFactGroup.h"
#include "development/mavlink_msg_gnss_integrity.h"

using namespace GPSTest;

namespace {
/// The toolbar indicator in @a window without a vehicle, with @a inputs replacing the application state it shows; null
/// on failure, with the reason in the engine's lastError().
std::unique_ptr<QObject> createIndicator(TestFixtures::QmlEngine& engine, const TestFixtures::QmlWindowFixture& window,
                                         QVariantMap inputs = {})
{
    inputs.insert(QStringLiteral("parent"), QVariant::fromValue(window.contentItem()));
    inputs.insert(QStringLiteral("_activeVehicle"), QVariant::fromValue(static_cast<QObject*>(nullptr)));
    return engine.create(TestFixtures::sourceQmlUrl(QStringLiteral("Toolbar/GPSIndicator.qml")), inputs);
}

/// The indicator's drawer page in @a window for @a vehicle, laid out for a 640 pixel drawer unless @a inputs give an
/// availableWidth; null on failure, with the reason in the engine's lastError().
std::unique_ptr<QObject> createIndicatorPage(TestFixtures::QmlEngine& engine,
                                             const TestFixtures::QmlWindowFixture& window, Vehicle* vehicle,
                                             QVariantMap inputs = {})
{
    inputs.insert(QStringLiteral("parent"), QVariant::fromValue(window.contentItem()));
    inputs.insert(QStringLiteral("activeVehicle"), QVariant::fromValue(vehicle));
    if (!inputs.contains(QStringLiteral("availableWidth"))) {
        inputs.insert(QStringLiteral("availableWidth"), 640);
    }
    return engine.create(TestFixtures::sourceQmlUrl(QStringLiteral("Toolbar/GPSIndicatorPage.qml")), inputs);
}

QColor colorOf(const QObject* item)
{
    return item->property("color").value<QColor>();
}

/// The one shown item of that name under @a root; null when none or several are shown. The page has the status of
/// two receivers, and so two Connect buttons, once a position receiver is in use.
QQuickItem* shownItem(const QObject* root, const char* name)
{
    QQuickItem* shown = nullptr;
    for (QQuickItem* const item : root->findChildren<QQuickItem*>(QString::fromLatin1(name))) {
        if (item->isVisible()) {
            if (shown) {
                return nullptr;
            }
            shown = item;
        }
    }
    return shown;
}

/// The texts shown under @a box that reach past its left or right edge, or are wider than the room their layout gives
/// them, each with its extent; empty when every text is inside the box.
QStringList textsOutside(QQuickItem* box)
{
    QStringList outside;
    const auto texts = TestFixtures::findItems(box, [](QQuickItem* item) {
        return item->isVisible() && item->property("contentWidth").isValid() &&
               !item->property("text").toString().isEmpty();
    });
    for (QQuickItem* const text : texts) {
        const QRectF rect = text->mapRectToItem(box, text->boundingRect());
        const qreal painted = text->property("contentWidth").toReal();
        if (rect.left() < -1 || rect.right() > box->width() + 1 || painted > text->width() + 1) {
            outside.append(QStringLiteral("\"%1\" at %2 to %3, painted %4 wide, in a box of %5")
                               .arg(text->property("text").toString())
                               .arg(rect.left())
                               .arg(rect.right())
                               .arg(painted)
                               .arg(box->width()));
        }
    }
    return outside;
}

/// The items shown under @a root that are named @a name; a list's delegates can share a name.
QList<QQuickItem*> shownItems(const QObject* root, const char* name)
{
    return TestFixtures::findItems(qobject_cast<QQuickItem*>(const_cast<QObject*>(root)), [name](QQuickItem* item) {
        return item->isVisible() && item->objectName() == QLatin1String(name);
    });
}

/// The number of lines the one label under @a box that shows @a text takes; 0 when none or several show it.
int lineCount(QQuickItem* box, const QString& text)
{
    const auto shown = TestFixtures::findItems(box, [&text](QQuickItem* item) {
        return item->isVisible() && item->property("text").toString() == text && item->property("lineCount").isValid();
    });
    return shown.size() == 1 ? shown.first()->property("lineCount").toInt() : 0;
}
}  // namespace

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
    RTKSettingsFixture settings(gpsReceiverManufacturerForType(GPSType::Quectel));
    QVERIFY(!GPSManager::instance()->receiver()->hasReceiver());
    AppReceiver base;
    GPSReceiverFactGroup* const facts = base.facts();

    TestFixtures::QmlWindowFixture window(width, 800);
    TestFixtures::QmlEngine engine;
    std::unique_ptr<QObject> page =
        createIndicatorPage(engine, window, nullptr, {{QStringLiteral("availableWidth"), width}});
    QVERIFY2(page, qPrintable(engine.lastError()));
    auto* item = qobject_cast<QQuickItem*>(page.get());
    QVERIFY(item);
    QVERIFY(window.show());
    QTRY_VERIFY_WITH_TIMEOUT(item->height() > 0 && item->implicitHeight() > 0, TestTimeout::shortMs());
    QVERIFY(item->width() <= width + 1);

    auto* status = page->findChild<QQuickItem*>(QStringLiteral("rtkReceiverStatus"));
    auto* satellites = page->findChild<QQuickItem*>(QStringLiteral("rtkSatellitesInView"));
    auto* usage = page->findChild<QQuickItem*>(QStringLiteral("rtkSatellitesUsed"));
    QVERIFY(status && satellites && usage);
    QVERIFY(status->isVisible());
    const QString disconnectedText = status->property("statusText").toString();
    QVERIFY(!disconnectedText.isEmpty());
    QVERIFY(!satellites->isVisible());
    QVERIFY(!usage->isVisible());
    // The status row, with its Connect button, fits the page. TCP connections are available even without serial
    // support, so the page offers Connect.
    QVERIFY(QQuickTest::qWaitForPolish(window.window(), TestTimeout::mediumMs()));
    QVERIFY(status->mapRectToItem(item, status->boundingRect()).right() <= width + 1);
    QQuickItem* connect = shownItem(page.get(), "rtkConnectButton");
    QVERIFY(connect);
    QVERIFY(connect->isEnabled());
    QVERIFY(connect->mapRectToItem(item, connect->boundingRect()).right() <= width + 1);

    QVERIFY(base.connect());
    settings.saved.setFactValue(facts->numSatellites(), 12);
    settings.saved.setFactValue(facts->numSatellitesUsed(), 7);
    QVERIFY(satellites->isVisible() && usage->isVisible());
    QVERIFY(status->property("statusText").toString() != disconnectedText);
    base.disconnect();
    // Values left from the receiver are hidden while it is disconnected.
    facts->numSatellites()->setRawValue(12);
    facts->numSatellitesUsed()->setRawValue(7);
    QVERIFY(status->isVisible() && !satellites->isVisible() && !usage->isVisible());
    QCOMPARE(status->property("statusText").toString(), disconnectedText);
    QVERIFY(item->height() > 0);

    // The links to the settings pages fit the page too.
    for (const char* const name : {"gpsIndicatorReceiverSettings", "gpsIndicatorCorrectionsSettings"}) {
        QQuickItem* const link = shownItem(page.get(), name);
        QVERIFY2(link, name);
        QVERIFY2(link->mapRectToItem(item, link->boundingRect()).right() <= width + 1, name);
    }
}

void GPSIndicatorTest::_receiverBesideVehicleGps()
{
    RTKSettingsFixture settings(gpsReceiverManufacturerForType(GPSType::UBlox));
    AppReceiver base;
    GPSReceiver* const receiver = GPSManager::instance()->receiver();
    auto configuration = receiverConfiguration();
    useTcp(configuration);
    configuration.autoConnect = false;
    configuration.tcpHost.clear();
    receiver->setConfiguration(configuration);
    QVERIFY(!receiver->hasEndpoint());
    Vehicle vehicle(MAV_AUTOPILOT_PX4, MAV_TYPE_QUADROTOR);
    auto* gps = vehicle.gpsFactGroup();
    QVERIFY(gps);
    gps->setLiveUpdates(true);
    gps->handleMessage(&vehicle, GPSTest::gpsRawMessage({.fixType = GPS_FIX_TYPE_3D_FIX, .satellitesVisible = 10}));

    TestFixtures::QmlWindowFixture window;
    TestFixtures::QmlEngine engine;
    std::unique_ptr<QObject> page = createIndicatorPage(engine, window, &vehicle);
    QVERIFY2(page, qPrintable(engine.lastError()));
    QVERIFY(window.show());
    QVERIFY(QQuickTest::qWaitForPolish(window.window(), TestTimeout::mediumMs()));
    // A receiver that is not set up adds nothing to a vehicle's GPS status.
    QVERIFY(!shownItem(page.get(), "rtkConnectButton"));

    // Once the settings name the receiver, the page offers Connect beside the vehicle's status.
    configuration.tcpHost = QStringLiteral("rtk.test");
    receiver->setConfiguration(configuration);
    QTRY_VERIFY_WITH_TIMEOUT(shownItem(page.get(), "rtkConnectButton"), TestTimeout::shortMs());
    QVERIFY(shownItem(page.get(), "rtkConnectButton")->isEnabled());
}

void GPSIndicatorTest::_holdsStatusAndActions()
{
    // A vehicle that reports its GPS, a connected base and a caster that is set up: every group the page has.
    RTKSettingsFixture settings(gpsReceiverManufacturerForType(GPSType::UBlox));
    NTRIPSettings* const ntrip = SettingsManager::instance()->ntripSettings();
    settings.saved.setFactValue(ntrip->ntripServerConnectEnabled(), false);
    const GPSTest::CasterHost caster(GPSManager::instance()->ntrip(), QStringLiteral("caster.example"));
    Vehicle vehicle(MAV_AUTOPILOT_PX4, MAV_TYPE_QUADROTOR);
    auto* gps = vehicle.gpsFactGroup();
    QVERIFY(gps);
    gps->setLiveUpdates(true);
    gps->handleMessage(&vehicle, GPSTest::gpsRawMessage({.fixType = GPS_FIX_TYPE_3D_FIX, .satellitesVisible = 10}));
    AppReceiver base;
    QVERIFY(base.connect());

    TestFixtures::QmlWindowFixture window;
    TestFixtures::QmlEngine engine;
    std::unique_ptr<QObject> page = createIndicatorPage(engine, window, &vehicle);
    QVERIFY2(page, qPrintable(engine.lastError()));
    auto* item = qobject_cast<QQuickItem*>(page.get());
    QVERIFY(item);
    QVERIFY(window.show());
    // The corrections of the base are shown once the correction manager has seen its source, a moment after the
    // base connected.
    auto* corrections = page->findChild<QQuickItem*>(QStringLiteral("gpsIndicatorCorrections"));
    QVERIFY(corrections);
    QTRY_VERIFY_WITH_TIMEOUT(corrections->isVisible(), TestTimeout::mediumMs());
    QVERIFY(QQuickTest::qWaitForPolish(window.window(), TestTimeout::mediumMs()));

    // Status, from the vehicle to the ground station, and the links to the settings pages last.
    qreal bottom = 0;
    for (const char* const name : {"gpsIndicatorVehicle", "gpsIndicatorReceiver", "gpsIndicatorCorrections",
                                   "gpsIndicatorNtrip", "gpsIndicatorGcsPosition", "gpsIndicatorSettings"}) {
        QQuickItem* const group = shownItem(page.get(), name);
        QVERIFY2(group, name);
        const QRectF rect = group->mapRectToItem(item, group->boundingRect());
        QVERIFY2(rect.top() >= bottom && rect.height() > 0, name);
        bottom = rect.bottom();
    }
    QVERIFY2(qAbs(item->implicitHeight() - bottom) <= 1,
             qPrintable(QStringLiteral("page %1, groups %2").arg(item->implicitHeight()).arg(bottom)));
    // The actions that are needed in flight: the receiver's connection and the caster's.
    QQuickItem* const receiverAction = shownItem(page.get(), "rtkConnectButton");
    QQuickItem* const casterAction = shownItem(page.get(), "ntripConnectButton");
    QVERIFY(receiverAction && receiverAction->isEnabled());
    QVERIFY(casterAction && casterAction->isEnabled());

    // No setting is edited here. The page has no expanded view, and none of the receiver's form, which is on the
    // settings page the first link is named after.
    QVERIFY(!page->property("_showExpand").toBool());
    for (const char* const name : {"rtkManufacturer", "rtkConnectionType", "rtkSerialDevice", "rtkAutoConnect",
                                   "rtkPersistentChangesCheckBox"}) {
        QVERIFY2(!page->findChild<QObject*>(QString::fromLatin1(name)), name);
    }
    QQuickItem* const receiverLink = shownItem(page.get(), "gpsIndicatorReceiverSettings");
    QQuickItem* const correctionsLink = shownItem(page.get(), "gpsIndicatorCorrectionsSettings");
    QVERIFY(receiverLink && correctionsLink);
    QCOMPARE(receiverLink->property("label").toString(), QStringLiteral("GNSS Receiver"));
    QCOMPARE(correctionsLink->property("label").toString(), QStringLiteral("RTK Corrections"));
}

void GPSIndicatorTest::_gcsPositionStaysInsideItsBox()
{
    TestFixtures::QmlWindowFixture window;
    TestFixtures::QmlEngine engine;
    std::unique_ptr<QObject> page = createIndicatorPage(engine, window, nullptr);
    QVERIFY2(page, qPrintable(engine.lastError()));
    QVERIFY(window.show());
    auto* box = page->findChild<QQuickItem*>(QStringLiteral("gpsIndicatorGcsPosition"));
    QVERIFY(box && box->isVisible());
    auto* status = box->findChild<QQuickItem*>(QStringLiteral("gcsPositionSourceStatus"));
    QVERIFY(status);
    // The longest status, which is wider than the status column.
    const QString sentence = PositionManager::tr("Position fix does not meet accuracy requirements");
    QVERIFY(status->setProperty("labelText", sentence));
    QVERIFY(QQuickTest::qWaitForPolish(window.window(), TestTimeout::mediumMs()));
    QVERIFY(qAbs(box->width() - page->property("_preferredStatusWidth").toReal()) <= 1);
    const auto shown = TestFixtures::findItems(status, [&sentence](QQuickItem* item) {
        return item->property("text").toString() == sentence && item->property("lineCount").isValid();
    });
    QCOMPARE(shown.size(), 1);
    QVERIFY(shown.first()->property("lineCount").toInt() > 1);
    // It wraps inside the box instead of running past it and taking the other rows' values along.
    const QStringList outside = textsOutside(box);
    QVERIFY2(outside.isEmpty(), qPrintable(outside.join(QStringLiteral("; "))));
}

void GPSIndicatorTest::_ntripStaysInsideItsBox()
{
    TestFixtures::SettingsFixture saved;
    NTRIPSettings* const settings = SettingsManager::instance()->ntripSettings();
    saved.setFactValue(settings->ntripServerConnectEnabled(), false);
    const QString mountpoint = QStringLiteral("VRS_RTCM32_MSM7_NEAREST_STATION");
    // The page shows the box once the application's caster is named. Its rows follow a stream of this test, which
    // has reported a position and whose corrections went to a vehicle, so that every row shows.
    const GPSTest::CasterHost caster(GPSManager::instance()->ntrip(), QStringLiteral("caster.example"));
    ManualScheduler scheduler;
    GPSCorrectionManager corrections(nullptr, &scheduler);
    QList<QByteArray> routed;
    captureVehicleFrames(corrections, routed);
    NTRIPManager ntrip(nullptr, &scheduler, {.corrections = &corrections});
    auto configuration = mockCasterConfiguration();
    configuration.connection.mountpoint = mountpoint;
    ntrip.setConfiguration(configuration);
    ntrip.setGgaPositionProvider(NTRIPGgaReporter::PositionSource::RTKReceiver,
                                 [] { return ggaObservation(QGeoCoordinate(47.3, 8.5, 400)); });
    MockNTRIPTransport* const transport = injectMockTransport(ntrip, true);
    ntrip.init();
    transport->simulateRtcmData(GPSTest::rtcmMessage(1005), 1005, scheduler.nowMs());
    deliverQueuedCalls();
    QCOMPARE(ntrip.connectionStatus(), NTRIPManager::ConnectionStatus::Connected);
    QVERIFY(corrections.vehicleBytesSubmitted() > 0);
    QVERIFY(!ntrip.ggaSource().isEmpty());

    TestFixtures::QmlWindowFixture window;
    TestFixtures::QmlEngine engine;
    std::unique_ptr<QObject> page = createIndicatorPage(engine, window, nullptr);
    QVERIFY2(page, qPrintable(engine.lastError()));
    QVERIFY(window.show());
    auto* box = page->findChild<QQuickItem*>(QStringLiteral("gpsIndicatorNtrip"));
    QVERIFY(box && box->isVisible());
    QVERIFY(box->setProperty("ntripManager", QVariant::fromValue(&ntrip)));
    // The longest value the counter shows.
    auto* received = box->findChild<QQuickItem*>(QStringLiteral("ntripDataReceived"));
    QVERIFY(received && received->isVisible());
    QVERIFY(received->setProperty("labelText", QStringLiteral("1,023.9MB (1,023.9KB/s)")));
    QVERIFY(QQuickTest::qWaitForPolish(window.window(), TestTimeout::mediumMs()));
    QVERIFY(qAbs(box->width() - page->property("_preferredStatusWidth").toReal()) <= 1);
    // The rows wrap inside the box instead of widening its layout, which would take every value of the box and the
    // button past its edge.
    const QStringList outside = textsOutside(box);
    QVERIFY2(outside.isEmpty(), qPrintable(outside.join(QStringLiteral("; "))));
    QVERIFY(lineCount(box, mountpoint) > 1);
    // What went on to vehicles is the Corrections status's to show, from whichever source it came.
    const auto queued = TestFixtures::findItems(box, [](QQuickItem* item) {
        return item->isVisible() && item->property("label").toString() == QStringLiteral("Queued to vehicles");
    });
    QVERIFY(queued.isEmpty());
}

void GPSIndicatorTest::_receiverStaysInsideItsBox()
{
    RTKSettingsFixture settings(gpsReceiverManufacturerForType(GPSType::UBlox));
    AppReceiver base;
    QVERIFY(base.connect());
    // The fix with the longest name, which with its label is wider than the status column.
    const QString fix = QStringLiteral("3D RTK GPS Lock (float)");
    settings.saved.setFactValue(base.facts()->fixType(), static_cast<int>(GPSFixQuality::RTKFloat));
    QCOMPARE(base.facts()->fixType()->enumStringValue(), fix);

    TestFixtures::QmlWindowFixture window;
    TestFixtures::QmlEngine engine;
    std::unique_ptr<QObject> page = createIndicatorPage(engine, window, nullptr);
    QVERIFY2(page, qPrintable(engine.lastError()));
    QVERIFY(window.show());
    auto* box = page->findChild<QQuickItem*>(QStringLiteral("gpsIndicatorReceiver"));
    QVERIFY(box && box->isVisible());
    auto* row = box->findChild<QQuickItem*>(QStringLiteral("rtkFixType"));
    QVERIFY(row && row->isVisible());
    QCOMPARE(row->property("labelText").toString(), fix);
    QVERIFY(QQuickTest::qWaitForPolish(window.window(), TestTimeout::mediumMs()));
    QVERIFY(qAbs(box->width() - page->property("_preferredStatusWidth").toReal()) <= 1);
    // It wraps inside the box instead of running past it and taking the other rows' values along.
    const QStringList outside = textsOutside(box);
    QVERIFY2(outside.isEmpty(), qPrintable(outside.join(QStringLiteral("; "))));
}

void GPSIndicatorTest::_vehicleStaysInsideItsBox_data()
{
    QTest::addColumn<qreal>("available");
    QTest::newRow("preferred") << qreal(640);
    QTest::newRow("narrow") << qreal(200);
}

void GPSIndicatorTest::_vehicleStaysInsideItsBox()
{
    QFETCH(qreal, available);
    // The fix with the longest name.
    const QString fix = QStringLiteral("3D RTK GPS Lock (fixed)");
    Vehicle vehicle(MAV_AUTOPILOT_PX4, MAV_TYPE_QUADROTOR);
    auto* gps = vehicle.gpsFactGroup();
    QVERIFY(gps);
    gps->setLiveUpdates(true);
    gps->handleMessage(&vehicle, GPSTest::gpsRawMessage({.fixType = GPS_FIX_TYPE_RTK_FIXED, .satellitesVisible = 20}));
    QCOMPARE(gps->lock()->enumStringValue(), fix);

    TestFixtures::QmlWindowFixture window;
    TestFixtures::QmlEngine engine;
    std::unique_ptr<QObject> page =
        createIndicatorPage(engine, window, &vehicle, {{QStringLiteral("availableWidth"), available}});
    QVERIFY2(page, qPrintable(engine.lastError()));
    QVERIFY(window.show());
    auto* box = page->findChild<QQuickItem*>(QStringLiteral("gpsIndicatorVehicle"));
    QVERIFY(box && box->isVisible());
    auto* row = box->findChild<QQuickItem*>(QStringLiteral("vehicleGpsLock"));
    QVERIFY(row && row->isVisible());
    QCOMPARE(row->property("labelText").toString(), fix);
    QVERIFY(QQuickTest::qWaitForPolish(window.window(), TestTimeout::mediumMs()));
    const qreal preferred = page->property("_preferredStatusWidth").toReal();
    QVERIFY2(qAbs(box->width() - qMin(available, preferred)) <= 1,
             qPrintable(QStringLiteral("box %1").arg(box->width())));
    // On a drop-down that is narrower than the status column prefers, it wraps inside the box instead of running
    // past it.
    const QStringList outside = textsOutside(box);
    QVERIFY2(outside.isEmpty(), qPrintable(outside.join(QStringLiteral("; "))));
    QCOMPARE(lineCount(box, fix) > 1, available < preferred);
}

void GPSIndicatorTest::_vehicleRtkValuesShownOnce()
{
    // Two vehicles that report an RTK baseline and a correction rate. The first is the active one, whose GPS status
    // the page shows.
    Vehicle active(MAV_AUTOPILOT_PX4, MAV_TYPE_QUADROTOR);
    Vehicle other(MAV_AUTOPILOT_PX4, MAV_TYPE_QUADROTOR);
    QmlObjectListModel vehicles;
    for (Vehicle* const vehicle : {&active, &other}) {
        auto* gps = vehicle->gpsFactGroup();
        QVERIFY(gps);
        gps->setLiveUpdates(true);
        gps->handleMessage(vehicle,
                           GPSTest::gpsRawMessage({.fixType = GPS_FIX_TYPE_RTK_FIXED, .satellitesVisible = 20}));
        mavlink_gps_rtk_t rtk{};
        rtk.baseline_a_mm = 3000;
        rtk.baseline_b_mm = 4000;
        rtk.rtk_rate = 1;
        mavlink_message_t rtkMessage{};
        mavlink_msg_gps_rtk_encode(1, 1, &rtkMessage, &rtk);
        gps->handleMessage(vehicle, rtkMessage);
        vehicles.append(vehicle);
    }

    TestFixtures::QmlWindowFixture window;
    TestFixtures::QmlEngine engine;
    std::unique_ptr<QObject> page = createIndicatorPage(engine, window, &active);
    QVERIFY2(page, qPrintable(engine.lastError()));
    QVERIFY(window.show());
    auto* box = page->findChild<QQuickItem*>(QStringLiteral("gpsIndicatorCorrections"));
    QVERIFY(box);
    // Shown as it is while a correction source is on.
    QVERIFY(box->setProperty("showWhenInactive", true));
    QVERIFY(box->setProperty("vehicles", QVariant::fromValue(&vehicles)));
    QVERIFY(box->isVisible());
    // The active vehicle's baseline and correction rate are rows of its GPS status.
    QCOMPARE(shownItems(page.get(), "vehicleGpsRtkBaseline").size(), 1);
    QCOMPARE(shownItems(page.get(), "vehicleGpsRtkRate").size(), 1);
    // The corrections therefore list every vehicle's fix, and these two values of the other vehicle alone.
    QCOMPARE(shownItems(box, "correctionsVehicleFix").size(), 2);
    QCOMPARE(shownItems(box, "correctionsVehicleBaseline").size(), 1);
    QCOMPARE(shownItems(box, "correctionsVehicleCorrectionRate").size(), 1);
}

void GPSIndicatorTest::_correctionsStayInsideTheirBox_data()
{
    QTest::addColumn<int>("units");
    QTest::addColumn<QString>("farthest");
    QTest::newRow("miles") << int(UnitsSettings::HorizontalDistanceUnitsFeet) << QStringLiteral("12437 mi");
    QTest::newRow("kilometers") << int(UnitsSettings::HorizontalDistanceUnitsMeters) << QStringLiteral("20015 km");
}

void GPSIndicatorTest::_correctionsStayInsideTheirBox()
{
    using namespace std::chrono_literals;
    QFETCH(int, units);
    QFETCH(QString, farthest);
    TestFixtures::SettingsFixture saved;
    saved.setFactValue(SettingsManager::instance()->unitsSettings()->horizontalDistanceUnits(), units);
    // A vehicle with the fix of the longest name, and the longest baseline and the highest rate GPS_RTK can report.
    Vehicle vehicle(MAV_AUTOPILOT_PX4, MAV_TYPE_QUADROTOR);
    auto* gps = vehicle.gpsFactGroup();
    QVERIFY(gps);
    gps->setLiveUpdates(true);
    gps->handleMessage(&vehicle, GPSTest::gpsRawMessage({.fixType = GPS_FIX_TYPE_RTK_FIXED, .satellitesVisible = 20}));
    mavlink_gps_rtk_t rtk{};
    rtk.baseline_a_mm = rtk.baseline_b_mm = rtk.baseline_c_mm = INT32_MAX;
    rtk.rtk_rate = UINT8_MAX;
    mavlink_message_t rtkMessage{};
    mavlink_msg_gps_rtk_encode(1, 1, &rtkMessage, &rtk);
    gps->handleMessage(&vehicle, rtkMessage);
    QmlObjectListModel vehicles;
    vehicles.append(&vehicle);
    // The source with the longest name, which has announced its base station.
    ManualScheduler scheduler;
    GPSCorrectionManager corrections(nullptr, &scheduler);
    QList<QByteArray> sent;
    GPSTest::captureVehicleFrames(corrections, sent);
    const auto local = corrections.openSource(GPSCorrectionSettings::LocalReceiver, {});
    // The RTCM 1005 frame of the recording Core/Protocols/fixtures/mixed.gps.
    local->submit(QByteArray::fromHex("d300133ed000038a58d9493c872f34109d07d6af48205ad7f7"), scheduler.nowMs());
    QVERIFY(scheduler.advanceBy(1s));
    local->submit(GPSTest::rtcmMessage(1077, 20), scheduler.nowMs());
    QVERIFY(scheduler.advanceBy(1s));
    QCOMPARE(corrections.state(), GPSCorrectionManager::State::Fresh);
    QVERIFY(corrections.basePosition().isValid());

    TestFixtures::QmlWindowFixture window;
    TestFixtures::QmlEngine engine;
    std::unique_ptr<QObject> page = createIndicatorPage(engine, window, nullptr);
    QVERIFY2(page, qPrintable(engine.lastError()));
    QVERIFY(window.show());
    auto* box = page->findChild<QQuickItem*>(QStringLiteral("gpsIndicatorCorrections"));
    QVERIFY(box);
    QVERIFY(box->setProperty("corrections", QVariant::fromValue(&corrections)));
    QVERIFY(box->setProperty("vehicles", QVariant::fromValue(&vehicles)));
    // A base station on the other side of the earth, which a mountpoint picked by mistake can be: the longest
    // distance there is.
    const QGeoCoordinate base = corrections.basePosition();
    QVERIFY(
        box->setProperty("gcsPosition", QVariant::fromValue(QGeoCoordinate(-base.latitude(), base.longitude() - 180))));
    QVERIFY(box->isVisible());
    auto* source = box->findChild<QQuickItem*>(QStringLiteral("correctionsSelectedSource"));
    auto* rate = box->findChild<QQuickItem*>(QStringLiteral("correctionsDataRate"));
    auto* distance = box->findChild<QQuickItem*>(QStringLiteral("correctionsBaseDistance"));
    auto* queued = box->findChild<QQuickItem*>(QStringLiteral("correctionsQueuedToVehicles"));
    QVERIFY(source && rate && distance && queued);
    QCOMPARE(source->property("labelText").toString(),
             GPSCorrectionManager::sourceName(static_cast<int>(GPSCorrectionSettings::LocalReceiver)));
    QVERIFY(rate->isVisible() && distance->isVisible() && queued->isVisible());
    // The longest rate and the longest size.
    QVERIFY(rate->setProperty("labelText", QStringLiteral("1,023.9KB/s")));
    QVERIFY(queued->setProperty("labelText", QStringLiteral("1,023.9GB")));
    const auto vehicleRow = [box](const char* name) {
        const auto rows = TestFixtures::findItems(
            box, [name](QQuickItem* item) { return item->objectName() == QLatin1String(name) && item->isVisible(); });
        return rows.size() == 1 ? rows.first() : nullptr;
    };
    QVERIFY(vehicleRow("correctionsVehicleFix"));
    QVERIFY(vehicleRow("correctionsVehicleBaseline"));
    QVERIFY(vehicleRow("correctionsVehicleCorrectionRate"));
    QCOMPARE(vehicleRow("correctionsVehicleCorrectionRate")->property("labelText").toString(),
             QStringLiteral("255 Hz"));
    QVERIFY(QQuickTest::qWaitForPolish(window.window(), TestTimeout::mediumMs()));
    QVERIFY(qAbs(box->width() - page->property("_preferredStatusWidth").toReal()) <= 1);
    // These rows do not wrap, so they must fit.
    const QStringList outside = textsOutside(box);
    QVERIFY2(outside.isEmpty(), qPrintable(outside.join(QStringLiteral("; "))));
    QCOMPARE(distance->property("labelText").toString(), farthest);
}

void GPSIndicatorTest::_statusColumnKeepsItsWidth()
{
    // A drawer that leaves the status column less than it prefers, and not whole pixels: the page's layout rounds
    // the size it gives the column.
    const qreal available = 200.4;
    TestFixtures::QmlWindowFixture window;
    TestFixtures::QmlEngine engine;
    std::unique_ptr<QObject> page =
        createIndicatorPage(engine, window, nullptr, {{QStringLiteral("availableWidth"), available}});
    QVERIFY2(page, qPrintable(engine.lastError()));
    auto* item = qobject_cast<QQuickItem*>(page.get());
    QVERIFY(item);
    QVERIFY(available < page->property("_preferredStatusWidth").toReal());
    QVERIFY(window.show());
    QVERIFY(QQuickTest::qWaitForPolish(window.window(), TestTimeout::mediumMs()));
    auto* box = page->findChild<QQuickItem*>(QStringLiteral("gpsIndicatorGcsPosition"));
    QVERIFY(box && box->isVisible());
    // The column and its boxes take the width that is available, not the width of their widest row.
    QVERIFY2(qAbs(box->width() - available) <= 1, qPrintable(QStringLiteral("box %1").arg(box->width())));
    QVERIFY2(qAbs(item->implicitWidth() - available) <= 1,
             qPrintable(QStringLiteral("page %1").arg(item->implicitWidth())));
}

void GPSIndicatorTest::_pageOffersNtripConnect()
{
    TestFixtures::SettingsFixture saved;
    NTRIPSettings* ntrip = SettingsManager::instance()->ntripSettings();
    saved.setFactValue(ntrip->ntripServerConnectEnabled(), false);
    GPSTest::CasterHost caster(GPSManager::instance()->ntrip());
    TestFixtures::QmlWindowFixture window;
    TestFixtures::QmlEngine engine;
    std::unique_ptr<QObject> page = createIndicatorPage(engine, window, nullptr);
    QVERIFY2(page, qPrintable(engine.lastError()));
    auto* section = page->findChild<QObject*>(QStringLiteral("gpsIndicatorNtrip"));
    QVERIFY(section);
    QVERIFY(!section->property("visible").toBool());

    caster.set(QStringLiteral("caster.example"));
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
    TestFixtures::QmlWindowFixture window;
    TestFixtures::QmlEngine engine;
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
    TestFixtures::QmlWindowFixture window;
    TestFixtures::QmlEngine engine;
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
    // Still initializing is worse than authenticated; a receiver with authentication disabled does not count.
    QTest::newRow("initializing-with-ok")
        << NONE << NONE << int(Auth::Ok) << int(Auth::Initializing) << QByteArray() << QByteArrayLiteral("colorYellow");
    QTest::newRow("disabled-with-ok") << NONE << NONE << int(Auth::Disabled) << int(Auth::Ok) << QByteArray()
                                      << QByteArrayLiteral("colorGreen");
    // A value above those MAVLink defines today counts as worse, as VehicleGPSFactGroup ranks within one receiver.
    QTest::newRow("undefined-interference") << 4 << 1 << NONE << NONE << QByteArrayLiteral("colorRed") << QByteArray();
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
    TestFixtures::QmlWindowFixture window;
    TestFixtures::QmlEngine engine;
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
    QTest::addColumn<GPSBaseState>("baseState");
    QTest::addColumn<int>("correctionState");
    QTest::addColumn<QString>("label");
    QTest::addColumn<QString>("detail");
    const auto row = [](const char* name, RTKSettings::ReceiverRole role, int fixType, GPSBaseState baseState,
                        State corrections, const char* label, const char* detail) {
        QTest::newRow(name) << int(role) << fixType << baseState << int(corrections) << label << detail;
    };
    row("passive-position-only", RTKSettings::Passive, 3, GPSBaseState::Inactive, State::Inactive, "GNSS", "3D");
    row("passive-float", RTKSettings::Passive, 5, GPSBaseState::Inactive, State::Waiting, "RTK", "Float");
    row("base-surveying", RTKSettings::ConfiguredBase, 0, GPSBaseState::Active, State::Waiting, "RTK", "Survey");
    row("base-valid", RTKSettings::ConfiguredBase, 0, GPSBaseState::Valid, State::Fresh, "RTK", "Base");
    // A base without a valid position sends no corrections, so it does not read as a base.
    row("base-without-position", RTKSettings::ConfiguredBase, 0, GPSBaseState::Inactive, State::Waiting, "RTK",
        "Waiting");
}

void GPSIndicatorTest::_showsReceiverWithoutVehicleGPS()
{
    QFETCH(int, role);
    QFETCH(int, fixType);
    QFETCH(GPSBaseState, baseState);
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
    TestFixtures::QmlWindowFixture window;
    TestFixtures::QmlEngine engine;
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

    ScriptedReceiverWorker* const worker = workers.current();
    worker->ready();
    worker->position(fixReport(gpsFixQualityFromValue(fixType)));
    GPSSatelliteReport used;
    used.used = 9;
    worker->satellites(used);
    worker->baseStatus({.state = baseState});
    QVERIFY(rtkLabel->property("visible").toBool());
    QCOMPARE(rtkLabel->property("text").toString(), label);
    QVERIFY(satellites->property("visible").toBool());
    QCOMPARE(satellites->property("text").toString(), QStringLiteral("9"));
    QCOMPARE(detailLabel->property("text").toString(), detail);
}

void GPSIndicatorTest::_showsPositionReceiverWithoutGnssReceiver()
{
    // The application's two receivers: the GNSS receiver, a base, and the position receiver, which is only read.
    ScriptedGPSReceiver gnss;
    ScriptedGPSReceiver position;
    for (GPSReceiver* const receiver : {&gnss.receiver, &position.receiver}) {
        receiver->facts()->setLiveUpdates(true);
    }
    TestFixtures::QmlWindowFixture window;
    TestFixtures::QmlEngine engine;
    std::unique_ptr<QObject> indicator =
        createIndicator(engine, window,
                        {{QStringLiteral("_receiver"), QVariant::fromValue(&gnss.receiver)},
                         {QStringLiteral("_positionReceiver"), QVariant::fromValue(&position.receiver)},
                         {QStringLiteral("_correctionState"), int(GPSCorrectionManager::State::Inactive)}});
    QVERIFY2(indicator, qPrintable(engine.lastError()));
    auto* label = indicator->findChild<QObject*>(QStringLiteral("gpsCorrectionsLabel"));
    auto* satellites = indicator->findChild<QObject*>(QStringLiteral("gpsSatelliteCount"));
    auto* detail = indicator->findChild<QObject*>(QStringLiteral("gpsDetail"));
    QVERIFY(label && satellites && detail);
    const QGCPalette palette;
    const auto shown = [&] {
        return satellites->property("visible").toBool()
                   ? QStringLiteral("%1 %2").arg(satellites->property("text").toString(),
                                                 detail->property("text").toString())
                   : QString();
    };
    const auto report = [](ScriptedGPSReceiver& receiver, int used, GPSIntegrityReport::AntennaState antenna) {
        GPSPositionReport fix = fixReport(GPSFixQuality::Fix3D);
        fix.integrity.antenna.state = antenna;
        receiver.workers.current()->position(fix);
        GPSSatelliteReport counted;
        counted.used = used;
        receiver.workers.current()->satellites(counted);
    };
    QVERIFY(shown().isEmpty());
    QVERIFY(!label->property("visible").toBool());

    // The position receiver alone is shown as the GNSS receiver is: the satellites it uses and its fix.
    QVERIFY(connectOverTcp(position.receiver, receiverConfiguration(kPassiveManufacturer)));
    position.workers.current()->ready();
    report(position, 7, GPSIntegrityReport::AntennaState::Ok);
    QCOMPARE(shown(), QStringLiteral("7 3D"));
    QVERIFY(label->property("visible").toBool());
    QCOMPARE(colorOf(label), palette.textPrimary());
    // Its warnings want attention as the GNSS receiver's do.
    report(position, 7, GPSIntegrityReport::AntennaState::Open);
    QCOMPARE(colorOf(label), palette.colorOrange());

    // A connected GNSS receiver comes first, a base in a word, and the position receiver's warning stays.
    QVERIFY(connectOverTcp(gnss.receiver));
    gnss.workers.current()->ready();
    report(gnss, 12, GPSIntegrityReport::AntennaState::Ok);
    gnss.workers.current()->baseStatus({.state = GPSBaseState::Valid});
    QCOMPARE(shown(), QStringLiteral("12 Base"));
    QCOMPARE(colorOf(label), palette.colorOrange());
    report(position, 7, GPSIntegrityReport::AntennaState::Ok);
    QCOMPARE(colorOf(label), palette.textPrimary());

    // Without it, the position receiver is shown again.
    gnss.receiver.disconnectReceiver();
    QCOMPARE(shown(), QStringLiteral("7 3D"));
    position.receiver.disconnectReceiver();
    QVERIFY(shown().isEmpty());
}

void GPSIndicatorTest::_labelFollowsVehicleFix_data()
{
    using State = GPSCorrectionManager::State;
    QTest::addColumn<int>("fixType");
    QTest::addColumn<int>("correctionState");
    QTest::addColumn<bool>("shown");
    QTest::addColumn<QString>("label");
    QTest::addColumn<bool>("warning");
    const auto row = [](const char* name, int fixType, State corrections, bool shown, const char* label, bool warning) {
        QTest::newRow(name) << fixType << int(corrections) << shown << label << warning;
    };
    row("3d-without-corrections", GPS_FIX_TYPE_3D_FIX, State::Inactive, false, "GNSS", false);
    // A correction source on the ground is no RTK: the label waits for the vehicle's fix, in the warning colour.
    row("3d-corrections-waiting", GPS_FIX_TYPE_3D_FIX, State::Waiting, true, "GNSS", true);
    row("3d-corrections-fresh", GPS_FIX_TYPE_3D_FIX, State::Fresh, true, "GNSS", true);
    row("dgps-corrections-fresh", GPS_FIX_TYPE_DGPS, State::Fresh, true, "GNSS", true);
    row("float", GPS_FIX_TYPE_RTK_FLOAT, State::Fresh, true, "Float", false);
    row("fixed", GPS_FIX_TYPE_RTK_FIXED, State::Fresh, true, "RTK", false);
    // The vehicle still has its fix after the stream stopped.
    row("fixed-corrections-waiting", GPS_FIX_TYPE_RTK_FIXED, State::Waiting, true, "RTK", true);
    // Corrections that reach the vehicle another way.
    row("fixed-without-corrections", GPS_FIX_TYPE_RTK_FIXED, State::Inactive, true, "RTK", false);
}

void GPSIndicatorTest::_labelFollowsVehicleFix()
{
    QFETCH(int, fixType);
    QFETCH(int, correctionState);
    QFETCH(bool, shown);
    QFETCH(QString, label);
    QFETCH(bool, warning);
    Vehicle vehicle(MAV_AUTOPILOT_PX4, MAV_TYPE_QUADROTOR);
    auto* gps = vehicle.gpsFactGroup();
    QVERIFY(gps);
    gps->setLiveUpdates(true);
    gps->handleMessage(&vehicle,
                       GPSTest::gpsRawMessage({.fixType = static_cast<uint8_t>(fixType), .satellitesVisible = 12}));
    TestFixtures::QmlWindowFixture window;
    TestFixtures::QmlEngine engine;
    std::unique_ptr<QObject> indicator =
        engine.create(TestFixtures::sourceQmlUrl(QStringLiteral("Toolbar/GPSIndicator.qml")),
                      {{QStringLiteral("parent"), QVariant::fromValue(window.contentItem())},
                       {QStringLiteral("_activeVehicle"), QVariant::fromValue(&vehicle)},
                       {QStringLiteral("_correctionState"), correctionState}});
    QVERIFY2(indicator, qPrintable(engine.lastError()));
    auto* fixLabel = indicator->findChild<QObject*>(QStringLiteral("gpsCorrectionsLabel"));
    QVERIFY(fixLabel);
    QCOMPARE(fixLabel->property("visible").toBool(), shown);
    QCOMPARE(fixLabel->property("text").toString(), label);
    const QGCPalette palette;
    QCOMPARE(colorOf(fixLabel), warning ? palette.colorOrange() : palette.textPrimary());

    // The label follows the fix the vehicle reports next.
    gps->handleMessage(&vehicle, GPSTest::gpsRawMessage({.fixType = GPS_FIX_TYPE_RTK_FLOAT, .satellitesVisible = 12}));
    QCOMPARE(fixLabel->property("text").toString(), QStringLiteral("Float"));
    QVERIFY(fixLabel->property("visible").toBool());
}

void GPSIndicatorTest::_receiverAntennaWarning()
{
    ScriptedGPSReceiver harness;
    GPSReceiver& receiver = harness.receiver;
    GPSReceiverFactGroup& facts = *receiver.facts();
    facts.setLiveUpdates(true);
    TestFixtures::QmlWindowFixture window;
    TestFixtures::QmlEngine engine;
    auto status = engine.create(TestFixtures::sourceQmlUrl(QStringLiteral("GPS/Qml/GPSReceiverStatus.qml")),
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
    QCOMPARE(colorOf(label), palette.textPrimary());
    for (const auto& [state, text, warning] :
         {std::tuple{GPSIntegrityReport::AntennaState::Ok, QStringLiteral("OK"), false},
          std::tuple{GPSIntegrityReport::AntennaState::Open, QStringLiteral("Open"), true},
          std::tuple{GPSIntegrityReport::AntennaState::Short, QStringLiteral("Short"), true}}) {
        facts.antennaState()->setRawValue(static_cast<int>(state));
        QVERIFY(antenna->property("visible").toBool());
        QCOMPARE(antenna->property("labelText").toString(), text);
        // A fault colours the indicator as jamming does.
        QCOMPARE(colorOf(label), warning ? palette.colorOrange() : palette.textPrimary());
    }
    receiver.disconnectReceiver();
    QVERIFY(!antenna->property("visible").toBool());
    QCOMPARE(colorOf(label), palette.textPrimary());
}

UT_REGISTER_TEST(GPSIndicatorTest, TestLabel::Unit)
