#include <algorithm>
#include <atomic>
#include <functional>
#include <memory>

#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QTcpServer>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "../../RTCM/RTCMTestFixtures.h"
#include "../MockNTRIPTransport.h"
#include "NTRIPGgaProvider.h"
#include "NTRIPHttpTransport.h"
#include "NTRIPSourceTableController.h"
#include "PortableTest.h"

namespace {
class WriteSocket : public QTcpSocket
{
public:
    explicit WriteSocket(QObject* parent)
        : QTcpSocket(parent)
    {
        open(QIODevice::WriteOnly);
    }

    std::function<qint64(qint64)> admit;

protected:
    qint64 writeData(const char*, qint64 size) override { return admit(size); }
};

NTRIPConnectionConfig config()
{
    NTRIPConnectionConfig result;
    result.host = QStringLiteral("127.0.0.1");
    result.mountpoint = QStringLiteral("TEST");
    return result;
}
}  // namespace

class NTRIPReentrancyTest : public PortableTest
{
    Q_OBJECT

private slots:
    void warningRetiresAttempt_data();
    void warningRetiresAttempt();
    void writeAdmissionFailure_data();
    void writeAdmissionFailure();
    void handshakeRetiresAttempt_data();
    void handshakeRetiresAttempt();
    void failureCanRestart_data();
    void failureCanRestart();
    void httpFraming_data();
    void httpFraming();
    void retryAfter_data();
    void retryAfter();
    void bodyPublicationRetiresAttempt_data();
    void bodyPublicationRetiresAttempt();
    void chunkedReceiptTimesAndEvidence();
    void socketTermination_data();
    void socketTermination();
    void filterConfigurationUpdatesWithoutReconnect();
    void invalidFetchRetiresPendingReply();
    void sourceTableSuccessAndCache();
    void abortCallbackSupersedesReplacement();
    void abortCallbackDeletesReply();
    void deletedReplyPublishesError();
    void fetchNotificationReentry_data();
    void fetchNotificationReentry();
    void ggaSourceSelection();
    void ggaCallbackStopsProvider();
};

void NTRIPReentrancyTest::warningRetiresAttempt_data()
{
    QTest::addColumn<int>("action");
    QTest::newRow("stop") << 0;
    QTest::newRow("delete") << 1;
    QTest::newRow("restart") << 2;
}

void NTRIPReentrancyTest::warningRetiresAttempt()
{
    QFETCH(int, action);
    auto configuration = config();
    configuration.username = QStringLiteral("test-user");
    auto transport = std::make_unique<NTRIPHttpTransport>(configuration, NTRIPRtcmFilterConfig{});
    auto* socket = new WriteSocket(transport.get());
    int writes = 0;
    socket->admit = [&](qint64 size) {
        ++writes;
        return size;
    };
    transport->_socket = socket;
    QPointer<QTcpSocket> retired = socket;
    connect(transport.get(), &NTRIPTransport::plaintextCredentialsWarning, this, [&]() {
        if (action == 1) {
            transport.reset();
        } else {
            transport->stop();
            if (action == 2) {
                transport->start();
            }
        }
    });
    static const QString warningMessage =
        QStringLiteral("Sending credentials without TLS \u2014 data is not encrypted");
#ifdef QGC_PORTABLE_TEST
    static std::atomic<QtMessageHandler> previousHandler{nullptr};
    static std::atomic_int warningCount{0};
    warningCount = 0;
    previousHandler =
        qInstallMessageHandler([](QtMsgType type, const QMessageLogContext& context, const QString& message) {
            if (type == QtWarningMsg && qstrcmp(context.category, "GPS.NTRIPHttpTransport") == 0 &&
                message == warningMessage) {
                ++warningCount;
            } else if (const auto handler = previousHandler.load()) {
                handler(type, context, message);
            }
        });
    const auto restoreHandler = qScopeGuard([] { qInstallMessageHandler(previousHandler.load()); });
#else
    expectLogMessage(
        "GPS.NTRIPHttpTransport", QtWarningMsg,
        QRegularExpression(QRegularExpression::anchoredPattern(QRegularExpression::escape(warningMessage))));
#endif
    transport->_sendHttpRequest();
#ifdef QGC_PORTABLE_TEST
    QCOMPARE(warningCount.load(), 1);
#else
    verifyExpectedLogMessage();
#endif
    QCOMPARE(writes, 0);
    if (action == 2) {
        QVERIFY(transport->_socket);
        QVERIFY(transport->_socket != retired);
        QVERIFY(transport->_connectTimeoutTimer.isActive());
    }
}

void NTRIPReentrancyTest::writeAdmissionFailure_data()
{
    QTest::addColumn<int>("accepted");
    QTest::addColumn<bool>("nmea");
    for (bool nmea : {false, true}) {
        QTest::newRow(nmea ? "nmea-failed" : "request-failed") << -1 << nmea;
        QTest::newRow(nmea ? "nmea-partial" : "request-partial") << 1 << nmea;
    }
}

void NTRIPReentrancyTest::writeAdmissionFailure()
{
    QFETCH(int, accepted);
    QFETCH(bool, nmea);
    NTRIPHttpTransport transport(config(), {});
    auto* socket = new WriteSocket(&transport);
    socket->admit = [accepted](qint64) { return accepted; };
    transport._socket = socket;
    QSignalSpy errors(&transport, &NTRIPTransport::error);
    if (nmea) {
        QVERIFY(!transport._write(QByteArrayLiteral("$GPGGA")));
    } else {
        transport._sendHttpRequest();
    }
    QCOMPARE(errors.size(), 1);
    QCOMPARE(qvariant_cast<NTRIPFailure>(errors.first().first()).code, NTRIPError::SocketError);
    QVERIFY(transport._stopped);
}

void NTRIPReentrancyTest::handshakeRetiresAttempt_data()
{
    QTest::addColumn<bool>("icy");
    QTest::addColumn<int>("action");
    for (bool icy : {false, true}) {
        QTest::newRow(icy ? "icy-stop" : "http-stop") << icy << 0;
        QTest::newRow(icy ? "icy-delete" : "http-delete") << icy << 1;
        QTest::newRow(icy ? "icy-restart" : "http-restart") << icy << 2;
    }
}

void NTRIPReentrancyTest::handshakeRetiresAttempt()
{
    QFETCH(bool, icy);
    QFETCH(int, action);
    auto transport = std::make_unique<NTRIPHttpTransport>(config(), NTRIPRtcmFilterConfig{});
    transport->_socket = new QTcpSocket(transport.get());
    transport->_socket->open(QIODevice::ReadOnly);
    const QByteArray response =
        (icy ? QByteArrayLiteral("ICY 200 OK\r\n") : QByteArrayLiteral("HTTP/1.1 200 OK\r\n\r\n")) +
        GpsTestHelpers::buildRtcmFrame(1005);
    int frames = 0;
    connect(transport.get(), &NTRIPTransport::RTCMDataUpdate, this, [&]() { ++frames; });
    connect(transport.get(), &NTRIPTransport::connected, this, [&]() {
        if (action == 1) {
            transport.reset();
        } else {
            transport->stop();
            if (action == 2) {
                transport->start();
            }
        }
    });
    transport->_processHttpBytes(response, 123);
    QCOMPARE(frames, 0);
    if (transport) {
        QVERIFY(!transport->_dataWatchdogTimer.isActive());
    }
}

void NTRIPReentrancyTest::failureCanRestart_data()
{
    warningRetiresAttempt_data();
}

void NTRIPReentrancyTest::failureCanRestart()
{
    QFETCH(int, action);
    auto transport = std::make_unique<NTRIPHttpTransport>(config(), NTRIPRtcmFilterConfig{});
    transport->_socket = new QTcpSocket(transport.get());
    const auto previous = transport->_socket;
    connect(transport.get(), &NTRIPTransport::error, this, [&]() {
        if (action == 0) {
            transport->stop();
        } else if (action == 1) {
            transport.reset();
        } else {
            transport->start();
        }
    });
    transport->_processHttpBytes("HTTP/1.1 503 Unavailable\r\nRetry-After: 17\r\n\r\n", 123);
    if (action == 2) {
        QVERIFY(transport->_socket);
        QVERIFY(transport->_socket != previous);
        QVERIFY(!transport->_stopped);
        QVERIFY(transport->_connectTimeoutTimer.isActive());
    } else if (action == 0) {
        QVERIFY(transport->_stopped);
        QVERIFY(!transport->_socket);
    } else {
        QVERIFY(!transport);
    }
}

void NTRIPReentrancyTest::httpFraming_data()
{
    QTest::addColumn<QByteArray>("wire");
    QTest::addColumn<QByteArray>("body");
    QTest::addColumn<bool>("connected");
    QTest::addColumn<int>("error");
    const int invalid = static_cast<int>(NTRIPError::InvalidHttpResponse);
    const int oversized = static_cast<int>(NTRIPError::HeaderTooLarge);
    const int mountpoint = static_cast<int>(NTRIPError::InvalidMountpoint);
    const QByteArray ok = "HTTP/1.1 200 OK\r\n";
    QTest::newRow("close-delimited") << ok + "\r\nabc" << QByteArray("abc") << true << -1;
    QTest::newRow("content-length") << ok + "Content-Length: 3\r\n\r\nabc" << QByteArray("abc") << true << -1;
    QTest::newRow("duplicate-length") << ok + "Content-Length: 3, 03\r\ncontent-length: 3\r\n\r\nabc"
                                      << QByteArray("abc") << true << -1;
    QTest::newRow("zero-length") << ok + "Content-Length: 0\r\n\r\n" << QByteArray() << true << -1;
    QTest::newRow("empty-status") << QByteArray("HTTP/1.1 204 No Content\r\n\r\n") << QByteArray() << true << -1;
    QTest::newRow("informational") << QByteArray("HTTP/1.1 100 Continue\r\n\r\n") + ok + "\r\nabc" << QByteArray("abc")
                                   << true << -1;
    const QByteArray binary = QByteArray::fromHex("d30000");
    QTest::newRow("bare-icy") << QByteArray("ICY 200 OK\r\n") + binary << binary << true << -1;
    QTest::newRow("icy-separator") << QByteArray("ICY 200 OK\r\n\r\n") + binary << binary << true << -1;
    QTest::newRow("icy-headers") << QByteArray("icy 200 OK\r\nServer: legacy\r\nContent-Length: 3\r\n\r\n") + binary
                                 << binary << true << -1;
    QTest::newRow("icy-folded-header") << QByteArray("ICY 200 OK\r\n Content-Length: 3\r\n\r\n") << QByteArray() << true
                                       << invalid;
    QTest::newRow("icy-missing-separator")
        << QByteArray("ICY 200 OK\r\nContent-Length: 3\r\n") + binary << QByteArray() << true << invalid;
    QTest::newRow("source-table") << QByteArray("SOURCETABLE 200 OK\r\nSTR;MP\r\n") << QByteArray() << false
                                  << mountpoint;
    QTest::newRow("source-content-type") << ok + "Content-Type: gnss/sourcetable; charset=utf-8\r\n\r\n"
                                         << QByteArray() << false << mountpoint;
    const QByteArray chunked = ok + "Transfer-Encoding: chunked\r\n\r\n";
    QTest::newRow("chunks") << chunked + "1;name=\"escaped\\\"value\"\r\na\r\n2\r\nbc\r\n0\r\nX-End: yes\r\n\r\n"
                            << QByteArray("abc") << true << -1;
    QTest::newRow("chunk-extension-whitespace") << chunked + "3 \t; name = value ;flag\r\nabc\r\n0\r\n\r\n"
                                                << QByteArray("abc") << true << -1;
    QTest::newRow("truncated-status") << QByteArray("HTTP/1.1 20") << QByteArray() << false << invalid;
    QTest::newRow("truncated-headers") << ok + "Server: x\r\n" << QByteArray() << false << invalid;
    QTest::newRow("truncated-length") << ok + "Content-Length: 4\r\n\r\nabc" << QByteArray("abc") << true << invalid;
    QTest::newRow("maximum-length-streamed")
        << ok + "Content-Length: 18446744073709551615\r\n\r\nabc" << QByteArray("abc") << true << invalid;
    QTest::newRow("extra-body") << ok + "Content-Length: 2\r\n\r\nabc" << QByteArray("ab") << true << invalid;
    QTest::newRow("truncated-chunk") << chunked + "4\r\nabc" << QByteArray("abc") << true << invalid;
    QTest::newRow("truncated-chunk-end") << chunked + "3\r\nabc\r" << QByteArray("abc") << true << invalid;
    QTest::newRow("missing-last-chunk") << chunked + "3\r\nabc\r\n" << QByteArray("abc") << true << invalid;
    QTest::newRow("truncated-trailer") << chunked + "3\r\nabc\r\n0\r\nX-End: yes\r\n"
                                       << QByteArray("abc") << true << invalid;
    QTest::newRow("bad-chunk-end") << chunked + "3\r\nabc!\n" << QByteArray("abc") << true << invalid;
    QTest::newRow("bad-trailer") << chunked + "0\r\nFolded: x\r\n y\r\n\r\n" << QByteArray() << true << invalid;
    QTest::newRow("framing-trailer") << chunked + "0\r\nContent-Length: 3\r\n\r\n" << QByteArray() << true << invalid;
    QTest::newRow("maximum-chunk-streamed") << chunked + "1000000\r\nx" << QByteArray("x") << true << invalid;
    for (const QByteArray size :
         {"-1", "+1", " 1", "1 ", "1000001", "10000000000000000", "1;=x", "1;x=\"", "1;x=\vvalue"}) {
        QTest::newRow(("chunk-" + size).constData()) << chunked + size + "\r\n" << QByteArray() << true << invalid;
    }
    for (const QByteArray header : {"Content-Length: -1",
                                    "Content-Length: +1",
                                    "Content-Length: 1.0",
                                    "Content-Length:",
                                    "Content-Length: 18446744073709551616",
                                    "Content-Length: 3,4",
                                    "Content-Length: 3\r\nContent-Length: 4",
                                    "Content-Length: 3\r\nTransfer-Encoding: chunked",
                                    "Transfer-Encoding: chunked\r\nContent-Length: 3",
                                    "Transfer-Encoding: gzip, chunked",
                                    "Transfer-Encoding: identity",
                                    "Transfer-Encoding: chunked, chunked",
                                    "Transfer-Encoding: chunked\r\nTransfer-Encoding: chunked",
                                    "Content-Encoding: gzip",
                                    "Bad Header: x",
                                    "Content-Length : 3",
                                    " Content-Length: 3",
                                    "X: value\r\n folded",
                                    "X: value\nOther: value",
                                    "X: bad\rvalue",
                                    "X: bad\x01value"}) {
        QTest::newRow(header.constData()) << ok + header + "\r\n\r\n" << QByteArray() << false << invalid;
    }
    QTest::newRow("nul-header") << ok + QByteArray("X: a\0b\r\n\r\n", 10) << QByteArray() << false << invalid;
    QTest::newRow("del-header") << ok + "X: value\x7f\r\n\r\n" << QByteArray() << false << invalid;
    for (const QByteArray status : {"garbage", "noise\r\nHTTP/1.1 200 OK", "HTTP/2 200 OK", "HTTP/1.1 600 Bad",
                                    "HTTP/1.1 2000 OK", "HTTP/1.1\t200 OK"}) {
        QTest::newRow(status.constData()) << status + "\r\n\r\n" << QByteArray() << false << invalid;
    }
    QTest::newRow("switching-protocol") << QByteArray("HTTP/1.1 101 Switching Protocols\r\n\r\n") << QByteArray()
                                        << false << invalid;
    QTest::newRow("too-many-informationals")
        << QByteArray("HTTP/1.1 100 Continue\r\n\r\n").repeated(5) << QByteArray() << false << invalid;
    QTest::newRow("http10-chunked") << QByteArray("HTTP/1.0 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n")
                                    << QByteArray() << false << invalid;
    QTest::newRow("line-limit") << ok + "X: " + QByteArray(NTRIPHttpDecoder::MAX_LINE_BYTES - 5, 'x') + "\r\n\r\n"
                                << QByteArray() << true << -1;
    QTest::newRow("oversized-line") << ok + "X: " + QByteArray(NTRIPHttpDecoder::MAX_LINE_BYTES - 4, 'x') + "\r\n\r\n"
                                    << QByteArray() << false << oversized;
    const QByteArray largeHeader = "X: " + QByteArray(8187, 'x') + "\r\n";
    const QByteArray nearLimit = ok + largeHeader.repeated(3);
    const auto remaining = NTRIPHttpDecoder::MAX_HEADER_BYTES - nearLimit.size() - 7;
    QTest::newRow("header-limit") << nearLimit + "X: " + QByteArray(remaining, 'x') + "\r\n\r\n"
                                  << QByteArray() << true << -1;
    QTest::newRow("oversized-header") << nearLimit + "X: " + QByteArray(remaining + 1, 'x') + "\r\n\r\n"
                                      << QByteArray() << false << oversized;
    QTest::newRow("header-count-limit") << ok + QByteArray("X: y\r\n").repeated(128) + "\r\n"
                                        << QByteArray() << true << -1;
    QTest::newRow("too-many-headers") << ok + QByteArray("X: y\r\n").repeated(129) + "\r\n"
                                      << QByteArray() << false << oversized;
}

void NTRIPReentrancyTest::httpFraming()
{
    QFETCH(QByteArray, wire);
    QFETCH(QByteArray, body);
    QFETCH(bool, connected);
    QFETCH(int, error);
    const auto now = QDateTime::fromString(QStringLiteral("2026-09-16T12:00:00Z"), Qt::ISODate);
    for (qsizetype fragment : {qsizetype(1), qsizetype(7), wire.size()}) {
        NTRIPHttpDecoder decoder;
        QByteArray decoded;
        int handshakes = 0;
        bool complete = false;
        std::optional<NTRIPFailure> failure;
        const auto consume = [&](const NTRIPHttpDecoder::Result& result) {
            decoded += result.body;
            handshakes += result.connected;
            complete = result.complete;
            if (result.failure) {
                failure = result.failure;
            }
        };
        for (qsizetype offset = 0; offset < wire.size(); offset += fragment) {
            consume(decoder.feed(QByteArrayView(wire).sliced(offset, std::min(fragment, wire.size() - offset)), now));
        }
        consume(decoder.finish());
        QCOMPARE(decoded, body);
        QCOMPARE(handshakes, connected ? 1 : 0);
        QCOMPARE(failure ? static_cast<int>(failure->code) : -1, error);
        QCOMPARE(complete, error == -1);
        decoder.reset();
        const auto fresh = decoder.feed("HTTP/1.1 200 OK\r\nContent-Length: 1\r\n\r\nx", now);
        QVERIFY(fresh.connected && fresh.complete && !fresh.failure);
        QCOMPARE(fresh.body, QByteArray("x"));
    }
}

void NTRIPReentrancyTest::retryAfter_data()
{
    QTest::addColumn<QByteArray>("value");
    QTest::addColumn<int>("delayMs");
    QTest::newRow("zero") << QByteArray("0") << 0;
    QTest::newRow("delta") << QByteArray("17") << 17000;
    QTest::newRow("cap") << QByteArray("301") << 300000;
    QTest::newRow("uint64-max") << QByteArray("18446744073709551615") << 300000;
    QTest::newRow("overflow") << QByteArray("18446744073709551616") << 0;
    QTest::newRow("negative") << QByteArray("-10") << 0;
    QTest::newRow("positive-sign") << QByteArray("+10") << 0;
    QTest::newRow("decimal") << QByteArray("1.5") << 0;
    QTest::newRow("invalid") << QByteArray("tomorrow") << 0;
    QTest::newRow("empty") << QByteArray() << 0;
    QTest::newRow("duplicate") << QByteArray("10\r\nRetry-After: 20") << 0;
    QTest::newRow("date") << QByteArray("Wed, 16 Sep 2026 12:00:10 GMT") << 9500;
    QTest::newRow("rfc850-date") << QByteArray("Wednesday, 16-Sep-26 12:00:10 GMT") << 9500;
    QTest::newRow("asctime-date") << QByteArray("Wed Sep 16 12:00:10 2026") << 9500;
    QTest::newRow("asctime-single-digit-day") << QByteArray("Tue Oct  6 12:00:10 2026") << 300000;
    QTest::newRow("rfc850-previous-century") << QByteArray("Friday, 16-Sep-94 12:00:10 GMT") << 0;
    QTest::newRow("past-date") << QByteArray("Wed, 16 Sep 2026 11:59:59 GMT") << 0;
    QTest::newRow("capped-date") << QByteArray("Wed, 16 Sep 2026 12:10:00 GMT") << 300000;
    QTest::newRow("invalid-date") << QByteArray("Mon, 31 Feb 2026 12:00:10 GMT") << 0;
    QTest::newRow("wrong-weekday") << QByteArray("Tue, 16 Sep 2026 12:00:10 GMT") << 0;
}

void NTRIPReentrancyTest::retryAfter()
{
    QFETCH(QByteArray, value);
    QFETCH(int, delayMs);
    const auto now = QDateTime::fromString(QStringLiteral("2026-09-16T12:00:00.500Z"), Qt::ISODateWithMs);
    NTRIPHttpTransport transport(config(), {});
    QSignalSpy errors(&transport, &NTRIPTransport::error);
    QList<NTRIPFailure> queued;
    connect(
        &transport, &NTRIPTransport::error, this, [&](const NTRIPFailure& failure) { queued.append(failure); },
        Qt::QueuedConnection);
    transport._processHttpBytes("HTTP/1.1 503 Unavailable\r\nRetry-After: " + value + "\r\n\r\n", 123, now);
    QCOMPARE(errors.size(), 1);
    const auto failure = qvariant_cast<NTRIPFailure>(errors[0][0]);
    QCOMPARE(failure.code, NTRIPError::HttpError);
    QCOMPARE(failure.retryAfter, std::chrono::milliseconds(delayMs));
    QVERIFY(queued.isEmpty());
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    QCOMPARE(queued.size(), 1);
    QCOMPARE(queued.first().retryAfter, failure.retryAfter);
    if (value.contains("GMT")) {
        NTRIPHttpDecoder decoder;
        const auto result = decoder.feed("HTTP/1.1 503 Unavailable\r\nRetry-After: " + value + "\r\n\r\n", {});
        QVERIFY(result.failure);
        QCOMPARE(result.failure->retryAfter, std::chrono::milliseconds(0));
    }
}

void NTRIPReentrancyTest::bodyPublicationRetiresAttempt_data()
{
    warningRetiresAttempt_data();
}

void NTRIPReentrancyTest::bodyPublicationRetiresAttempt()
{
    QFETCH(int, action);
    auto transport = std::make_unique<NTRIPHttpTransport>(config(), NTRIPRtcmFilterConfig{});
    const QByteArray frame = GpsTestHelpers::buildRtcmFrame(1005);
    int frames = 0;
    int errors = 0;
    connect(transport.get(), &NTRIPTransport::error, this, [&]() { ++errors; });
    connect(transport.get(), &NTRIPTransport::RTCMDataUpdate, this, [&]() {
        ++frames;
        if (action == 1) {
            transport.reset();
        } else if (action == 0) {
            transport->stop();
        } else {
            transport->start();
        }
    });
    const QByteArray wire = "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n" +
                            QByteArray::number(frame.size() * 2, 16) + "\r\n" + frame + frame + "!\r\n";
    transport->_processHttpBytes(wire, 123);
    QCOMPARE(frames, 1);
    QCOMPARE(errors, 0);
    if (action == 2) {
        QVERIFY(transport->_connectTimeoutTimer.isActive());
        QVERIFY(!transport->_dataWatchdogTimer.isActive());
    }
}

void NTRIPReentrancyTest::chunkedReceiptTimesAndEvidence()
{
    NTRIPHttpTransport transport(config(), {.whitelist = QStringLiteral("1005")});
    QSignalSpy observed(&transport, &NTRIPTransport::correctionFrameReceived);
    QSignalSpy accepted(&transport, &NTRIPTransport::RTCMDataUpdate);
    QList<RTCMFrameDecoder::Result> queued;
    connect(
        &transport, &NTRIPTransport::correctionFrameReceived, this,
        [&](const RTCMFrameDecoder::Result& frame) { queued.append(frame); }, Qt::QueuedConnection);
    const auto first = GpsTestHelpers::buildRtcmFrame(1005);
    const auto second = GpsTestHelpers::buildRtcmFrame(1077);
    transport._processHttpBytes("HTTP/1.1 200 OK\r\nTransfer-Encoding: chu", 50);
    transport._processHttpBytes("nked\r\n\r\n1\r\n" + first.first(1), 100);
    transport._processHttpBytes(
        "\r\n" + QByteArray::number(first.size() - 1 + second.size(), 16) + "\r\n" + first.sliced(1) + second + "\r\n",
        200);
    QCOMPARE(observed.size(), 2);
    QCOMPARE(accepted.size(), 1);
    QCOMPARE(qvariant_cast<RTCMFrameDecoder::Result>(observed[0][0]).receivedAtMs, 100);
    const auto filtered = qvariant_cast<RTCMFrameDecoder::Result>(observed[1][0]);
    QVERIFY(filtered.valid && filtered.filtered);
    QCOMPARE(filtered.receivedAtMs, 200);
    QVERIFY(queued.isEmpty());
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    QCOMPARE(queued.size(), 2);
    QCOMPARE(queued[0].receivedAtMs, 100);
    QCOMPARE(queued[1].receivedAtMs, 200);
}

void NTRIPReentrancyTest::socketTermination_data()
{
    QTest::addColumn<int>("mode");
    QTest::newRow("close-delimited") << 0;
    QTest::newRow("length-complete") << 1;
    QTest::newRow("length-truncated") << 2;
    QTest::newRow("chunked-complete") << 3;
    QTest::newRow("chunked-truncated") << 4;
}

void NTRIPReentrancyTest::socketTermination()
{
    QFETCH(int, mode);
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    auto configuration = config();
    configuration.port = server.serverPort();
    NTRIPHttpTransport transport(configuration, {});
    QSignalSpy errors(&transport, &NTRIPTransport::error);
    QSignalSpy frames(&transport, &NTRIPTransport::correctionFrameReceived);
    transport.start();
    QTRY_VERIFY(server.hasPendingConnections());
    std::unique_ptr<QTcpSocket> peer(server.nextPendingConnection());
    QTRY_VERIFY(peer->bytesAvailable() > 0);
    peer->readAll();
    const QByteArray frame = GpsTestHelpers::buildRtcmFrame(1005);
    const QByteArray body = frame.repeated(4000);
    QByteArray wire = "HTTP/1.1 200 OK\r\n";
    if (mode == 1 || mode == 2) {
        wire += "Content-Length: " + QByteArray::number(body.size() + (mode == 2)) + "\r\n";
    } else if (mode >= 3) {
        wire += "Transfer-Encoding: chunked\r\n";
    }
    wire += "\r\n";
    wire += mode >= 3 ? QByteArray::number(body.size(), 16) + "\r\n" + body + "\r\n" : body;
    if (mode == 3) {
        wire += "0\r\n\r\n";
    }
    QCOMPARE(peer->write(wire), wire.size());
    peer->disconnectFromHost();
    QTRY_COMPARE(errors.size(), 1);
    QCOMPARE(frames.size(), 4000);
    QCOMPARE(qvariant_cast<NTRIPFailure>(errors[0][0]).code,
             mode == 2 || mode == 4 ? NTRIPError::InvalidHttpResponse : NTRIPError::ServerDisconnected);
    QVERIFY(!transport._connectTimeoutTimer.isActive());
    QVERIFY(!transport._dataWatchdogTimer.isActive());
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    QCOMPARE(errors.size(), 1);
}

void NTRIPReentrancyTest::filterConfigurationUpdatesWithoutReconnect()
{
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    auto connection = config();
    connection.port = server.serverPort();
    const NTRIPRtcmFilterConfig initialFilter{.whitelist = QStringLiteral("1005,invalid,0,-1")};
    QCOMPARE(initialFilter.messageIds(), QVector<int>{1005});
    NTRIPHttpTransport transport(connection, initialFilter);
    QSignalSpy connected(&transport, &NTRIPTransport::connected);
    QSignalSpy finished(&transport, &NTRIPTransport::finished);
    QSignalSpy observed(&transport, &NTRIPTransport::correctionFrameReceived);
    QSignalSpy accepted(&transport, &NTRIPTransport::RTCMDataUpdate);
    transport.start();
    QTRY_VERIFY(server.hasPendingConnections());
    std::unique_ptr<QTcpSocket> peer(server.nextPendingConnection());
    QTRY_VERIFY(peer->bytesAvailable() > 0);
    QVERIFY(peer->readAll().startsWith("GET /TEST HTTP/1.1"));
    const auto frames = GpsTestHelpers::buildRtcmFrame(1005) + GpsTestHelpers::buildRtcmFrame(1077);
    const auto response = QByteArrayLiteral("HTTP/1.1 200 OK\r\n\r\n") + frames;
    QCOMPARE(peer->write(response), response.size());
    QTRY_COMPARE(observed.size(), 2);
    QCOMPARE(accepted.size(), 1);
    QCOMPARE(accepted.last()[1].toInt(), 1005);

    const auto socket = transport._socket;
    const auto attempt = transport._attempt;
    const NTRIPRtcmFilterConfig replacement{.whitelist = QStringLiteral("1077")};
    transport.setRtcmWhitelist(replacement.messageIds());
    QCOMPARE(peer->write(frames), frames.size());
    QTRY_COMPARE(observed.size(), 4);
    QCOMPARE(accepted.size(), 2);
    QCOMPARE(accepted.last()[1].toInt(), 1077);

    transport.setRtcmWhitelist(NTRIPRtcmFilterConfig{}.messageIds());
    QCOMPARE(peer->write(frames), frames.size());
    QTRY_COMPARE(observed.size(), 6);
    QCOMPARE(accepted.size(), 4);
    QCOMPARE(transport.config(), connection);
    QCOMPARE(transport._socket, socket);
    QCOMPARE(transport._attempt, attempt);
    QCOMPARE(connected.size(), 1);
    QCOMPARE(finished.size(), 0);
}

void NTRIPReentrancyTest::invalidFetchRetiresPendingReply()
{
    NTRIPSourceTableController controller;
    controller.fetch(config());
    const auto previous = controller._reply;
    QVERIFY(previous);
    const auto revision = controller._fetchRevision;
    controller.fetch({});
    QCOMPARE(controller.fetchStatus(), NTRIPSourceTableController::FetchStatus::Error);
    QVERIFY(!controller._reply);
    QVERIFY(!previous->isRunning());
    controller._onReplyFinished(previous, revision);
    QCOMPARE(controller.fetchStatus(), NTRIPSourceTableController::FetchStatus::Error);
    QCOMPARE(controller.mountpointModel()->rowCount(), 0);
}

void NTRIPReentrancyTest::sourceTableSuccessAndCache()
{
    const QByteArray table =
        "STR;MP1;Id1;RTCM 3.2;details;2;GPS;NET;USA;40.0;-74.0;0;1;gen;none;B;N;4800;misc\r\n"
        "ENDSOURCETABLE\r\n";
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    auto configuration = config();
    configuration.port = server.serverPort();
    NTRIPSourceTableController controller;
    controller.fetch(configuration);
    QTRY_VERIFY(server.hasPendingConnections());
    std::unique_ptr<QTcpSocket> peer(server.nextPendingConnection());
    QTRY_VERIFY(peer->bytesAvailable() > 0);
    QVERIFY(peer->readAll().startsWith("GET / HTTP/1.1"));
    const QByteArray response =
        "HTTP/1.1 200 OK\r\nContent-Length: " + QByteArray::number(table.size()) + "\r\n\r\n" + table;
    QCOMPARE(peer->write(response), response.size());
    QTRY_COMPARE(controller.fetchStatus(), NTRIPSourceTableController::FetchStatus::Success);
    QCOMPARE(controller.mountpointModel()->rowCount(), 1);
    controller.fetch(configuration);
    QCOMPARE(controller.fetchStatus(), NTRIPSourceTableController::FetchStatus::Success);
    QVERIFY(!controller._reply);
    QVERIFY(controller._cacheAge.isValid());

    auto invalid = configuration;
    invalid.mountpoint = QStringLiteral("invalid\r\nmount");
    controller.fetch(invalid);
    QCOMPARE(controller.fetchStatus(), NTRIPSourceTableController::FetchStatus::Error);
    QVERIFY(!controller._cacheAge.isValid());
    QCOMPARE(controller.mountpointModel()->rowCount(), 0);
}

void NTRIPReentrancyTest::abortCallbackSupersedesReplacement()
{
    NTRIPSourceTableController controller;
    controller.fetch(config());
    const auto previous = controller._reply;
    auto replacement = config();
    replacement.port = 2102;
    connect(previous, &QNetworkReply::finished, this, [&]() { controller.fetch(replacement); });
    controller.fetch({});
    QCOMPARE(controller.fetchStatus(), NTRIPSourceTableController::FetchStatus::InProgress);
    QVERIFY(controller._reply && controller._reply != previous);
    QCOMPARE(controller._reply->url().port(), 2102);
}

void NTRIPReentrancyTest::abortCallbackDeletesReply()
{
    NTRIPSourceTableController controller;
    controller.fetch(config());
    const auto previous = controller._reply;
    connect(previous, &QNetworkReply::finished, this, [previous]() { delete previous.data(); });
    controller.fetch({});
    QVERIFY(!previous);
    QVERIFY(!controller._reply);
    QCOMPARE(controller.fetchStatus(), NTRIPSourceTableController::FetchStatus::Error);
}

void NTRIPReentrancyTest::deletedReplyPublishesError()
{
    NTRIPSourceTableController controller;
    controller.fetch(config());
    delete controller._reply.data();
    QVERIFY(!controller._reply);
    QCOMPARE(controller.fetchStatus(), NTRIPSourceTableController::FetchStatus::Error);
    QVERIFY(!controller.fetchError().isEmpty());
}

void NTRIPReentrancyTest::fetchNotificationReentry_data()
{
    QTest::addColumn<bool>("destroy");
    QTest::newRow("replace") << false;
    QTest::newRow("delete") << true;
}

void NTRIPReentrancyTest::fetchNotificationReentry()
{
    QFETCH(bool, destroy);
    auto controller = std::make_unique<NTRIPSourceTableController>();
    connect(controller.get(), &NTRIPSourceTableController::fetchStatusChanged, this, [&]() {
        if (controller->fetchStatus() == NTRIPSourceTableController::FetchStatus::InProgress) {
            if (destroy) {
                controller.reset();
            } else {
                controller->fetch({});
            }
        }
    });
    controller->fetch(config());
    if (!destroy) {
        QCOMPARE(controller->fetchStatus(), NTRIPSourceTableController::FetchStatus::Error);
        QVERIFY(!controller->_reply);
    }
}

void NTRIPReentrancyTest::ggaSourceSelection()
{
    using Source = NTRIPGgaProvider::PositionSource;
    NTRIPGgaProvider provider;
    MockNTRIPTransport transport;
    QList<Source> calls;
    for (auto source : {Source::VehicleGPS, Source::VehicleEKF, Source::RTKBase, Source::GCSPosition}) {
        provider.setPositionProvider(source, [&, source]() {
            calls.append(source);
            return source == Source::RTKBase ? PositionResult{QGeoCoordinate(47, 8, 450), QStringLiteral("RTK")}
                                             : PositionResult{};
        });
    }
    provider.start(&transport);
    QCOMPARE(calls, (QList<Source>{Source::VehicleGPS, Source::VehicleEKF, Source::RTKBase}));
    QCOMPARE(transport.sentNmea.size(), 1);
    provider.stop();
    calls.clear();
    provider.configure({Source::VehicleEKF});
    provider.start(&transport);
    QCOMPARE(calls, (QList<Source>{Source::VehicleEKF}));
    QCOMPARE(transport.sentNmea.size(), 1);
    QVERIFY(provider.currentSource().isEmpty());
}

void NTRIPReentrancyTest::ggaCallbackStopsProvider()
{
    NTRIPGgaProvider provider;
    MockNTRIPTransport transport;
    provider.setPositionProvider(NTRIPGgaProvider::PositionSource::VehicleGPS, [&]() {
        provider.stop();
        return PositionResult{QGeoCoordinate(47, 8), QStringLiteral("Vehicle GPS")};
    });
    provider.start(&transport);
    QVERIFY(transport.sentNmea.isEmpty());
    QVERIFY(provider.currentSource().isEmpty());
}

QGC_REGISTER_PORTABLE_TEST(NTRIPReentrancyTest, TestLabel::Unit)
#include "NTRIPReentrancyTest.moc"
