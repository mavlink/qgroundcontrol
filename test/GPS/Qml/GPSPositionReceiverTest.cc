#include "GPSPositionReceiverTest.h"

#include <memory>

#include <QtQuick/QQuickItem>
#include <QtQuickTest/quicktest.h>

#include "Fact.h"
#include "Fixtures/RAIIFixtures.h"
#include "GPSManager.h"
#include "GPSReceiver.h"
#include "GPSReceiverFactGroup.h"
#include "GPSSettingsBindings.h"
#include "PositionReceiverSettings.h"
#include "QmlTesting/QmlFixtures.h"
#include "RTKSettings.h"
#include "Receiver/Support/GPSReceiverTestSupport.h"
#include "Receiver/Support/ScriptedReceiverWorker.h"
#include "SettingsManager.h"

using namespace GPSTest;

namespace {

/// The Position Receiver section over settings of its own and a scripted receiver that follows them, as the
/// application's position receiver follows the application's. The settings start as a new installation has them.
struct Section
{
    Section()
    {
        GPSSettingsBindings::bindPositionReceiver(&settings, &receiver);
        receiver.facts()->setLiveUpdates(true);
    }

    // Scripted workers never run, so the receiver's shutdown is what deletes those of its ended sessions.
    ~Section() { receiver.shutdown(); }

    Section(const Section&) = delete;
    Section& operator=(const Section&) = delete;

    /// Creates the section; false on failure, with the reason in the engine's lastError().
    [[nodiscard]] bool create()
    {
        item = engine.create(TestFixtures::sourceQmlUrl(QStringLiteral("GPS/Qml/GPSPositionReceiver.qml")),
                             {{QStringLiteral("receiver"), QVariant::fromValue(&receiver)},
                              {QStringLiteral("settings"), QVariant::fromValue(&settings)}});
        return item != nullptr;
    }

    template <typename T = QQuickItem>
    T* find(const char* name) const
    {
        return item->findChild<T*>(QString::fromLatin1(name));
    }

    /// The section shows the control @a name.
    bool shows(const char* name) const
    {
        const QQuickItem* const control = find(name);
        return control && control->isVisible();
    }

    /// Selects a receiver on a TCP address, which scripted workers never open.
    void useTcp()
    {
        settings.connectionType()->setRawValue(RTKSettings::Tcp);
        settings.tcpHost()->setRawValue(QStringLiteral("position.test"));
        settings.tcpPort()->setRawValue(2102);
    }

    PositionReceiverSettings settings;
    ScriptedGPSReceiver scripted;
    GPSReceiver& receiver = scripted.receiver;
    TestFixtures::QmlEngine engine;
    std::unique_ptr<QObject> item;
};

}  // namespace

void GPSPositionReceiverTest::_showsConnectionOnceInUse()
{
    Section section;
    QVERIFY2(section.create(), qPrintable(section.engine.lastError()));
    auto* const toggle = section.find("positionReceiverEnabled");
    QVERIFY(toggle);

    // Out of use, as a new installation has it, the section is its switch: there is nothing to connect.
    QVERIFY(toggle->isVisible());
    QVERIFY(!toggle->property("checked").toBool());
    for (const char* control : {"positionReceiverConnectionType", "positionReceiverTcpHost", "positionReceiverTcpPort",
                                "positionReceiverUdpPort", "positionReceiverSerialPort", "positionReceiverOutputNote",
                                "rtkRecordReceiverData"}) {
        QVERIFY2(!section.shows(control), control);
    }
    QVERIFY(!section.find("positionReceiverStatus"));
    QVERIFY(!section.find("rtkConnectButton"));

    // The switch puts the receiver in use: its connection, the recording switch and its status with Connect appear.
    QVERIFY(QMetaObject::invokeMethod(toggle, "click"));
    QVERIFY(section.settings.enabled()->rawValue().toBool());
    QVERIFY(section.receiver.configuration().autoConnect);
    QVERIFY(section.shows("positionReceiverConnectionType"));
    QVERIFY(section.shows("positionReceiverOutputNote"));
    QVERIFY(section.shows("rtkRecordReceiverData"));
    QVERIFY(section.shows("positionReceiverStatus"));
    QVERIFY(section.shows("rtkConnectButton"));

    // Each connection has its own fields. Without serial links a serial selection connects over TCP.
    const bool serial = section.receiver.serialSupported();
    section.settings.connectionType()->setRawValue(RTKSettings::Serial);
    QCOMPARE(section.shows("positionReceiverSerialPort"), serial);
    QCOMPARE(section.shows("positionReceiverTcpHost"), !serial);
    QVERIFY(!section.shows("positionReceiverUdpPort"));
    section.settings.connectionType()->setRawValue(RTKSettings::Tcp);
    QVERIFY(section.shows("positionReceiverTcpHost") && section.shows("positionReceiverTcpPort"));
    QVERIFY(!section.shows("positionReceiverSerialPort") && !section.shows("positionReceiverUdpPort"));
    section.settings.connectionType()->setRawValue(RTKSettings::Udp);
    QVERIFY(section.shows("positionReceiverUdpPort"));
    QVERIFY(!section.shows("positionReceiverTcpHost") && !section.shows("positionReceiverSerialPort"));
    // UDP only receives, which is all a receiver that is never configured needs.
    QVERIFY(section.find("rtkConnectButton")->isEnabled());
}

void GPSPositionReceiverTest::_connectsAndDisconnectsFromSection()
{
    Section section;
    section.useTcp();
    section.settings.enabled()->setRawValue(true);
    QVERIFY2(section.create(), qPrintable(section.engine.lastError()));
    auto* const toggle = section.find("positionReceiverEnabled");
    auto* const host = section.find("positionReceiverTcpHost");
    auto* const connect = section.find("rtkConnectButton");
    auto* const status = section.find("rtkReceiverStatus");
    auto* const fix = section.find("rtkFixType");
    QVERIFY(toggle && host && connect && status && fix);
    QVERIFY(host->isEnabled());

    // Connect opens the saved connection, as a receiver that is sent nothing, and locks the form.
    QVERIFY(QMetaObject::invokeMethod(connect, "click"));
    ScriptedReceiverWorker* const worker = section.scripted.workers.current();
    QVERIFY(worker);
    QCOMPARE(worker->type(), GPSType::Passive);
    QVERIFY(!host->isEnabled());
    QVERIFY(!section.shows("positionReceiverOutputNote"));

    // The status is the receiver's: what it is doing, and the fix it reports.
    worker->ready();
    QCOMPARE(section.receiver.state(), GPSReceiver::State::Connected);
    QCOMPARE(status->property("statusText").toString(), section.receiver.statusText());
    QVERIFY(!section.receiver.statusText().isEmpty());
    QVERIFY(!fix->isVisible());
    worker->position(fixReport(GPSFixQuality::Fix3D));
    QVERIFY(fix->isVisible());

    // The same button disconnects, which pauses the automatic connection and unlocks the form.
    QVERIFY(QMetaObject::invokeMethod(connect, "click"));
    QCOMPARE(section.receiver.state(), GPSReceiver::State::Disconnected);
    QVERIFY(host->isEnabled());

    // Turning the switch off disconnects a connected receiver, and its status goes.
    QVERIFY(QMetaObject::invokeMethod(connect, "click"));
    QVERIFY(section.receiver.hasReceiver());
    QVERIFY(QMetaObject::invokeMethod(toggle, "click"));
    QVERIFY(!section.settings.enabled()->rawValue().toBool());
    QVERIFY(!section.receiver.hasReceiver());
    QVERIFY(!host->isVisible());
    QTRY_VERIFY_WITH_TIMEOUT(!section.find("positionReceiverStatus"), TestTimeout::shortMs());
}

void GPSPositionReceiverTest::_dropDownShowsStatusWhileInUse()
{
    TestFixtures::SettingsFixture saved;
    Fact* const enabled = SettingsManager::instance()->positionReceiverSettings()->enabled();
    saved.setFactValue(enabled, false);
    TestFixtures::QmlWindowFixture window;
    TestFixtures::QmlEngine engine;
    const std::unique_ptr<QObject> page =
        engine.create(TestFixtures::sourceQmlUrl(QStringLiteral("Toolbar/GPSIndicatorPage.qml")),
                      {{QStringLiteral("parent"), QVariant::fromValue(window.contentItem())},
                       {QStringLiteral("activeVehicle"), QVariant::fromValue(static_cast<QObject*>(nullptr))},
                       {QStringLiteral("availableWidth"), 640}});
    QVERIFY2(page, qPrintable(engine.lastError()));
    QVERIFY(window.show());
    const auto connectButtons = [&page]() {
        return TestFixtures::findItems(qobject_cast<QQuickItem*>(page.get()), [](QQuickItem* item) {
            return item->isVisible() && item->objectName() == QLatin1String("rtkConnectButton");
        });
    };

    // Out of use, the drop-down has the GNSS receiver's status and Connect alone.
    QVERIFY(!page->findChild<QQuickItem*>(QStringLiteral("positionReceiverStatus")));
    QCOMPARE(connectButtons().size(), 1);

    // In use, the position receiver's status follows the GNSS receiver's, with a Connect of its own.
    enabled->setRawValue(true);
    auto* const status = page->findChild<QQuickItem*>(QStringLiteral("positionReceiverStatus"));
    QVERIFY(status && status->isVisible());
    QCOMPARE(status->property("receiver").value<GPSReceiver*>(), GPSManager::instance()->positionReceiver());
    QCOMPARE(status->property("heading").toString(), QStringLiteral("Position Receiver Status"));
    QCOMPARE(connectButtons().size(), 2);
    // It is laid out as the other groups of the status column are: as wide, and above the last of them.
    QVERIFY(QQuickTest::qWaitForPolish(window.window(), TestTimeout::mediumMs()));
    auto* const gcsPosition = page->findChild<QQuickItem*>(QStringLiteral("gpsIndicatorGcsPosition"));
    QVERIFY(gcsPosition && gcsPosition->width() > 0);
    QCOMPARE(status->width(), gcsPosition->width());
    QVERIFY(status->height() > 0);
    const qreal bottom = status->mapToItem(gcsPosition->parentItem(), QPointF(0, status->height())).y();
    QVERIFY2(bottom <= gcsPosition->y(), qPrintable(QStringLiteral("%1 > %2").arg(bottom).arg(gcsPosition->y())));
}

UT_REGISTER_TEST(GPSPositionReceiverTest, TestLabel::Unit)
