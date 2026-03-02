#include "ADSBTest.h"

#include <QtNetwork/QTcpServer>
#include <QtTest/QSignalSpy>

#include "ADSBTCPLink.h"
#include "ADSBVehicle.h"
#include "ADSBVehicleManager.h"
#include "QmlObjectListModel.h"

void ADSBTest::_adsbVehicleTest()
{
    ADSB::VehicleInfo_t vehicleInfo;
    vehicleInfo.icaoAddress = 1;
    vehicleInfo.callsign = QStringLiteral("1");
    vehicleInfo.location = QGeoCoordinate(1., 1., 1.);
    vehicleInfo.heading = 1.;
    vehicleInfo.alert = false;
    vehicleInfo.availableFlags = ADSB::CallsignAvailable;
    ADSBVehicle* const adsbVehicle = new ADSBVehicle(vehicleInfo, this);
    QVERIFY(adsbVehicle != nullptr);
    QVERIFY(!adsbVehicle->expired());
    QCOMPARE(adsbVehicle->icaoAddress(), vehicleInfo.icaoAddress);
    QCOMPARE(adsbVehicle->callsign(), vehicleInfo.callsign);
    QCOMPARE_NE(adsbVehicle->coordinate(), vehicleInfo.location);
    ADSB::VehicleInfo_t vehicleInfo2;
    vehicleInfo2.icaoAddress = 2;
    vehicleInfo2.callsign = QStringLiteral("2");
    vehicleInfo2.location = QGeoCoordinate(2., 2.);
    vehicleInfo2.availableFlags = ADSB::CallsignAvailable | ADSB::LocationAvailable;
    adsbVehicle->update(vehicleInfo2);
    QCOMPARE_NE(adsbVehicle->callsign(), vehicleInfo2.callsign);
    QCOMPARE_NE(adsbVehicle->coordinate(), vehicleInfo2.location);
    vehicleInfo2.icaoAddress = 1;
    adsbVehicle->update(vehicleInfo2);
    QCOMPARE(adsbVehicle->callsign(), vehicleInfo2.callsign);
    QCOMPARE(adsbVehicle->coordinate(), vehicleInfo2.location);
}

void ADSBTest::_adsbTcpLinkTest()
{
    QTcpServer* const server = new QTcpServer(this);
    QVERIFY(server);
    QVERIFY(server->listen(QHostAddress::LocalHost, 0));
    const quint16 serverPort = server->serverPort();
    QVERIFY(serverPort > 0);
    ADSBTCPLink* const adsbLink = new ADSBTCPLink(QHostAddress::LocalHost, serverPort, this);
    QVERIFY(adsbLink);
    QVERIFY(adsbLink->init());
    QSignalSpy spy(adsbLink, &ADSBTCPLink::adsbVehicleUpdate);
    bool timeout = false;
    QVERIFY(server->waitForNewConnection(TestTimeout::mediumMs(), &timeout));
    QVERIFY(!timeout);
    QTcpSocket* const clientSocket = server->nextPendingConnection();
    QVERIFY(clientSocket != nullptr);
    // Send valid SBS-1 BaseStation format messages (MSG type 3 has lat/lon)
    // Field 19 (alert) must be a valid integer for _parseAndEmitLocation to emit
    const QByteArray message(
        "MSG,3,1,1,4840D6,1,2024/01/01,12:00:00.000,2024/01/01,12:00:00.000,,35000,,,47.0,-122.0,,,,0,,0\n");
    for (uint8_t i = 0; i < 50; i++) {
        clientSocket->write(message);
    }
    clientSocket->flush();
    clientSocket->waitForBytesWritten(TestTimeout::shortMs());
    QVERIFY_SIGNAL_WAIT(spy, TestTimeout::mediumMs());
    QVERIFY(spy.count() > 0);
    server->close();
}

void ADSBTest::_adsbTcpLinkRejectsNullHostTest()
{
    ADSBTCPLink* const adsbLink = new ADSBTCPLink(QHostAddress(), 30003, this);
    QVERIFY(adsbLink);
    QVERIFY(!adsbLink->init());
}

void ADSBTest::_adsbTcpLinkIgnoresInvalidMessagesTest()
{
    QTcpServer* const server = new QTcpServer(this);
    QVERIFY(server);
    QVERIFY(server->listen(QHostAddress::LocalHost, 0));
    const quint16 serverPort = server->serverPort();
    QVERIFY(serverPort > 0);

    ADSBTCPLink* const adsbLink = new ADSBTCPLink(QHostAddress::LocalHost, serverPort, this);
    QVERIFY(adsbLink);
    QVERIFY(adsbLink->init());

    QSignalSpy spy(adsbLink, &ADSBTCPLink::adsbVehicleUpdate);
    bool timeout = false;
    QVERIFY(server->waitForNewConnection(TestTimeout::mediumMs(), &timeout));
    QVERIFY(!timeout);
    QTcpSocket* const clientSocket = server->nextPendingConnection();
    QVERIFY(clientSocket != nullptr);

    const QByteArray invalidNotMsg("NOT,3,1,1,4840D6,1,2024/01/01,12:00:00.000\n");
    const QByteArray invalidType("MSG,9,1,1,4840D6,1,2024/01/01,12:00:00.000\n");
    const QByteArray invalidIcao(
        "MSG,3,1,1,ZZZZZZ,1,2024/01/01,12:00:00.000,2024/01/01,12:00:00.000,,35000,,,47.0,-122.0,,,,0,,0\n");
    const QByteArray zeroLatLon(
        "MSG,3,1,1,4840D6,1,2024/01/01,12:00:00.000,2024/01/01,12:00:00.000,,35000,,,0,0,,,,0,,0\n");
    clientSocket->write(invalidNotMsg);
    clientSocket->write(invalidType);
    clientSocket->write(invalidIcao);
    clientSocket->write(zeroLatLon);
    clientSocket->flush();
    clientSocket->waitForBytesWritten(TestTimeout::shortMs());

    QVERIFY_NO_SIGNAL_WAIT(spy, TestTimeout::shortMs());
    QCOMPARE(spy.count(), 0);

    server->close();
}

void ADSBTest::_adsbTcpLinkCallsignMessageTest()
{
    QTcpServer* const server = new QTcpServer(this);
    QVERIFY(server);
    QVERIFY(server->listen(QHostAddress::LocalHost, 0));
    const quint16 serverPort = server->serverPort();
    QVERIFY(serverPort > 0);

    ADSBTCPLink* const adsbLink = new ADSBTCPLink(QHostAddress::LocalHost, serverPort, this);
    QVERIFY(adsbLink);
    QVERIFY(adsbLink->init());

    QSignalSpy spy(adsbLink, &ADSBTCPLink::adsbVehicleUpdate);
    bool timeout = false;
    QVERIFY(server->waitForNewConnection(TestTimeout::mediumMs(), &timeout));
    QVERIFY(!timeout);
    QTcpSocket* const clientSocket = server->nextPendingConnection();
    QVERIFY(clientSocket != nullptr);

    const QByteArray callsignMessage(
        "MSG,1,1,1,ABCDEF,1,2024/01/01,12:00:00.000,2024/01/01,12:00:00.000,CALL123,,,,,,,,,,\n");
    clientSocket->write(callsignMessage);
    clientSocket->flush();
    clientSocket->waitForBytesWritten(TestTimeout::shortMs());

    QVERIFY_SIGNAL_WAIT(spy, TestTimeout::mediumMs());
    QCOMPARE(spy.count(), 1);
    const ADSB::VehicleInfo_t vehicleInfo = spy.takeFirst().at(0).value<ADSB::VehicleInfo_t>();
    QCOMPARE(vehicleInfo.icaoAddress, static_cast<uint32_t>(0xABCDEF));
    QCOMPARE(vehicleInfo.callsign, QStringLiteral("CALL123"));
    QVERIFY(vehicleInfo.availableFlags.testFlag(ADSB::CallsignAvailable));

    server->close();
}

void ADSBTest::_adsbVehicleManagerTest()
{
    ADSBVehicleManager* const manager = ADSBVehicleManager::instance();
    QVERIFY(manager);
    QVERIFY(manager->adsbVehicles());

    // Capture initial count - singleton may have vehicles from previous tests
    const int initialCount = manager->adsbVehicles()->count();

    // Use unique ICAO address to ensure we add a new vehicle
    ADSB::VehicleInfo_t vehicleInfo;
    vehicleInfo.icaoAddress = 0xDEADBEEF;
    vehicleInfo.callsign = QStringLiteral("TEST1");
    vehicleInfo.location = QGeoCoordinate(1., 1., 1.);
    vehicleInfo.heading = 1.;
    vehicleInfo.alert = false;
    vehicleInfo.availableFlags = ADSB::LocationAvailable;
    manager->adsbVehicleUpdate(vehicleInfo);
    QCOMPARE(manager->adsbVehicles()->count(), initialCount + 1);
}

UT_REGISTER_TEST(ADSBTest, TestLabel::Unit)
