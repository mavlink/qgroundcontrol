#include "NTRIPHttpTransportTest.h"

#include <chrono>
#include <functional>
#include <memory>

#include <QtCore/QRegularExpression>
#include <QtNetwork/QAbstractSocket>
#include <QtNetwork/QTcpSocket>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "Fixtures/RAIIFixtures.h"
#include "ManualScheduler.h"
#include "MultiSignalSpy.h"
#include "NTRIP/Support/CasterSession.h"
#include "NTRIP/Support/NTRIPTestHelpers.h"
#include "NTRIP/Support/ScriptedNTRIPCaster.h"
#include "NTRIPError.h"
#include "NTRIPHttpCodec.h"
#include "NTRIPHttpSession.h"
#include "NTRIPHttpTransport.h"
#include "NTRIPManager.h"
#include "Protocols/Support/ProtocolTestPackets.h"
#include "RTCMFramer.h"
#include "Support/GPSTestHelpers.h"

using namespace GPSTest;

NTRIPHttpSession* NTRIPHttpTransportTest::_attachSession(NTRIPHttpTransport& transport)
{
    return transport._attachSession(new NTRIPHttpSession(&transport, transport._scheduler));
}

void NTRIPHttpTransportTest::_feed(NTRIPHttpTransport& transport, QByteArrayView bytes, qint64 receivedAtMs)
{
    NTRIPHttpSession* const session = transport._session ? transport._session.data() : _attachSession(transport);
    session->_receive(bytes, receivedAtMs);
}

namespace {
/// A write-only socket that admits as many bytes per write as @a admit returns.
class WriteSocket : public QTcpSocket
{
public:
    explicit WriteSocket(QObject* parent)
        : QTcpSocket(parent)
    {
        open(QIODevice::WriteOnly);
        setSocketState(QAbstractSocket::ConnectedState);
    }

    std::function<qint64(qint64)> admit;

protected:
    qint64 writeData(const char*, qint64 size) override { return admit ? admit(size) : size; }
};
}  // namespace

// ---------------------------------------------------------------------------
// RTCM framing and filtering
// ---------------------------------------------------------------------------

void NTRIPHttpTransportTest::_rtcmWhitelist_data()
{
    QTest::addColumn<QString>("whitelist");
    QTest::addColumn<QVector<int>>("messageIds");
    QTest::newRow("empty") << QString() << QVector<int>{};
    QTest::newRow("single") << QStringLiteral("1005") << QVector<int>{1005};
    QTest::newRow("multiple") << QStringLiteral("1005,1077,1087") << QVector<int>{1005, 1077, 1087};
    QTest::newRow("invalid-entries") << QStringLiteral("1005,abc,,1077") << QVector<int>{1005, 1077};
    QTest::newRow("non-positive-entries") << QStringLiteral("1005,invalid,0,-1") << QVector<int>{1005};
}

void NTRIPHttpTransportTest::_rtcmWhitelist()
{
    QFETCH(QString, whitelist);
    QFETCH(QVector<int>, messageIds);
    QCOMPARE(NTRIPRTCMFilterConfig{whitelist}.messageIds(), messageIds);
}

void NTRIPHttpTransportTest::_filterRejectsInvalidFrame_data()
{
    QTest::addColumn<QByteArray>("bad");
    // How the framer recovers from each kind of invalid frame is RTCMConformanceTest's.
    auto badCrc = GPSTest::rtcmMessage(1005, 4);
    badCrc.back() ^= 0xff;
    QTest::newRow("bad-crc") << badCrc;
}

void NTRIPHttpTransportTest::_filterRejectsInvalidFrame()
{
    QFETCH(QByteArray, bad);
    ManualScheduler scheduler;
    NTRIPHttpTransport t(connectionConfig(), {}, nullptr, &scheduler);

    QSignalSpy detailed(&t, &NTRIPTransport::correctionFrameReceived);

    const QByteArray good = GPSTest::rtcmMessage(1077, 2);
    expectLogMessage("GPS.NTRIPHttpTransport", QtWarningMsg, QRegularExpression(QStringLiteral("Invalid RTCM frame")));
    _feed(t, "HTTP/1.1 200 OK\r\n\r\n" + bad + good, 123);
    verifyExpectedLogMessage();

    // The invalid frame is dropped; the transport emits only the valid one after it.
    QCOMPARE(detailed.size(), 1);
    const auto recovered = frameAt(detailed, 0);
    QVERIFY(recovered.valid);
    QCOMPARE(recovered.data, good);
    QCOMPARE(recovered.receivedAtMs, 123);
}

void NTRIPHttpTransportTest::_filterConfigurationUpdatesWithoutReconnect()
{
    ScriptedNTRIPCaster caster;
    QVERIFY(caster.isListening());
    auto connection = caster.connectionConfig();
    NTRIPHttpTransport transport(connection, {1005});
    QSignalSpy connected(&transport, &NTRIPTransport::connected);
    QSignalSpy observed(&transport, &NTRIPTransport::correctionFrameReceived);
    transport.start();
    auto* casterConnection = caster.waitForConnection();
    QVERIFY(casterConnection && casterConnection->peer);
    QVERIFY(casterConnection->waitForRequest().startsWith("GET /TEST HTTP/1.1"));
    const auto frames = GPSTest::rtcmMessage(1005) + GPSTest::rtcmMessage(1077);
    const auto response = QByteArrayLiteral("HTTP/1.1 200 OK\r\n\r\n") + frames;
    QCOMPARE(casterConnection->write(response), response.size());
    // Only whitelisted messages are emitted.
    QTRY_COMPARE_WITH_TIMEOUT(observed.size(), 1, TestTimeout::mediumMs());
    QCOMPARE(frameAt(observed, 0).messageId, 1005);

    // Frames written to the original connection keep arriving, so the whitelist changes did not reconnect.
    transport.setRtcmWhitelist({1077});
    QCOMPARE(casterConnection->write(frames), frames.size());
    QTRY_COMPARE_WITH_TIMEOUT(observed.size(), 2, TestTimeout::mediumMs());
    QCOMPARE(frameAt(observed, 1).messageId, 1077);

    transport.setRtcmWhitelist({});
    QCOMPARE(casterConnection->write(frames), frames.size());
    QTRY_COMPARE_WITH_TIMEOUT(observed.size(), 4, TestTimeout::mediumMs());
    QCOMPARE(frameAt(observed, 2).messageId, 1005);
    QCOMPARE(frameAt(observed, 3).messageId, 1077);
    QCOMPARE(connected.size(), 1);
    QCOMPARE(casterConnection->peer->state(), QAbstractSocket::ConnectedState);
}

void NTRIPHttpTransportTest::_receiptTimesAndEvidence_data()
{
    QTest::addColumn<bool>("chunked");
    QTest::newRow("identity") << false;
    QTest::newRow("chunked") << true;
}

void NTRIPHttpTransportTest::_receiptTimesAndEvidence()
{
    QFETCH(bool, chunked);
    NTRIPHttpTransport transport(connectionConfig(), {});
    QSignalSpy observed(&transport, &NTRIPTransport::correctionFrameReceived);
    const auto first = GPSTest::rtcmMessage(1005);
    const auto second = GPSTest::rtcmMessage(1077);
    if (chunked) {
        _feed(transport, "HTTP/1.1 200 OK\r\nTransfer-Encoding: chu", 50);
        _feed(transport, "nked\r\n\r\n1\r\n" + first.first(1), 100);
        _feed(transport,
              "\r\n" + QByteArray::number(first.size() - 1 + second.size(), 16) + "\r\n" + first.sliced(1) + second +
                  "\r\n",
              200);
    } else {
        _feed(transport, "HTTP/1.1 200 OK\r\n\r\n" + first.first(1), 100);
        _feed(transport, first.sliced(1) + second, 200);
    }
    QCOMPARE(observed.size(), 2);
    QCOMPARE(frameAt(observed, 0).receivedAtMs, 100);
    QCOMPARE(frameAt(observed, 1).receivedAtMs, 200);
}

void NTRIPHttpTransportTest::_icyPartialFrame_data()
{
    QTest::addColumn<QByteArray>("suffix");
    QTest::newRow("nul-crc-tail") << QByteArray(1, '\0');
    QTest::newRow("printable-tail") << QByteArray("tail");
    QTest::newRow("colon-tail") << QByteArray("field: tail");
    QTest::newRow("carriage-return-tail") << QByteArray("\rX");
    QTest::newRow("line-feed-tail") << QByteArray("\n");
    QTest::newRow("terminated-tail") << QByteArray("tail\r\n");
    QTest::newRow("header-like-tail") << QByteArray("Tail: data\r\n");
    QTest::newRow("whitespace-tail") << QByteArray(" \t");
}

void NTRIPHttpTransportTest::_icyPartialFrame()
{
    QFETCH(QByteArray, suffix);
    const auto frame = QByteArray::fromHex("d300023ed0a4e000");
    for (const bool bytewise : {false, true}) {
        NTRIPHttpTransport transport(connectionConfig(), {});
        QSignalSpy frames(&transport, &NTRIPTransport::correctionFrameReceived);
        QSignalSpy errors(&transport, &NTRIPTransport::error);
        QSignalSpy connected(&transport, &NTRIPTransport::connected);
        const QByteArray prefix = "ICY 200 OK\r\n" + suffix;
        if (bytewise) {
            for (char ch : prefix) {
                _feed(transport, QByteArray(1, ch), 50);
            }
            _feed(transport, frame.first(1), 100);
            QCOMPARE(frames.size(), 0);
            for (char ch : frame.sliced(1)) {
                _feed(transport, QByteArray(1, ch), 200);
            }
        } else {
            _feed(transport, prefix + frame, 100);
        }
        QCOMPARE(connected.size(), 1);
        QCOMPARE(frames.size(), 1);
        QCOMPARE(frameAt(frames).receivedAtMs, 100);
        _feed(transport, frame.repeated(1024), 300);
        QCOMPARE(frames.size(), 1025);
        QCOMPARE(frameAt(frames, frames.size() - 1).receivedAtMs, 300);
        QVERIFY(errors.isEmpty());
    }
}

// ---------------------------------------------------------------------------
// Connection lifecycle
// ---------------------------------------------------------------------------

void NTRIPHttpTransportTest::_invalidConfigurationIsNotConnected()
{
    auto config = connectionConfig(2101, QStringLiteral("TEST"));
    config.host = QStringLiteral("caster\r\nInjected: value");
    config.username = QStringLiteral("user");
    config.password = QStringLiteral("pass");
    NTRIPHttpTransport transport(config, {});
    QSignalSpy errors(&transport, &NTRIPTransport::error);
    QSignalSpy connected(&transport, &NTRIPTransport::connected);
    transport.start();
    QCOMPARE(errors.size(), 1);
    QCOMPARE(failureAt(errors).code, NTRIPError::InvalidConfig);
    QVERIFY(connected.isEmpty());
    // No session, and so no socket, was opened.
    QVERIFY(!transport.findChild<NTRIPHttpSession*>());
}

void NTRIPHttpTransportTest::_connectionWaitsForHttpResponse()
{
    CasterSession session;
    NTRIPHttpTransport& transport = session.transport;
    const QSignalSpy& connected = session.signal("connected");
    const QSignalSpy& frames = session.signal("correctionFrameReceived");
    auto* connection = session.start();
    QVERIFY(connection);
    QVERIFY(connection->waitForRequest().startsWith("GET /TEST HTTP/1.1\r\n"));
    QVERIFY(connected.isEmpty());
    const QByteArray frame = GPSTest::rtcmMessage(1005, 4);
    const QByteArray response = "HTTP/1.1 200 OK\r\nContent-Type: gnss/data\r\n\r\n" + frame;
    QCOMPARE(connection->write(response), response.size());
    QTRY_COMPARE_WITH_TIMEOUT(connected.size(), 1, TestTimeout::mediumMs());
    QTRY_COMPARE_WITH_TIMEOUT(frames.size(), 1, TestTimeout::mediumMs());
    const auto result = frameAt(frames);
    QVERIFY(result.valid && !result.filtered);
    QCOMPARE(result.data, frame);
    // The response cancelled the connection timeout.
    QVERIFY(session.scheduler.advanceBy(NTRIPHttpTransport::CONNECT_TIMEOUT));
    QVERIFY_NO_SIGNAL(session.spy, "error");
    // The position sent to the caster is logged only by size.
    const TestFixtures::LoggingCategoryFixture logging(QStringLiteral("GPS.NTRIPHttpTransport"));
    const QByteArray expected = "$GPGGA,120000,4723.8620,N,00832.7360,E,1,12,1.0,100.0,M,0.0,M,,*77\r\n";
    const QString metadata = QStringLiteral("Queued NMEA bytes: %1").arg(expected.size());
    // An empty sentence is not written.
    transport.sendNMEA({});
    expectLogMessage("GPS.NTRIPHttpTransport", QtDebugMsg, exactMessage(metadata));
    transport.sendNMEA(expected);
    verifyExpectedLogMessage();
    QTRY_COMPARE_WITH_TIMEOUT(connection->peer->bytesAvailable(), expected.size(), TestTimeout::mediumMs());
    QCOMPARE(connection->peer->readAll(), expected);
    QCOMPARE(debugMessages(QStringLiteral("GPS.NTRIPHttpTransport")), QStringList{metadata});
    transport.stop();
}

void NTRIPHttpTransportTest::_legacyCaster_data()
{
    QTest::addColumn<bool>("authenticated");
    QTest::newRow("anonymous") << false;
    QTest::newRow("authenticated") << true;
}

void NTRIPHttpTransportTest::_legacyCaster()
{
    QFETCH(bool, authenticated);
    ScriptedNTRIPCaster caster;
    QVERIFY(caster.isListening());
    auto configuration = caster.connectionConfig();
    if (authenticated) {
        configuration.username = QStringLiteral("test-user");
        configuration.password = QStringLiteral("test-password");
        expectLogMessage("GPS.NTRIPHttpTransport", QtWarningMsg,
                         QRegularExpression(QStringLiteral("Sending credentials without TLS")));
    }
    NTRIPHttpTransport transport(configuration, {});
    MultiSignalSpy spy;
    QVERIFY(spy.init(&transport));
    transport.start();
    auto* connection = caster.waitForConnection();
    QVERIFY(connection && connection->peer);
    const QByteArray request = connection->waitForRequest();
    const bool accepted =
        request.startsWith("GET /TEST HTTP/1.1\r\n") && request.contains("\r\nUser-Agent: NTRIP ") &&
        (!authenticated ||
         request.contains("\r\nAuthorization: Basic " + QByteArray("test-user:test-password").toBase64() + "\r\n"));
    const auto frame = GPSTest::rtcmMessage(1005);
    if (accepted) {
        const QByteArray response = "ICY 200 OK\r\n" + frame;
        QCOMPARE(connection->write(response), response.size());
    } else {
        connection->disconnectFromHost();
    }
    QTRY_VERIFY_WITH_TIMEOUT(spy.emitted("correctionFrameReceived") || spy.emitted("error"), TestTimeout::mediumMs());
    QVERIFY_NO_SIGNAL(spy, "error");
    QCOMPARE(spy.count("connected"), 1);
    QCOMPARE(spy.count("correctionFrameReceived"), 1);
    QCOMPARE(spy.argument<RTCMDecodedFrame>("correctionFrameReceived").data, frame);
    transport.stop();
    if (authenticated) {
        verifyExpectedLogMessage();
    }
}

void NTRIPHttpTransportTest::_writeAdmissionFailure_data()
{
    QTest::addColumn<int>("accepted");
    QTest::addColumn<bool>("nmea");
    for (bool nmea : {false, true}) {
        QTest::newRow(nmea ? "nmea-failed" : "request-failed") << -1 << nmea;
        QTest::newRow(nmea ? "nmea-partial" : "request-partial") << 1 << nmea;
    }
}

void NTRIPHttpTransportTest::_writeAdmissionFailure()
{
    QFETCH(int, accepted);
    QFETCH(bool, nmea);
    NTRIPHttpTransport transport(connectionConfig(), {});
    auto* socket = new WriteSocket(&transport);
    socket->admit = [accepted](qint64) { return accepted; };
    NTRIPHttpSession* const session = _attachSession(transport);
    session->_attach(socket);
    QSignalSpy errors(&transport, &NTRIPTransport::error);
    if (nmea) {
        transport.sendNMEA(QByteArrayLiteral("$GPGGA"));
    } else {
        session->_request = NTRIPHttpRequest::build(connectionConfig()).bytes;
        session->_establish();
    }
    QCOMPARE(errors.size(), 1);
    QCOMPARE(failureAt(errors).code, NTRIPError::SocketError);
}

void NTRIPHttpTransportTest::_handshakeTimeoutClosesSocket()
{
    CasterSession session;
    auto* connection = session.start();
    QVERIFY(connection);
    QVERIFY(connection->waitForRequest().startsWith("GET /TEST HTTP/1.1\r\n"));
    QVERIFY_NO_SIGNAL(session.spy, "connected");
    expectLogMessage("GPS.NTRIPHttpTransport", QtWarningMsg, QRegularExpression(QStringLiteral("Connection timeout")));
    QVERIFY(session.scheduler.advanceBy(NTRIPHttpTransport::CONNECT_TIMEOUT));
    QCOMPARE(session.spy.count("error"), 1);
    QCOMPARE(failureAt(session.signal("error")).code, NTRIPError::ConnectionTimeout);
    QTRY_COMPARE_WITH_TIMEOUT(connection->peer->state(), QAbstractSocket::UnconnectedState, TestTimeout::mediumMs());
    QCOMPARE(session.scheduler.pendingCount(), 0);
    QVERIFY_NO_SIGNAL(session.spy, "connected");
    verifyExpectedLogMessage();
}

void NTRIPHttpTransportTest::_remoteCloseEmitsSingleError()
{
    CasterSession session;
    const QSignalSpy& errors = session.signal("error");
    auto* connection = session.start();
    QVERIFY(connection);
    connection->disconnectFromHost();
    QTRY_COMPARE_WITH_TIMEOUT(errors.size(), 1, TestTimeout::mediumMs());
    deliverQueuedCalls();
    QCOMPARE(errors.size(), 1);
    const auto failure = failureAt(errors);
    QCOMPARE(failure.code, NTRIPError::InvalidHttpResponse);
    QCOMPARE(failure.detail, QCoreApplication::translate("NTRIPHttpTransport",
                                                         "Caster disconnected before completing the HTTP response"));
    QCOMPARE(session.scheduler.pendingCount(), 0);
}

void NTRIPHttpTransportTest::_socketTermination_data()
{
    QTest::addColumn<QByteArray>("wire");
    QTest::addColumn<bool>("truncated");
    const QByteArray body = GPSTest::rtcmMessage(1005).repeated(4000);
    QTest::newRow("close-delimited") << "HTTP/1.1 200 OK\r\n\r\n" + body << false;
    QTest::newRow("length-complete") << okResponse(body) << false;
    QTest::newRow("length-truncated") << "HTTP/1.1 200 OK\r\nContent-Length: " + QByteArray::number(body.size() + 1) +
                                             "\r\n\r\n" + body
                                      << true;
    QTest::newRow("chunked-complete") << okChunkedResponse(body) << false;
    QTest::newRow("chunked-truncated") << okChunkedResponse(body, false) << true;
}

void NTRIPHttpTransportTest::_socketTermination()
{
    QFETCH(QByteArray, wire);
    QFETCH(bool, truncated);
    CasterSession session;
    const QSignalSpy& errors = session.signal("error");
    auto* connection = session.start();
    QVERIFY(connection);
    QVERIFY(connection->waitForRequest().startsWith("GET /TEST HTTP/1.1"));
    QCOMPARE(connection->write(wire), wire.size());
    connection->disconnectFromHost();
    QTRY_COMPARE_WITH_TIMEOUT(errors.size(), 1, TestTimeout::mediumMs());
    QCOMPARE(session.spy.count("correctionFrameReceived"), 4000);
    const auto failure = failureAt(errors);
    QCOMPARE(failure.code, truncated ? NTRIPError::InvalidHttpResponse : NTRIPError::ServerDisconnected);
    if (truncated) {
        // Truncation after the stream started reads as a mid-transfer close, not a missing HTTP response.
        QCOMPARE(failure.detail,
                 QCoreApplication::translate("NTRIPHttpTransport", "Caster closed the connection mid-transfer"));
    }
    QCOMPARE(session.scheduler.pendingCount(), 0);
    deliverQueuedCalls();
    QCOMPARE(errors.size(), 1);
}

void NTRIPHttpTransportTest::_errorBodyEnds_data()
{
    QTest::addColumn<bool>("socketFailure");
    QTest::newRow("deadline") << false;
    QTest::newRow("socket-failure") << true;
}

void NTRIPHttpTransportTest::_errorBodyEnds()
{
    QFETCH(bool, socketFailure);
    ManualScheduler scheduler;
    NTRIPHttpTransport transport(connectionConfig(), {}, nullptr, &scheduler);
    QSignalSpy errors(&transport, &NTRIPTransport::error);
    QSignalSpy frames(&transport, &NTRIPTransport::correctionFrameReceived);
    _feed(transport, "HTTP/1.1 503 Unavailable\r\nRetry-After: 120\r\nContent-Length: 1000000\r\n\r\nPartial detail",
          123);
    QVERIFY(errors.isEmpty());
    if (socketFailure) {
        transport._session->_fail(NTRIPError::SocketError, QStringLiteral("Connection reset"));
    } else {
        _feed(transport, "x", 123);
        QVERIFY(scheduler.advanceBy(NTRIPHttpSession::ERROR_BODY_TIMEOUT));
    }
    // The caster's error, with what arrived of its body, is reported either way.
    QCOMPARE(errors.size(), 1);
    const auto failure = failureAt(errors);
    QCOMPARE(failure.code, NTRIPError::HttpError);
    QCOMPARE(failure.retryAfter, std::chrono::seconds(120));
    QVERIFY(failure.detail.contains(QStringLiteral("Partial detail")));
    QVERIFY(frames.isEmpty());
    QCOMPARE(scheduler.pendingCount(), 0);
}

void NTRIPHttpTransportTest::_validFrameWatchdog_data()
{
    QTest::addColumn<bool>("filtered");
    QTest::newRow("garbage-does-not-refresh") << false;
    QTest::newRow("valid-filtered-frames-refresh") << true;
}

void NTRIPHttpTransportTest::_validFrameWatchdog()
{
    QFETCH(bool, filtered);
    ManualScheduler scheduler;
    NTRIPHttpTransport transport(connectionConfig(), QVector<int>{1077}, nullptr, &scheduler);
    QSignalSpy errors(&transport, &NTRIPTransport::error);
    _feed(transport, "HTTP/1.1 200 OK\r\n\r\n", 123);
    const auto data = filtered ? GPSTest::rtcmMessage(1005) : QByteArray("garbage");
    if (filtered) {
        for (int i = 0; i < 3; ++i) {
            QVERIFY(scheduler.advanceBy(NTRIPHttpTransport::DATA_WATCHDOG / 2));
            _feed(transport, data, 123);
        }
        QVERIFY(scheduler.advanceBy(NTRIPHttpTransport::DATA_WATCHDOG / 2));
        _feed(transport, "garbage", 123);
        QVERIFY(errors.isEmpty());
    } else {
        QVERIFY(scheduler.advanceBy(NTRIPHttpTransport::DATA_WATCHDOG / 2));
        _feed(transport, data, 123);
    }
    QVERIFY(scheduler.advanceBy(NTRIPHttpTransport::DATA_WATCHDOG / 2));
    QCOMPARE(errors.size(), 1);
    const auto failure = failureAt(errors);
    QCOMPARE(failure.code, NTRIPError::DataWatchdog);
    QVERIFY(failure.detail.contains(QStringLiteral("valid RTCM")));
    QCOMPARE(scheduler.pendingCount(), 0);
}

void NTRIPHttpTransportTest::_silentStreamWatchdog()
{
    CasterSession session;
    const QSignalSpy& errors = session.signal("error");
    auto* connection = session.start();
    QVERIFY(connection);
    QVERIFY(connection->waitForRequest().endsWith("\r\n\r\n"));
    const QByteArray response = "HTTP/1.1 200 OK\r\n\r\n" + GPSTest::rtcmMessage(1005);
    QCOMPARE(connection->write(response), response.size());
    QTRY_COMPARE_WITH_TIMEOUT(session.spy.count("correctionFrameReceived"), 1, TestTimeout::mediumMs());
    QVERIFY(session.scheduler.advanceBy(NTRIPHttpTransport::DATA_WATCHDOG - std::chrono::milliseconds(1)));
    QVERIFY(errors.isEmpty());
    expectLogMessage("GPS.NTRIPHttpTransport", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^No data received for")));
    QVERIFY(session.scheduler.advanceBy(std::chrono::milliseconds(1)));
    verifyExpectedLogMessage();
    QCOMPARE(errors.size(), 1);
    const auto failure = failureAt(errors);
    QCOMPARE(failure.code, NTRIPError::DataWatchdog);
    QVERIFY(failure.detail.contains(QStringLiteral("No data received")));
    QCOMPARE(session.scheduler.pendingCount(), 0);
}

// ---------------------------------------------------------------------------
// Observers that stop or restart the transport
// ---------------------------------------------------------------------------

void NTRIPHttpTransportTest::_handshakeRetiresAttempt_data()
{
    QTest::addColumn<bool>("icy");
    QTest::addColumn<bool>("chunked");
    QTest::addColumn<bool>("restart");
    for (bool icy : {false, true}) {
        QTest::newRow(icy ? "icy-stop" : "http-stop") << icy << false << false;
        QTest::newRow(icy ? "icy-restart" : "http-restart") << icy << false << true;
    }
    QTest::newRow("chunked-stop") << false << true << false;
    QTest::newRow("chunked-restart") << false << true << true;
}

void NTRIPHttpTransportTest::_handshakeRetiresAttempt()
{
    QFETCH(bool, icy);
    QFETCH(bool, chunked);
    QFETCH(bool, restart);
    ManualScheduler scheduler;
    NTRIPHttpTransport transport(connectionConfig(), QVector<int>{}, nullptr, &scheduler);
    auto* socket = new QTcpSocket(&transport);
    socket->open(QIODevice::ReadOnly);
    _attachSession(transport)->_attach(socket);
    const auto frame = GPSTest::rtcmMessage(1005);
    const QByteArray response =
        chunked ? okChunkedResponse(frame)
                : (icy ? QByteArrayLiteral("ICY 200 OK\r\n") : QByteArrayLiteral("HTTP/1.1 200 OK\r\n\r\n")) + frame;
    int frames = 0;
    connect(&transport, &NTRIPTransport::correctionFrameReceived, this, [&]() { ++frames; });
    connect(&transport, &NTRIPTransport::connected, this, [&]() {
        transport.stop();
        if (restart) {
            transport.start();
        }
    });
    _feed(transport, response, 123);
    QCOMPARE(frames, 0);
    // The retired attempt armed no data watchdog; only a restarted attempt's connection timeout is pending.
    QCOMPARE(scheduler.pendingCount(), restart ? 1 : 0);
}

void NTRIPHttpTransportTest::_failureCanRestart_data()
{
    QTest::addColumn<bool>("restart");
    QTest::addColumn<bool>("deferred");
    for (bool restart : {false, true}) {
        const char* action = restart ? "restart" : "stop";
        QTest::addRow("immediate-%s", action) << restart << false;
        QTest::addRow("deferred-%s", action) << restart << true;
    }
}

void NTRIPHttpTransportTest::_failureCanRestart()
{
    QFETCH(bool, restart);
    QFETCH(bool, deferred);
    ScriptedNTRIPCaster caster;
    QVERIFY(caster.isListening());
    int failures = 0;
    ManualScheduler scheduler;
    NTRIPHttpTransport transport(caster.connectionConfig(), QVector<int>{}, nullptr, &scheduler);
    _attachSession(transport)->_attach(new QTcpSocket(&transport));
    QSignalSpy errors(&transport, &NTRIPTransport::error);
    connect(&transport, &NTRIPTransport::error, this, [&]() {
        ++failures;
        if (restart) {
            transport.start();
        } else {
            transport.stop();
        }
    });
    _feed(transport,
          "HTTP/1.1 503 Unavailable\r\nRetry-After: 17\r\nContent-Length: " + QByteArray(deferred ? "100" : "0") +
              "\r\n\r\n",
          123);
    if (deferred) {
        QCOMPARE(failures, 0);
        QVERIFY(scheduler.advanceBy(NTRIPHttpSession::ERROR_BODY_TIMEOUT));
    }
    QTRY_COMPARE_WITH_TIMEOUT(failures, 1, TestTimeout::mediumMs());
    QCOMPARE(failureAt(errors).code, NTRIPError::HttpError);
    if (!restart) {
        QCOMPARE(scheduler.pendingCount(), 0);
        return;
    }
    // The restarted attempt runs: its connection times out on its own deadline.
    QVERIFY(caster.waitForConnection());
    expectLogMessage("GPS.NTRIPHttpTransport", QtWarningMsg, QRegularExpression(QStringLiteral("Connection timeout")));
    QVERIFY(scheduler.advanceBy(NTRIPHttpTransport::CONNECT_TIMEOUT));
    verifyExpectedLogMessage();
    QCOMPARE(failures, 2);
    QCOMPARE(failureAt(errors, 1).code, NTRIPError::ConnectionTimeout);
}

void NTRIPHttpTransportTest::_pendingErrorRetiresAttempt_data()
{
    QTest::addColumn<int>("action");
    QTest::newRow("stop") << 0;
    QTest::newRow("delete") << 1;
    QTest::newRow("restart") << 2;
}

void NTRIPHttpTransportTest::_pendingErrorRetiresAttempt()
{
    QFETCH(int, action);
    ScriptedNTRIPCaster caster;
    QVERIFY(caster.isListening());
    ManualScheduler scheduler;
    auto transport =
        std::make_unique<NTRIPHttpTransport>(caster.connectionConfig(), QVector<int>{}, nullptr, &scheduler);
    QSignalSpy errors(transport.get(), &NTRIPTransport::error);
    _feed(*transport, "HTTP/1.1 503 Unavailable\r\nContent-Length: 100\r\n\r\n", 123);
    // The error waits for its body.
    QCOMPARE(scheduler.pendingCount(), 1);
    if (action == 1) {
        transport.reset();
    } else if (action == 0) {
        transport->stop();
    } else {
        transport->start();
        _feed(*transport, "HTTP/1.1 200 OK\r\n\r\n", 456);
    }
    // Only a restarted, connected attempt's data watchdog remains.
    QCOMPARE(scheduler.pendingCount(), action == 2 ? 1 : 0);
    QVERIFY(scheduler.advanceBy(NTRIPHttpSession::ERROR_BODY_TIMEOUT + std::chrono::milliseconds{50}));
    QVERIFY(errors.isEmpty());
}

void NTRIPHttpTransportTest::_bodyPublicationRetiresAttempt_data()
{
    QTest::addColumn<bool>("chunked");
    QTest::addColumn<bool>("restart");
    for (bool chunked : {false, true}) {
        QTest::newRow(chunked ? "chunked-stop" : "identity-stop") << chunked << false;
        QTest::newRow(chunked ? "chunked-restart" : "identity-restart") << chunked << true;
    }
}

void NTRIPHttpTransportTest::_bodyPublicationRetiresAttempt()
{
    QFETCH(bool, chunked);
    QFETCH(bool, restart);
    ManualScheduler scheduler;
    NTRIPHttpTransport transport(connectionConfig(), QVector<int>{}, nullptr, &scheduler);
    const QByteArray frame = GPSTest::rtcmMessage(1005);
    int frames = 0;
    int errors = 0;
    connect(&transport, &NTRIPTransport::error, this, [&]() { ++errors; });
    connect(&transport, &NTRIPTransport::correctionFrameReceived, this, [&](const RTCMDecodedFrame& result) {
        QVERIFY(result.valid && !result.filtered);
        QCOMPARE(result.receivedAtMs, 123);
        ++frames;
        if (restart) {
            transport.start();
        } else {
            transport.stop();
        }
    });
    const QByteArray wire = chunked ? "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n" +
                                          QByteArray::number(frame.size() * 2, 16) + "\r\n" + frame + frame + "!\r\n"
                                    : "HTTP/1.1 200 OK\r\n\r\n" + frame + frame;
    _feed(transport, wire, 123);
    QCOMPARE(frames, 1);
    QCOMPARE(errors, 0);
    // A stopped transport has nothing pending; a restarted one only its new connection timeout.
    QCOMPARE(scheduler.pendingCount(), restart ? 1 : 0);
}

UT_REGISTER_TEST_LIGHTWEIGHT(NTRIPHttpTransportTest, TestLabel::Unit)
