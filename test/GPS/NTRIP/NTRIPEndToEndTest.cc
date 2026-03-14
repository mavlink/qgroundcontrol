#include "NTRIPEndToEndTest.h"
#include "RTCMTestHelper.h"
#include "NTRIPHttpTransport.h"
#include "NTRIPTransportConfig.h"
#include "NTRIPError.h"
#include "RTCMMavlink.h"
#include "RTCMRouter.h"

#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include <mavlink_types.h>
#include <common/mavlink_msg_gps_rtcm_data.h>

namespace {

// Minimal NTRIP caster that listens on localhost, accepts one connection,
// responds with an HTTP 200, and then sends whatever bytes are queued.
class LocalCaster : public QObject
{
    Q_OBJECT
public:
    explicit LocalCaster(QObject *parent = nullptr) : QObject(parent)
    {
        connect(&_server, &QTcpServer::newConnection, this, [this]() {
            _client = _server.nextPendingConnection();
            connect(_client, &QTcpSocket::readyRead, this, [this]() {
                _receivedData.append(_client->readAll());
                if (_receivedData.contains("\r\n\r\n") && !_handshakeSent) {
                    _handshakeSent = true;
                    if (_rejectAuth) {
                        _client->write("HTTP/1.1 401 Unauthorized\r\n\r\n");
                        _client->flush();
                        return;
                    }
                    _client->write("HTTP/1.1 200 OK\r\nContent-Type: gnss/data\r\n\r\n");
                    _client->flush();
                    emit ready();
                    if (!_pendingData.isEmpty()) {
                        _client->write(_pendingData);
                        _client->flush();
                        _pendingData.clear();
                    }
                }
            });
        });
    }

    bool listen() { return _server.listen(QHostAddress::LocalHost, 0); }
    quint16 port() const { return _server.serverPort(); }

    void sendRtcm(const QByteArray &data)
    {
        if (_client && _handshakeSent) {
            _client->write(data);
            _client->flush();
        } else {
            _pendingData.append(data);
        }
    }

    void setRejectAuth(bool reject) { _rejectAuth = reject; }
    QByteArray receivedData() const { return _receivedData; }

signals:
    void ready();

private:
    QTcpServer _server;
    QTcpSocket *_client = nullptr;
    QByteArray _pendingData;
    QByteArray _receivedData;
    bool _handshakeSent = false;
    bool _rejectAuth = false;
};

} // namespace

// ---------------------------------------------------------------------------
// Test: Single RTCM frame flows through entire pipeline
// LocalCaster -> NTRIPHttpTransport -> RTCMParser -> (RTCMDataUpdate) ->
//   RTCMRouter -> RTCMMavlink -> captured mavlink_gps_rtcm_data_t
// ---------------------------------------------------------------------------

void NTRIPEndToEndTest::testRtcmThroughFullPipeline()
{
    // 1. Set up local caster
    LocalCaster caster;
    QVERIFY(caster.listen());

    // 2. Set up MAVLink capture
    RTCMMavlink mavlink;
    QList<mavlink_gps_rtcm_data_t> mavlinkMsgs;
    mavlink.setMessageSender([&](const mavlink_gps_rtcm_data_t &msg) {
        mavlinkMsgs.append(msg);
    });

    RTCMRouter router(&mavlink);

    // 3. Build a valid RTCM frame (message type 1005, 10 bytes payload body)
    const QByteArray rtcmFrame = RTCMTestHelper::buildFrame(1005, QByteArray(10, '\x42'));

    // 4. Create transport and wire it to the router
    NTRIPTransportConfig config;
    config.host = QStringLiteral("127.0.0.1");
    config.port = caster.port();
    config.mountpoint = QStringLiteral("TEST");

    NTRIPHttpTransport transport(config);
    connect(&transport, &NTRIPTransport::RTCMDataUpdate, &router, &RTCMRouter::routeToVehicles);

    QSignalSpy connectedSpy(&transport, &NTRIPTransport::connected);

    // 5. Start the transport — it connects to our local caster
    transport.start();

    // Wait for HTTP handshake
    QVERIFY(connectedSpy.wait(5000));

    // 6. Send RTCM data from caster
    caster.sendRtcm(rtcmFrame);

    // Wait for MAVLink output
    QTRY_VERIFY_WITH_TIMEOUT(!mavlinkMsgs.isEmpty(), 3000);

    // 7. Verify the MAVLink message contains our RTCM frame
    QCOMPARE(mavlinkMsgs.size(), 1);
    const auto &msg = mavlinkMsgs.first();

    // Frame = header(3) + payload(12) + crc(3) = 18 bytes, fits in one MAVLink message
    QCOMPARE(msg.len, rtcmFrame.size());

    QByteArray received(reinterpret_cast<const char *>(msg.data), msg.len);
    QCOMPARE(received, rtcmFrame);

    // Verify sequence id is encoded in flags
    QCOMPARE((msg.flags >> 3) & 0x1F, 0); // first message, seq=0

    QCOMPARE(mavlink.totalBytesSent(), static_cast<quint64>(rtcmFrame.size()));

    transport.stop();
}

// ---------------------------------------------------------------------------
// Test: Multiple RTCM frames in one TCP write are all parsed and forwarded
// ---------------------------------------------------------------------------

void NTRIPEndToEndTest::testMultipleRtcmFramesBatched()
{
    LocalCaster caster;
    QVERIFY(caster.listen());

    RTCMMavlink mavlink;
    QList<mavlink_gps_rtcm_data_t> mavlinkMsgs;
    mavlink.setMessageSender([&](const mavlink_gps_rtcm_data_t &msg) {
        mavlinkMsgs.append(msg);
    });

    RTCMRouter router(&mavlink);

    // Build 3 different RTCM frames
    const QByteArray frame1 = RTCMTestHelper::buildFrame(1005, QByteArray(6, '\x01'));
    const QByteArray frame2 = RTCMTestHelper::buildFrame(1077, QByteArray(8, '\x02'));
    const QByteArray frame3 = RTCMTestHelper::buildFrame(1087, QByteArray(4, '\x03'));

    NTRIPTransportConfig config;
    config.host = QStringLiteral("127.0.0.1");
    config.port = caster.port();
    config.mountpoint = QStringLiteral("TEST");

    NTRIPHttpTransport transport(config);
    connect(&transport, &NTRIPTransport::RTCMDataUpdate, &router, &RTCMRouter::routeToVehicles);

    QSignalSpy connectedSpy(&transport, &NTRIPTransport::connected);
    transport.start();
    QVERIFY(connectedSpy.wait(5000));

    // Send all three frames in a single TCP write
    QByteArray batch;
    batch.append(frame1);
    batch.append(frame2);
    batch.append(frame3);
    caster.sendRtcm(batch);

    QTRY_VERIFY_WITH_TIMEOUT(mavlinkMsgs.size() >= 3, 3000);

    QCOMPARE(mavlinkMsgs.size(), 3);
    QCOMPARE(QByteArray(reinterpret_cast<const char *>(mavlinkMsgs[0].data), mavlinkMsgs[0].len), frame1);
    QCOMPARE(QByteArray(reinterpret_cast<const char *>(mavlinkMsgs[1].data), mavlinkMsgs[1].len), frame2);
    QCOMPARE(QByteArray(reinterpret_cast<const char *>(mavlinkMsgs[2].data), mavlinkMsgs[2].len), frame3);

    transport.stop();
}

// ---------------------------------------------------------------------------
// Test: Large RTCM frame gets fragmented into multiple MAVLink messages
// ---------------------------------------------------------------------------

void NTRIPEndToEndTest::testLargeRtcmFragmented()
{
    LocalCaster caster;
    QVERIFY(caster.listen());

    RTCMMavlink mavlink;
    QList<mavlink_gps_rtcm_data_t> mavlinkMsgs;
    mavlink.setMessageSender([&](const mavlink_gps_rtcm_data_t &msg) {
        mavlinkMsgs.append(msg);
    });

    RTCMRouter router(&mavlink);

    // Build a large RTCM frame (body = 300 bytes -> total frame > 180 = MAVLINK_MSG_GPS_RTCM_DATA_FIELD_DATA_LEN)
    const QByteArray largeBody(300, '\xAA');
    const QByteArray rtcmFrame = RTCMTestHelper::buildFrame(1077, largeBody);

    NTRIPTransportConfig config;
    config.host = QStringLiteral("127.0.0.1");
    config.port = caster.port();
    config.mountpoint = QStringLiteral("TEST");

    NTRIPHttpTransport transport(config);
    connect(&transport, &NTRIPTransport::RTCMDataUpdate, &router, &RTCMRouter::routeToVehicles);

    QSignalSpy connectedSpy(&transport, &NTRIPTransport::connected);
    transport.start();
    QVERIFY(connectedSpy.wait(5000));

    caster.sendRtcm(rtcmFrame);

    // Frame is header(3) + payload(302) + crc(3) = 308 bytes
    // MAVLINK_MSG_GPS_RTCM_DATA_FIELD_DATA_LEN = 180
    // So we expect ceil(308/180) = 2 fragments
    QTRY_VERIFY_WITH_TIMEOUT(mavlinkMsgs.size() >= 2, 3000);

    // All fragments should have the fragmented bit set
    for (const auto &msg : mavlinkMsgs) {
        QVERIFY(msg.flags & 0x01); // fragmented flag
    }

    // Reassemble and verify
    QByteArray reassembled;
    for (const auto &msg : mavlinkMsgs) {
        reassembled.append(reinterpret_cast<const char *>(msg.data), msg.len);
    }
    QCOMPARE(reassembled, rtcmFrame);

    // Verify fragment ids are sequential
    for (int i = 0; i < mavlinkMsgs.size(); ++i) {
        const uint8_t fragmentId = (mavlinkMsgs[i].flags >> 1) & 0x03;
        QCOMPARE(fragmentId, static_cast<uint8_t>(i));
    }

    QCOMPARE(mavlink.totalBytesSent(), static_cast<quint64>(rtcmFrame.size()));
    transport.stop();
}

// ---------------------------------------------------------------------------
// Test: Auth failure propagates from caster through transport as error
// ---------------------------------------------------------------------------

void NTRIPEndToEndTest::testAuthFailureEndToEnd()
{
    LocalCaster caster;
    caster.setRejectAuth(true);
    QVERIFY(caster.listen());

    NTRIPTransportConfig config;
    config.host = QStringLiteral("127.0.0.1");
    config.port = caster.port();
    config.mountpoint = QStringLiteral("SECURE");
    config.username = QStringLiteral("user");
    config.password = QStringLiteral("wrong");

    NTRIPHttpTransport transport(config);

    QSignalSpy errorSpy(&transport, &NTRIPTransport::error);
    transport.start();

    QVERIFY(errorSpy.wait(5000));
    QCOMPARE(errorSpy.size(), 1);
    QCOMPARE(errorSpy.at(0).at(0).value<NTRIPError>(), NTRIPError::AuthFailed);
    QVERIFY(errorSpy.at(0).at(1).toString().contains(QStringLiteral("401")));

    transport.stop();
}

// ---------------------------------------------------------------------------
// Test: NMEA GGA sent by transport reaches the caster
// ---------------------------------------------------------------------------

void NTRIPEndToEndTest::testNmeaGgaSentToCaster()
{
    LocalCaster caster;
    QVERIFY(caster.listen());

    NTRIPTransportConfig config;
    config.host = QStringLiteral("127.0.0.1");
    config.port = caster.port();
    config.mountpoint = QStringLiteral("TEST");

    NTRIPHttpTransport transport(config);

    QSignalSpy connectedSpy(&transport, &NTRIPTransport::connected);
    transport.start();
    QVERIFY(connectedSpy.wait(5000));

    const QByteArray gga = QByteArrayLiteral("$GPGGA,123456.00,4740.0000,N,00830.0000,E,1,12,1.0,400.0,M,0.0,M,,*XX\r\n");
    transport.sendNMEA(gga);

    // Give time for the write to reach the caster
    QTest::qWait(200);

    // The caster should have received the HTTP request + the GGA sentence
    QVERIFY(caster.receivedData().contains("$GPGGA"));

    transport.stop();
}

// ---------------------------------------------------------------------------
// Test: Whitelist filters out non-matching RTCM message types
// ---------------------------------------------------------------------------

void NTRIPEndToEndTest::testWhitelistFiltersMessages()
{
    LocalCaster caster;
    QVERIFY(caster.listen());

    RTCMMavlink mavlink;
    QList<mavlink_gps_rtcm_data_t> mavlinkMsgs;
    mavlink.setMessageSender([&](const mavlink_gps_rtcm_data_t &msg) {
        mavlinkMsgs.append(msg);
    });
    RTCMRouter router(&mavlink);

    // Only allow message type 1005
    NTRIPTransportConfig config;
    config.host = QStringLiteral("127.0.0.1");
    config.port = caster.port();
    config.mountpoint = QStringLiteral("TEST");
    config.whitelist = QStringLiteral("1005");

    NTRIPHttpTransport transport(config);
    connect(&transport, &NTRIPTransport::RTCMDataUpdate, &router, &RTCMRouter::routeToVehicles);

    QSignalSpy connectedSpy(&transport, &NTRIPTransport::connected);
    transport.start();
    QVERIFY(connectedSpy.wait(5000));

    // Send one allowed (1005) and one filtered (1077)
    const QByteArray allowed = RTCMTestHelper::buildFrame(1005, QByteArray(6, '\x01'));
    const QByteArray filtered = RTCMTestHelper::buildFrame(1077, QByteArray(8, '\x02'));

    QByteArray batch;
    batch.append(allowed);
    batch.append(filtered);
    caster.sendRtcm(batch);

    // Wait for processing
    QTest::qWait(500);

    // Only the whitelisted message should have arrived
    QCOMPARE(mavlinkMsgs.size(), 1);
    QCOMPARE(QByteArray(reinterpret_cast<const char *>(mavlinkMsgs[0].data), mavlinkMsgs[0].len), allowed);

    transport.stop();
}

#include "NTRIPEndToEndTest.moc"

UT_REGISTER_TEST(NTRIPEndToEndTest, TestLabel::Unit)
