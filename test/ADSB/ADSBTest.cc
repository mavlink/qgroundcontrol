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
    QVERIFY(server->listen(QHostAddress::SpecialAddress::AnyIPv4, 30003));
    ADSBTCPLink* const adsbLink = new ADSBTCPLink(QHostAddress::LocalHost, 30003, this);
    QVERIFY(adsbLink);
    QVERIFY(adsbLink->init());
    // QSignalSpy spy(adsbLink, &ADSBTCPLink::adsbVehicleUpdate);
    bool timeout = false;
    QVERIFY(server->waitForNewConnection(1000, &timeout));
    QVERIFY(!timeout);
    QTcpSocket* const clientSocket = server->nextPendingConnection();
    QVERIFY(clientSocket != nullptr);
    const QByteArray message("MSG,8D4840D6202CC371C32CE0576098");
    for (uint8_t i = 0; i < 50; i++) {
        (void)clientSocket->write(message);
    }
    // QVERIFY(spy.wait(5000));
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
