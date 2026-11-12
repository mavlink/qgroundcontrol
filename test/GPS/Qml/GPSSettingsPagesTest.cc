#include "GPSSettingsPagesTest.h"

#include <memory>

#include <QtCore/QScopeGuard>
#include <QtCore/QTime>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtQuick/QQuickItem>

#include "Fact.h"
#include "Fixtures/RAIIFixtures.h"
#include "GPSCorrectionManager.h"
#include "GPSCorrectionSettings.h"
#include "GPSManager.h"
#include "GPSReceiver.h"
#include "NTRIP/Support/ScriptedNTRIPCaster.h"
#include "NTRIPManager.h"
#include "NTRIPSettings.h"
#include "PositionManager.h"
#include "PositionReceiverSettings.h"
#include "Protocols/Support/ProtocolTestPackets.h"
#include "QmlTesting/QmlFixtures.h"
#include "RTKSettings.h"
#include "SettingsManager.h"
#include "Support/GPSTestHelpers.h"

using namespace GPSTest;

namespace {

/// A settings page of the application by its QML type, created outside the main window.
std::unique_ptr<QObject> createPage(TestFixtures::QmlEngine& engine, const QByteArray& type)
{
    return engine.create(QByteArrayLiteral(R"(
        import QtQuick
        import QGroundControl
        import QGroundControl.AppSettings

        Item {
            // The page's scroll area takes its palette from the window that shows the page.
            QGCPalette { id: qgcPal }
            )") + type + QByteArrayLiteral(R"( {}
        }
    )"));
}

/// The text input of the field @a name of @a page, which a labelled field holds; null unless it has exactly one.
QQuickItem* textInput(const QObject* page, const char* name)
{
    const QList<QQuickItem*> inputs =
        TestFixtures::findItems(page->findChild<QQuickItem*>(QString::fromLatin1(name)),
                                [](QQuickItem* item) { return item->inherits("QQuickTextInput"); });
    return inputs.size() == 1 ? inputs.constFirst() : nullptr;
}

/// Enters @a text in @a input as typing it and leaving the field does; false for a field that cannot be edited.
bool enter(QQuickItem* input, const QString& text)
{
    return input->isVisible() && input->isEnabled() && input->setProperty("text", text) &&
           QMetaObject::invokeMethod(input, "editingFinished");
}

/// Clicks @a button; false for a button that is hidden or disabled.
bool press(QObject* button)
{
    return button->property("visible").toBool() && button->property("enabled").toBool() &&
           QMetaObject::invokeMethod(button, "click");
}

/// The value the row @a name of @a page shows; empty while the row is hidden.
QString rowValue(const QObject* page, const char* name)
{
    const auto* const row = page->findChild<QQuickItem*>(QString::fromLatin1(name));
    return row && row->isVisible() ? row->property("labelText").toString() : QString();
}

/// Whether a visible text of @a page reads @a text.
bool showsText(QObject* page, const QString& text)
{
    return !TestFixtures::findItems(qobject_cast<QQuickItem*>(page), [&text](QQuickItem* item) {
                return item->isVisible() && item->property("lineCount").isValid() &&
                       item->property("text").toString() == text;
            }).isEmpty();
}

/// The sentences of the @a second-th epoch of an NMEA receiver that has a fix on eight satellites.
QByteArray nmeaEpoch(int second)
{
    const QByteArray time = QTime(9, 27, 50).addSecs(second).toString(QStringLiteral("HHmmss.zzz")).toLatin1();
    const QByteArrayList sentences{"$GPRMC," + time + ",A,5321.6802,N,00630.3372,W,0.02,31.66,280511,,,A",
                                   "$GPGST," + time + ",1,1,1,0,1,1,2",
                                   "$GPGGA," + time + ",5321.6802,N,00630.3372,W,1,8,1.03,61.7,M,55.2,M,,"};
    QByteArray epoch;
    for (const QByteArray& sentence : sentences) {
        epoch += repairChecksum(sentence);
    }
    return epoch;
}

}  // namespace

void GPSSettingsPagesTest::initTestCase()
{
    UnitTest::initTestCase();
    SettingsManager* const settings = SettingsManager::instance();
    // From here until the process ends, the application's receiver follows the saved settings and looks every second
    // for a connection to open. Connecting by itself stays off for all of that time: the default settings would
    // configure an RTK receiver plugged into this machine as a base station.
    settings->rtkSettings()->autoConnect()->setRawValue(false);
    settings->ntripSettings()->ntripServerConnectEnabled()->setRawValue(false);
    GPSManager* const gps = GPSManager::instance();
    // Unit tests never create Qt's platform source.
    gps->positionManager()->setPlatformSourceFactory([](QObject* parent) {
        auto* const source = new PositionSource;
        source->setParent(parent);
        return source;
    });
    gps->init();
}

void GPSSettingsPagesTest::_receiverConnectsFromPage()
{
    // A receiver that sends NMEA on a TCP port of this machine, as a serial-to-TCP bridge serves one.
    QTcpServer bridge;
    QVERIFY(bridge.listen(QHostAddress::LocalHost));
    const QString address = QStringLiteral("127.0.0.1");
    TestFixtures::SettingsFixture saved;
    RTKSettings* const settings = SettingsManager::instance()->rtkSettings();
    saved.setFactValue(settings->receiverRole(), RTKSettings::Passive);
    saved.setFactValue(settings->forwardReceiverRtcm(), false);
    saved.setFactValue(settings->connectionType(), RTKSettings::Tcp);
    saved.setFactValue(settings->tcpHost(), QString());
    saved.setFactValue(settings->tcpPort(), 0);
    GPSManager* const gps = GPSManager::instance();
    GPSReceiver* const receiver = gps->receiver();
    const auto disconnected = qScopeGuard([receiver]() { receiver->disconnectReceiver(); });

    TestFixtures::QmlEngine engine;
    const std::unique_ptr<QObject> page = createPage(engine, "GNSSReceiverSettings");
    QVERIFY2(page, qPrintable(engine.lastError()));
    auto* const connect = page->findChild<QObject*>(QStringLiteral("rtkConnectButton"));
    auto* const status = page->findChild<QObject*>(QStringLiteral("rtkReceiverStatus"));
    QQuickItem* const host = textInput(page.get(), "rtkTcpHost");
    QQuickItem* const port = textInput(page.get(), "rtkTcpPort");
    QVERIFY(connect && status && host && port);

    // The address entered in the form is saved, and the receiver has it before Connect is pressed.
    QVERIFY(!receiver->hasEndpoint());
    QVERIFY(enter(host, address));
    QVERIFY(enter(port, QString::number(bridge.serverPort())));
    QCOMPARE(settings->tcpHost()->rawValue().toString(), address);
    QCOMPARE(settings->tcpPort()->rawValue().toUInt(), bridge.serverPort());
    QVERIFY(receiver->hasEndpoint());

    QVERIFY(press(connect));
    QTRY_VERIFY_WITH_TIMEOUT(bridge.hasPendingConnections(), TestTimeout::mediumMs());
    QTcpSocket* const link = bridge.nextPendingConnection();
    QTRY_COMPARE_WITH_TIMEOUT(receiver->state(), GPSReceiver::State::Connected, TestTimeout::mediumMs());
    // The page's status group says so, and where the receiver is; the form is locked meanwhile.
    QVERIFY(!receiver->statusText().isEmpty());
    QCOMPARE(status->property("statusText").toString(), receiver->statusText());
    QCOMPARE(rowValue(page.get(), "rtkReceiverEndpoint"),
             QStringLiteral("%1:%2").arg(address).arg(bridge.serverPort()));
    QVERIFY(!host->isEnabled());

    // What the receiver sends shows in the status group. It keeps reporting, as a receiver does: one epoch alone
    // would go stale while a slow machine catches up.
    int epoch = 0;
    const auto reports = [&](const char* row, const QString& value) {
        link->write(nmeaEpoch(epoch++));
        return rowValue(page.get(), row) == value;
    };
    QTRY_VERIFY_WITH_TIMEOUT(reports("rtkSatellitesUsed", QStringLiteral("8")), TestTimeout::mediumMs());
    QCOMPARE(rowValue(page.get(), "rtkDetectedReceiver"), QStringLiteral("NMEA"));
    // Its position is the ground station's: the page's last group names the receiver as the source in use.
    PositionManager* const positions = gps->positionManager();
    QTRY_VERIFY_WITH_TIMEOUT(reports("gcsPositionSelectedSource", positions->selectedSourceName()) &&
                                 positions->selectedSource() == PositionManager::SelectedSource::Receiver,
                             TestTimeout::mediumMs());

    // The same button disconnects: the link closes, the status goes, and the form can be edited again.
    QVERIFY(press(connect));
    QCOMPARE(receiver->state(), GPSReceiver::State::Disconnected);
    QTRY_COMPARE_WITH_TIMEOUT(link->state(), QAbstractSocket::UnconnectedState, TestTimeout::mediumMs());
    QVERIFY(rowValue(page.get(), "rtkReceiverEndpoint").isEmpty());
    QVERIFY(host->isEnabled());
}

void GPSSettingsPagesTest::_positionReceiverConnectsFromPage()
{
    // The operator's receiver, which sends NMEA on a TCP port of this machine. The GNSS receiver has no connection.
    QTcpServer bridge;
    QVERIFY(bridge.listen(QHostAddress::LocalHost));
    const QString address = QStringLiteral("127.0.0.1");
    TestFixtures::SettingsFixture saved;
    PositionReceiverSettings* const settings = SettingsManager::instance()->positionReceiverSettings();
    saved.setFactValue(settings->enabled(), false);
    saved.setFactValue(settings->connectionType(), RTKSettings::Tcp);
    saved.setFactValue(settings->tcpHost(), QString());
    saved.setFactValue(settings->tcpPort(), 0);
    GPSManager* const gps = GPSManager::instance();
    GPSReceiver* const receiver = gps->positionReceiver();
    const auto disconnected = qScopeGuard([receiver]() { receiver->disconnectReceiver(); });

    TestFixtures::QmlEngine engine;
    const std::unique_ptr<QObject> page = createPage(engine, "GNSSReceiverSettings");
    QVERIFY2(page, qPrintable(engine.lastError()));
    auto* const inUse = page->findChild<QObject*>(QStringLiteral("positionReceiverEnabled"));
    QQuickItem* const host = textInput(page.get(), "positionReceiverTcpHost");
    QQuickItem* const port = textInput(page.get(), "positionReceiverTcpPort");
    QVERIFY(inUse && host && port);
    // Out of use, the section has neither a connection to edit nor a status.
    QVERIFY(!host->isVisible());
    QVERIFY(!page->findChild<QObject*>(QStringLiteral("positionReceiverStatus")));

    // The switch puts the receiver in use, and the address entered in the form is the receiver's.
    QVERIFY(press(inUse));
    QVERIFY(settings->enabled()->rawValue().toBool());
    QVERIFY(enter(host, address));
    QVERIFY(enter(port, QString::number(bridge.serverPort())));
    QVERIFY(receiver->hasEndpoint());
    auto* const status = page->findChild<QObject*>(QStringLiteral("positionReceiverStatus"));
    QVERIFY(status);
    auto* const connect = status->findChild<QObject*>(QStringLiteral("rtkConnectButton"));
    QVERIFY(connect);

    // Its own Connect opens its connection, and its own status shows what it sends. The GNSS receiver stays as it
    // was.
    QVERIFY(press(connect));
    QTRY_VERIFY_WITH_TIMEOUT(bridge.hasPendingConnections(), TestTimeout::mediumMs());
    QTcpSocket* const link = bridge.nextPendingConnection();
    QTRY_COMPARE_WITH_TIMEOUT(receiver->state(), GPSReceiver::State::Connected, TestTimeout::mediumMs());
    QCOMPARE(gps->receiver()->state(), GPSReceiver::State::Disconnected);
    QVERIFY(!host->isEnabled());
    int epoch = 0;
    const auto reports = [&](const QObject* group, const char* row, const QString& value) {
        link->write(nmeaEpoch(epoch++));
        return rowValue(group, row) == value;
    };
    QTRY_VERIFY_WITH_TIMEOUT(reports(status, "rtkSatellitesUsed", QStringLiteral("8")), TestTimeout::mediumMs());
    QCOMPARE(rowValue(status, "rtkReceiverEndpoint"), QStringLiteral("%1:%2").arg(address).arg(bridge.serverPort()));
    // Its position is the ground station's: the page's last group names it as the source in use.
    PositionManager* const positions = gps->positionManager();
    const QString positionReceiver = PositionManager::tr("Position receiver");
    QTRY_VERIFY_WITH_TIMEOUT(reports(page.get(), "gcsPositionSelectedSource", positionReceiver) &&
                                 positions->selectedSource() == PositionManager::SelectedSource::Receiver,
                             TestTimeout::mediumMs());

    // The switch takes it out of use: the link closes, and its status goes with its position.
    QVERIFY(press(inUse));
    QVERIFY(!settings->enabled()->rawValue().toBool());
    QCOMPARE(receiver->state(), GPSReceiver::State::Disconnected);
    QTRY_COMPARE_WITH_TIMEOUT(link->state(), QAbstractSocket::UnconnectedState, TestTimeout::mediumMs());
    QVERIFY(!host->isVisible());
    QVERIFY(rowValue(page.get(), "gcsPositionSelectedSource") != positionReceiver);
    QTRY_VERIFY_WITH_TIMEOUT(!page->findChild<QObject*>(QStringLiteral("positionReceiverStatus")),
                             TestTimeout::shortMs());
}

void GPSSettingsPagesTest::_ntripConnectsFromPage()
{
    ScriptedNTRIPCaster caster;
    QVERIFY(caster.isListening());
    const QString address = QStringLiteral("127.0.0.1");
    TestFixtures::SettingsFixture saved;
    NTRIPSettings* const settings = SettingsManager::instance()->ntripSettings();
    Fact* const enabled = settings->ntripServerConnectEnabled();
    saved.setFactValue(settings->ntripServerHostAddress(), QString());
    saved.setFactValue(settings->ntripServerPort(), settings->ntripServerPort()->rawDefaultValue());
    saved.setFactValue(settings->ntripMountpoint(), QString());
    saved.setFactValue(enabled, false);
    GPSManager* const gps = GPSManager::instance();
    NTRIPManager* const ntrip = gps->ntrip();
    using Status = NTRIPManager::ConnectionStatus;
    // The manager follows the setting a moment later, and must have let go of the caster before that goes away.
    const auto disconnected = qScopeGuard([enabled, ntrip]() {
        enabled->setRawValue(false);
        (void) QTest::qWaitFor([ntrip]() { return ntrip->connectionStatus() == Status::Disconnected; },
                               TestTimeout::mediumMs());
    });

    TestFixtures::QmlEngine engine;
    const std::unique_ptr<QObject> page = createPage(engine, "RTKCorrectionsSettings");
    QVERIFY2(page, qPrintable(engine.lastError()));
    auto* const connect = page->findChild<QObject*>(QStringLiteral("ntripConnectButton"));
    auto* const source = page->findChild<QObject*>(QStringLiteral("correctionsSelectedSource"));
    auto* const stream = page->findChild<QQuickItem*>(QStringLiteral("correctionsSelectedStream"));
    QQuickItem* const host = textInput(page.get(), "settingsTextField_ntripServerHostAddress");
    QQuickItem* const port = textInput(page.get(), "settingsTextField_ntripServerPort");
    QQuickItem* const mountpoint = textInput(page.get(), "ntripMountpointField");
    QVERIFY(connect && source && stream && host && port && mountpoint);

    // Connect needs a caster, which the manager has as soon as the host is entered in the form.
    QVERIFY(!press(connect));
    QVERIFY(enter(host, address));
    QVERIFY(enter(port, QString::number(caster.port())));
    QVERIFY(enter(mountpoint, QStringLiteral("MOUNT")));
    QVERIFY(ntrip->canConnect());

    // Connect turns the saved connection on, which locks the form, and the caster is asked for the mountpoint.
    QVERIFY(press(connect));
    ScriptedNTRIPCaster::Connection* const connection = caster.waitForConnection();
    QVERIFY(connection && connection->peer);
    const QByteArray request = connection->waitForRequest();
    QVERIFY2(request.startsWith("GET /MOUNT "), request.constData());
    QVERIFY(enabled->rawValue().toBool());
    QVERIFY(!host->isEnabled());

    // The caster's corrections make the connection the selected stream, which the page's first group shows. A caster
    // keeps sending: one message alone would go stale while a slow machine catches up.
    const QByteArray correction = rtcmMessage(1005, 20);
    connection->write("HTTP/1.1 200 OK\r\nContent-Type: gnss/data\r\n\r\n");
    const QString ntripName = GPSCorrectionManager::sourceName(static_cast<int>(GPSCorrectionSettings::Ntrip));
    const auto corrected = [&]() {
        connection->write(correction);
        return source->property("labelText").toString() == ntripName;
    };
    QTRY_VERIFY_WITH_TIMEOUT(corrected(), TestTimeout::mediumMs());
    const QString shownStream = stream->isVisible() ? stream->property("text").toString() : QString();
    QVERIFY2(shownStream.endsWith(QStringLiteral("%1:%2/MOUNT").arg(address).arg(caster.port())),
             qPrintable(shownStream));
    QCOMPARE(ntrip->connectionStatus(), Status::Connected);
    QVERIFY(showsText(page.get(), ntrip->statusMessage()));

    // The same button turns the saved connection off: the stream closes, and the form can be edited again.
    QVERIFY(press(connect));
    QTRY_COMPARE_WITH_TIMEOUT(ntrip->connectionStatus(), Status::Disconnected, TestTimeout::mediumMs());
    QVERIFY(!enabled->rawValue().toBool());
    QTRY_VERIFY_WITH_TIMEOUT(!connection->peer || connection->peer->state() == QAbstractSocket::UnconnectedState,
                             TestTimeout::mediumMs());
    QVERIFY(showsText(page.get(), ntrip->statusMessage()));
    QVERIFY(host->isEnabled());
}

UT_REGISTER_TEST_STANDALONE(GPSSettingsPagesTest, TestLabel::Integration)
