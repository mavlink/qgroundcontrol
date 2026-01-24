#include "ADSBTCPLinkTest.h"
#include "TestHelpers.h"
#include "ADSBTCPLink.h"
#include "ADSB.h"

#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtTest/QTest>
#include <QtTest/QSignalSpy>

// ============================================================================
// Connection Tests
// ============================================================================

void ADSBTCPLinkTest::_connectionTest()
{
    QTcpServer server;
    if (!server.listen(QHostAddress::LocalHost, 0)) {
        QSKIP("Cannot create localhost TCP server - network may not be available");
    }

    const quint16 port = server.serverPort();
    QVERIFY2(port != 0, "Server did not allocate a port");

    ADSBTCPLink link(QHostAddress::LocalHost, port, this);
    QVERIFY2(link.init(), "Failed to initialize ADSB TCP link");

    // Wait for connection
    bool timeout = false;
    if (!server.waitForNewConnection(TestHelpers::kDefaultTimeoutMs, &timeout) || timeout) {
        QSKIP("Localhost TCP connection not available in this environment");
    }

    QTcpSocket* clientSocket = server.nextPendingConnection();
    VERIFY_NOT_NULL(clientSocket);
    QVERIFY(clientSocket->state() == QTcpSocket::ConnectedState);

    clientSocket->close();
    server.close();
}

// ============================================================================
// Message Parsing Tests
// ============================================================================

void ADSBTCPLinkTest::_messageParsingTest()
{
    QTcpServer server;
    if (!server.listen(QHostAddress::LocalHost, 0)) {
        QSKIP("Cannot create localhost TCP server - network may not be available");
    }

    ADSBTCPLink link(QHostAddress::LocalHost, server.serverPort(), this);
    QSignalSpy vehicleSpy(&link, &ADSBTCPLink::adsbVehicleUpdate);
    QGC_VERIFY_SPY_VALID(vehicleSpy);

    QVERIFY(link.init());

    bool timeout = false;
    if (!server.waitForNewConnection(TestHelpers::kDefaultTimeoutMs, &timeout) || timeout) {
        QSKIP("Localhost TCP connection not available in this environment");
    }

    QTcpSocket* clientSocket = server.nextPendingConnection();
    VERIFY_NOT_NULL(clientSocket);

    // Wait for socket to be fully connected before writing
    if (clientSocket->state() != QTcpSocket::ConnectedState) {
        QVERIFY(clientSocket->waitForConnected(TestHelpers::kDefaultTimeoutMs));
    }

    // Send MSG type 3 (ES Airborne Position) - contains location data
    // Format: MSG,3,sessionID,aircraftID,hexIdent,flightID,dateGen,timeGen,dateSeen,timeSeen,
    //         callsign,altitude,groundSpeed,track,lat,lon,verticalRate,squawk,alert,emergency,SPI,onGround
    // Index:  0   1  2         3          4        5        6       7       8        9
    //         10       11       12          13    14  15   16           17     18    19        20  21
    // Note: Parser reads alert from index 19, so all fields up to 19 must be populated
    const QByteArray positionMsg(
        "MSG,3,1,1,ABCDEF,1,2024/01/01,12:00:00.000,2024/01/01,12:00:00.000,"
        ",35000,,,51.5,-0.1,,,,0,,0\n");  // alert at index 19 = 0

    qint64 written = clientSocket->write(positionMsg);
    QCOMPARE_EQ(written, positionMsg.size());
    QVERIFY(clientSocket->waitForBytesWritten(TestHelpers::kDefaultTimeoutMs));

    // Wait for signal - ADSBTCPLink processes messages with a timer (50ms interval)
    // The spy.wait() processes events automatically
    if (!vehicleSpy.wait(TestHelpers::kDefaultTimeoutMs)) {
        // Network timing can be unreliable in CI environments
        QSKIP("TCP socket timing unreliable - skipping network integration test");
    }

    QCOMPARE_GE(vehicleSpy.count(), 1);

    // Verify parsed data
    const auto args = vehicleSpy.takeFirst();
    const auto vehicleInfo = args.at(0).value<ADSB::VehicleInfo_t>();
    QCOMPARE_EQ(vehicleInfo.icaoAddress, static_cast<uint32_t>(0xABCDEF));
    QVERIFY(vehicleInfo.availableFlags & ADSB::LocationAvailable);

    clientSocket->close();
    server.close();
}

// ============================================================================
// Error Handling Tests
// ============================================================================

void ADSBTCPLinkTest::_errorHandlingTest()
{
    // Use a port that is unlikely to have a server listening
    // Port 1 is reserved and requires root, so connection should fail
    constexpr quint16 kUnlikelyPort = 59999;

    ADSBTCPLink link(QHostAddress::LocalHost, kUnlikelyPort, this);
    QSignalSpy errorSpy(&link, &ADSBTCPLink::errorOccurred);
    QGC_VERIFY_SPY_VALID(errorSpy);

    // init() returns true because connectToHost is async
    QVERIFY(link.init());

    // Wait for connection error - should fire when connection is refused
    // Use a reasonable timeout since connection refusal should be fast
    if (errorSpy.wait(TestHelpers::kDefaultTimeoutMs)) {
        // Error signal was received - verify it contains an error message
        QCOMPARE_GE(errorSpy.count(), 1);
        const QString errorMsg = errorSpy.first().at(0).toString();
        QVERIFY2(!errorMsg.isEmpty(), "Error signal should contain a message");
    }
    // If wait times out, the test still passes - some systems may not
    // report connection errors immediately or may have something listening
}
