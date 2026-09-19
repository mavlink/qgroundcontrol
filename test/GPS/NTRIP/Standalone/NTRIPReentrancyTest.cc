#include <algorithm>
#include <atomic>
#include <functional>
#include <memory>

#include <QtCore/QElapsedTimer>
#include <QtCore/QRegularExpression>
#include <QtCore/QTimer>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QTcpServer>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "../../RTCM/RTCMTestFixtures.h"
#include "../MockNTRIPTransport.h"
#include "NTRIPGgaProvider.h"
#include "NTRIPHttpDecoder.h"
#include "NTRIPHttpTransport.h"
#include "NTRIPSourceTableController.h"
#include "PortableTest.h"

namespace {
#ifdef QGC_PORTABLE_TEST
class WarningCapture
{
public:
    WarningCapture(const char* category, const QRegularExpression& pattern)
        : _category(category)
        , _pattern(pattern)
    {
        _active = this;
        _previous =
            qInstallMessageHandler([](QtMsgType type, const QMessageLogContext& context, const QString& message) {
                auto* capture = _active.load();
                if (capture && type == QtWarningMsg && qstrcmp(context.category, capture->_category) == 0 &&
                    capture->_pattern.match(message).hasMatch()) {
                    ++capture->_count;
                } else if (const auto handler = _previous.load()) {
                    handler(type, context, message);
                }
            });
    }

    ~WarningCapture()
    {
        qInstallMessageHandler(_previous.load());
        _active = nullptr;
    }

    int count() const { return _count.load(); }

private:
    const char* _category;
    QRegularExpression _pattern;
    std::atomic_int _count{0};
    static inline std::atomic<WarningCapture*> _active{nullptr};
    static inline std::atomic<QtMessageHandler> _previous{nullptr};
};
#endif

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
    void legacyCaster_data();
    void legacyCaster();
    void icyPartialFrame_data();
    void icyPartialFrame();
    void errorDiagnostics_data();
    void errorDiagnostics();
    void errorBodyBounds();
    void errorBodyDeadline();
    void errorBodySocketFailure();
    void pendingErrorRetiresAttempt_data();
    void pendingErrorRetiresAttempt();
    void httpFraming_data();
    void httpFraming();
    void retryAfter_data();
    void retryAfter();
    void bodyPublicationRetiresAttempt_data();
    void bodyPublicationRetiresAttempt();
    void receiptTimesAndEvidence_data();
    void receiptTimesAndEvidence();
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
    void ggaSourceChangesPreserveCadence();
    void ggaIntervalChangesRestartCadence_data();
    void ggaIntervalChangesRestartCadence();
    void ggaConfigurationPreservesFastRetry();
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
    const QRegularExpression warningPattern(
        QRegularExpression::anchoredPattern(QRegularExpression::escape(warningMessage)));
#ifdef QGC_PORTABLE_TEST
    const WarningCapture warning("GPS.NTRIPHttpTransport", warningPattern);
#else
    expectLogMessage("GPS.NTRIPHttpTransport", QtWarningMsg, warningPattern);
#endif
    transport->_sendHttpRequest();
#ifdef QGC_PORTABLE_TEST
    QCOMPARE(warning.count(), 1);
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
    QTest::addColumn<bool>("chunked");
    QTest::addColumn<int>("action");
    for (bool icy : {false, true}) {
        QTest::newRow(icy ? "icy-stop" : "http-stop") << icy << false << 0;
        QTest::newRow(icy ? "icy-delete" : "http-delete") << icy << false << 1;
        QTest::newRow(icy ? "icy-restart" : "http-restart") << icy << false << 2;
    }
    QTest::newRow("chunked-stop") << false << true << 0;
    QTest::newRow("chunked-delete") << false << true << 1;
    QTest::newRow("chunked-restart") << false << true << 2;
}

void NTRIPReentrancyTest::handshakeRetiresAttempt()
{
    QFETCH(bool, icy);
    QFETCH(bool, chunked);
    QFETCH(int, action);
    auto transport = std::make_unique<NTRIPHttpTransport>(config(), NTRIPRtcmFilterConfig{});
    transport->_socket = new QTcpSocket(transport.get());
    transport->_socket->open(QIODevice::ReadOnly);
    const auto frame = GpsTestHelpers::buildRtcmFrame(1005);
    const QByteArray response =
        chunked ? "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n" + QByteArray::number(frame.size(), 16) +
                      "\r\n" + frame + "\r\n0\r\n\r\n"
                : (icy ? QByteArrayLiteral("ICY 200 OK\r\n") : QByteArrayLiteral("HTTP/1.1 200 OK\r\n\r\n")) + frame;
    int frames = 0;
    connect(transport.get(), &NTRIPTransport::correctionFrameReceived, this, [&]() { ++frames; });
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
    QTest::addColumn<int>("action");
    QTest::addColumn<bool>("deferred");
    for (int action : {0, 1, 2}) {
        QTest::newRow(qPrintable(QStringLiteral("immediate-%1").arg(action))) << action << false;
        QTest::newRow(qPrintable(QStringLiteral("deferred-%1").arg(action))) << action << true;
    }
}

void NTRIPReentrancyTest::failureCanRestart()
{
    QFETCH(int, action);
    QFETCH(bool, deferred);
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    auto configuration = config();
    configuration.port = server.serverPort();
    int failures = 0;
    auto transport = std::make_unique<NTRIPHttpTransport>(configuration, NTRIPRtcmFilterConfig{});
    transport->_socket = new QTcpSocket(transport.get());
    const auto previous = transport->_socket;
    connect(transport.get(), &NTRIPTransport::error, this, [&]() {
        ++failures;
        if (action == 0) {
            transport->stop();
        } else if (action == 1) {
            transport.reset();
        } else {
            transport->start();
        }
    });
    transport->_processHttpBytes("HTTP/1.1 503 Unavailable\r\nRetry-After: 17\r\nContent-Length: " +
                                     QByteArray(deferred ? "100" : "0") + "\r\n\r\n",
                                 123);
    QTRY_COMPARE(failures, 1);
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

void NTRIPReentrancyTest::legacyCaster_data()
{
    QTest::addColumn<bool>("authenticated");
    QTest::newRow("anonymous") << false;
    QTest::newRow("authenticated") << true;
}

void NTRIPReentrancyTest::legacyCaster()
{
    QFETCH(bool, authenticated);
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    auto configuration = config();
    configuration.port = server.serverPort();
    if (authenticated) {
        configuration.username = QStringLiteral("test-user");
        configuration.password = QStringLiteral("test-password");
    }
    const QRegularExpression warningPattern(QStringLiteral("Sending credentials without TLS"));
#ifdef QGC_PORTABLE_TEST
    std::optional<WarningCapture> warning;
    if (authenticated) {
        warning.emplace("GPS.NTRIPHttpTransport", warningPattern);
    }
#else
    if (authenticated) {
        expectLogMessage("GPS.NTRIPHttpTransport", QtWarningMsg, warningPattern);
    }
#endif
    NTRIPHttpTransport transport(configuration, {});
    QSignalSpy connected(&transport, &NTRIPTransport::connected);
    QSignalSpy frames(&transport, &NTRIPTransport::correctionFrameReceived);
    QSignalSpy errors(&transport, &NTRIPTransport::error);
    transport.start();
    QTRY_VERIFY(server.hasPendingConnections());
    std::unique_ptr<QTcpSocket> peer(server.nextPendingConnection());
    QByteArray request;
    QTRY_VERIFY((request += peer->readAll()).endsWith("\r\n\r\n"));
    const bool accepted =
        request.startsWith("GET /TEST HTTP/1.1\r\n") && request.contains("\r\nUser-Agent: NTRIP ") &&
        (!authenticated ||
         request.contains("\r\nAuthorization: Basic " + QByteArray("test-user:test-password").toBase64() + "\r\n"));
    const auto frame = GpsTestHelpers::buildRtcmFrame(1005);
    if (accepted) {
        const QByteArray response = "ICY 200 OK\r\n" + frame;
        QCOMPARE(peer->write(response), response.size());
    } else {
        peer->disconnectFromHost();
    }
    QTRY_VERIFY(!frames.isEmpty() || !errors.isEmpty());
    QVERIFY(errors.isEmpty());
    QCOMPARE(connected.size(), 1);
    QCOMPARE(frames.size(), 1);
    QCOMPARE(qvariant_cast<RTCMFrameDecoder::Result>(frames.first().first()).data, frame);
    transport.stop();
#ifdef QGC_PORTABLE_TEST
    if (warning) {
        QCOMPARE(warning->count(), 1);
    }
#else
    if (authenticated) {
        verifyExpectedLogMessage();
    }
#endif
}

void NTRIPReentrancyTest::icyPartialFrame_data()
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

void NTRIPReentrancyTest::icyPartialFrame()
{
    QFETCH(QByteArray, suffix);
    const auto frame = QByteArray::fromHex("d300023ed0a4e000");
    for (const bool bytewise : {false, true}) {
        NTRIPHttpTransport transport(config(), {});
        QSignalSpy frames(&transport, &NTRIPTransport::correctionFrameReceived);
        QSignalSpy errors(&transport, &NTRIPTransport::error);
        QSignalSpy connected(&transport, &NTRIPTransport::connected);
        const QByteArray prefix = "ICY 200 OK\r\n" + suffix;
        if (bytewise) {
            for (char ch : prefix) {
                transport._processHttpBytes(QByteArray(1, ch), 50);
            }
            transport._processHttpBytes(frame.first(1), 100);
            QCOMPARE(frames.size(), 0);
            for (char ch : frame.sliced(1)) {
                transport._processHttpBytes(QByteArray(1, ch), 200);
            }
        } else {
            transport._processHttpBytes(prefix + frame, 100);
        }
        QCOMPARE(connected.size(), 1);
        QCOMPARE(frames.size(), 1);
        QCOMPARE(qvariant_cast<RTCMFrameDecoder::Result>(frames.first().first()).receivedAtMs, 100);
        transport._processHttpBytes(frame.repeated(1024), 300);
        QCOMPARE(frames.size(), 1025);
        QCOMPARE(qvariant_cast<RTCMFrameDecoder::Result>(frames.last().first()).receivedAtMs, 300);
        QVERIFY(errors.isEmpty());
    }
}

void NTRIPReentrancyTest::errorDiagnostics_data()
{
    QTest::addColumn<QByteArray>("wire");
    QTest::addColumn<int>("error");
    QTest::addColumn<QString>("preview");
    QTest::addColumn<int>("retryMs");
    const int http = static_cast<int>(NTRIPError::HttpError);
    const QByteArray denied = "HTTP/1.1 403 Forbidden\r\n";
    const QByteArray unavailable = "HTTP/1.1 503 Unavailable\r\nRetry-After: 120\r\n";
    const QByteArray html = "<b>Mountpoint denied</b>\nUse another mountpoint";
    const QString preview = QStringLiteral("Mountpoint denied Use another mountpoint");
    const QByteArray length = "Content-Length: " + QByteArray::number(html.size()) + "\r\n\r\n";
    QTest::newRow("content-length") << denied + length + html << http << preview << 0;
    QTest::newRow("close-delimited") << denied + "\r\n" + html << http << preview << 0;
    QTest::newRow("truncated-length") << denied + "Content-Length: 1000\r\n\r\n" + html << http << preview << 0;
    const QByteArray chunks =
        unavailable + "Transfer-Encoding: chunked\r\n\r\n" + QByteArray::number(html.size(), 16) + "\r\n" + html;
    QTest::newRow("chunked") << chunks + "\r\n0\r\n\r\n" << http << preview << 120000;
    QTest::newRow("malformed-error-chunk") << chunks + "!\n" << http << preview << 120000;
    QTest::newRow("binary-error-body") << denied + "\r\n" + GpsTestHelpers::buildRtcmFrame(1005) << http << QString()
                                       << 0;
    QTest::newRow("control-characters") << denied + "\r\nAccess\x01 denied\nRetry later" << http
                                        << QStringLiteral("Access denied Retry later") << 0;
    QTest::newRow("compressed-authentication")
        << QByteArray("HTTP/1.1 401 Unauthorized\r\nContent-Encoding: gzip\r\nContent-Length: 20\r\n\r\n") +
               QByteArray::fromHex("1f8b080000000000000303000000000000000000")
        << static_cast<int>(NTRIPError::AuthFailed) << QString() << 0;
    QTest::newRow("compressed-retry") << unavailable + "Content-Encoding: gzip\r\n\r\n" << http << QString() << 120000;
    QTest::newRow("unsupported-error-transfer") << unavailable + "Transfer-Encoding: gzip, chunked\r\n\r\n"
                                                << http << QString() << 120000;
}

void NTRIPReentrancyTest::errorDiagnostics()
{
    QFETCH(QByteArray, wire);
    QFETCH(int, error);
    QFETCH(QString, preview);
    QFETCH(int, retryMs);
    for (const qsizetype fragment : {qsizetype(1), qsizetype(7), wire.size()}) {
        NTRIPHttpDecoder decoder;
        std::optional<NTRIPFailure> failure;
        int failures = 0;
        const auto consume = [&](const NTRIPHttpDecoder::Result& result) {
            QVERIFY(result.body.isEmpty());
            QVERIFY(!result.connected);
            QVERIFY(!result.complete);
            if (result.failure) {
                failure = result.failure;
                ++failures;
            }
        };
        for (qsizetype offset = 0; offset < wire.size(); offset += fragment) {
            consume(decoder.feed(QByteArrayView(wire).sliced(offset, std::min(fragment, wire.size() - offset)),
                                 QDateTime::currentDateTimeUtc()));
        }
        consume(decoder.finish());
        QCOMPARE(failures, 1);
        QVERIFY(failure);
        QCOMPARE(static_cast<int>(failure->code), error);
        QCOMPARE(failure->retryAfter, std::chrono::milliseconds(retryMs));
        QVERIFY2(failure->detail.contains(preview), qPrintable(failure->detail));
        QVERIFY(!failure->detail.contains(QLatin1Char('<')));
        QVERIFY(!failure->detail.contains(QRegularExpression(QStringLiteral("[\\x00-\\x1f\\x7f]"))));
        QVERIFY(!failure->detail.contains(QStringLiteral("encoding")));
        const auto separator = failure->detail.indexOf(QStringLiteral(" \u2014 "));
        if (separator >= 0) {
            QVERIFY(failure->detail.size() - separator - 3 <= NTRIPHttpDecoder::MAX_ERROR_PREVIEW_CHARS);
        }
    }
}

void NTRIPReentrancyTest::errorBodyBounds()
{
    NTRIPHttpDecoder decoder;
    auto result = decoder.feed("HTTP/1.1 403 Forbidden\r\nContent-Length: 1000000\r\n\r\n", {});
    QVERIFY(result.awaitingErrorBody);
    QVERIFY(!result.failure);
    result = decoder.feed(QByteArray(NTRIPHttpDecoder::MAX_ERROR_BODY_BYTES - 1, 'x'), {});
    QVERIFY(result.awaitingErrorBody);
    QVERIFY(!result.failure);
    result = decoder.feed("x", {});
    QVERIFY(result.failure);
    QVERIFY(!result.awaitingErrorBody);
    QVERIFY(result.body.isEmpty());
    QCOMPARE(result.failure->detail, QStringLiteral("HTTP 403: Forbidden \u2014 ") +
                                         QString(NTRIPHttpDecoder::MAX_ERROR_PREVIEW_CHARS, QLatin1Char('x')));
    QVERIFY(!decoder.feed("must not be appended", {}).failure);
}

void NTRIPReentrancyTest::errorBodyDeadline()
{
    NTRIPHttpTransport transport(config(), {});
    QSignalSpy errors(&transport, &NTRIPTransport::error);
    QSignalSpy frames(&transport, &NTRIPTransport::correctionFrameReceived);
    transport._processHttpBytes(
        "HTTP/1.1 503 Unavailable\r\nRetry-After: 120\r\nContent-Length: 1000000\r\n\r\nPartial detail", 123);
    QVERIFY(errors.isEmpty());
    QTimer drip;
    drip.setInterval(NTRIPHttpTransport::kErrorBodyTimeout / 5);
    connect(&drip, &QTimer::timeout, &transport, [&]() { transport._processHttpBytes("x", 123); });
    drip.start();
    QTRY_COMPARE(errors.size(), 1);
    drip.stop();
    const auto failure = qvariant_cast<NTRIPFailure>(errors.first().first());
    QCOMPARE(failure.code, NTRIPError::HttpError);
    QCOMPARE(failure.retryAfter, std::chrono::seconds(120));
    QVERIFY(failure.detail.contains(QStringLiteral("Partial detail")));
    QVERIFY(frames.isEmpty());
    QVERIFY(!transport._errorBodyTimer.isActive());
    QVERIFY(!transport._connectTimeoutTimer.isActive());
    QVERIFY(!transport._dataWatchdogTimer.isActive());
}

void NTRIPReentrancyTest::pendingErrorRetiresAttempt_data()
{
    warningRetiresAttempt_data();
}

void NTRIPReentrancyTest::errorBodySocketFailure()
{
    NTRIPHttpTransport transport(config(), {});
    QSignalSpy errors(&transport, &NTRIPTransport::error);
    transport._processHttpBytes(
        "HTTP/1.1 503 Unavailable\r\nRetry-After: 120\r\nContent-Length: 100\r\n\r\nPartial detail", 123);
    QVERIFY(errors.isEmpty());
    transport._fail(NTRIPError::SocketError, QStringLiteral("Connection reset"));
    QCOMPARE(errors.size(), 1);
    const auto failure = qvariant_cast<NTRIPFailure>(errors.first().first());
    QCOMPARE(failure.code, NTRIPError::HttpError);
    QCOMPARE(failure.retryAfter, std::chrono::seconds(120));
    QVERIFY(failure.detail.contains(QStringLiteral("Partial detail")));
    QVERIFY(!transport._errorBodyTimer.isActive());
}

void NTRIPReentrancyTest::pendingErrorRetiresAttempt()
{
    QFETCH(int, action);
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    auto configuration = config();
    configuration.port = server.serverPort();
    auto transport = std::make_unique<NTRIPHttpTransport>(configuration, NTRIPRtcmFilterConfig{});
    QSignalSpy errors(transport.get(), &NTRIPTransport::error);
    transport->_processHttpBytes("HTTP/1.1 503 Unavailable\r\nContent-Length: 100\r\n\r\n", 123);
    QVERIFY(transport->_errorBodyTimer.isActive());
    if (action == 1) {
        transport.reset();
    } else if (action == 0) {
        transport->stop();
    } else {
        transport->start();
        transport->_processHttpBytes("HTTP/1.1 200 OK\r\n\r\n", 456);
    }
    if (transport) {
        QVERIFY(!transport->_errorBodyTimer.isActive());
    }
    bool deadlinePassed = false;
    QObject context;
    QTimer::singleShot(NTRIPHttpTransport::kErrorBodyTimeout + std::chrono::milliseconds{50}, &context,
                       [&]() { deadlinePassed = true; });
    QTRY_VERIFY(deadlinePassed);
    QVERIFY(errors.isEmpty());
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
    QTest::newRow("icy-non-header-prefix") << QByteArray("ICY 200 OK\r\n Content-Length: 3\r\n\r\n")
                                           << QByteArray(" Content-Length: 3\r\n\r\n") << true << -1;
    QTest::newRow("icy-missing-separator")
        << QByteArray("ICY 200 OK\r\nContent-Length: 3\r\n") + binary << binary << true << -1;
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
    transport._processHttpBytes("HTTP/1.1 503 Unavailable\r\nRetry-After: " + value + "\r\nContent-Length: 0\r\n\r\n",
                                123, now);
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
        const auto result =
            decoder.feed("HTTP/1.1 503 Unavailable\r\nRetry-After: " + value + "\r\nContent-Length: 0\r\n\r\n", {});
        QVERIFY(result.failure);
        QCOMPARE(result.failure->retryAfter, std::chrono::milliseconds(0));
    }
}

void NTRIPReentrancyTest::bodyPublicationRetiresAttempt_data()
{
    QTest::addColumn<bool>("chunked");
    QTest::addColumn<int>("action");
    for (bool chunked : {false, true}) {
        QTest::newRow(chunked ? "chunked-stop" : "identity-stop") << chunked << 0;
        QTest::newRow(chunked ? "chunked-delete" : "identity-delete") << chunked << 1;
        QTest::newRow(chunked ? "chunked-restart" : "identity-restart") << chunked << 2;
    }
}

void NTRIPReentrancyTest::bodyPublicationRetiresAttempt()
{
    QFETCH(bool, chunked);
    QFETCH(int, action);
    auto transport = std::make_unique<NTRIPHttpTransport>(config(), NTRIPRtcmFilterConfig{});
    const QByteArray frame = GpsTestHelpers::buildRtcmFrame(1005);
    int frames = 0;
    int errors = 0;
    connect(transport.get(), &NTRIPTransport::error, this, [&]() { ++errors; });
    connect(transport.get(), &NTRIPTransport::correctionFrameReceived, this,
            [&](const RTCMFrameDecoder::Result& result) {
                QVERIFY(result.valid && !result.filtered);
                QCOMPARE(result.receivedAtMs, 123);
                ++frames;
                if (action == 1) {
                    transport.reset();
                } else if (action == 0) {
                    transport->stop();
                } else {
                    transport->start();
                }
            });
    const QByteArray wire = chunked ? "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n" +
                                          QByteArray::number(frame.size() * 2, 16) + "\r\n" + frame + frame + "!\r\n"
                                    : "HTTP/1.1 200 OK\r\n\r\n" + frame + frame;
    transport->_processHttpBytes(wire, 123);
    QCOMPARE(frames, 1);
    QCOMPARE(errors, 0);
    if (action == 2) {
        QVERIFY(transport->_connectTimeoutTimer.isActive());
        QVERIFY(!transport->_dataWatchdogTimer.isActive());
    }
}

void NTRIPReentrancyTest::receiptTimesAndEvidence_data()
{
    QTest::addColumn<bool>("chunked");
    QTest::newRow("identity") << false;
    QTest::newRow("chunked") << true;
}

void NTRIPReentrancyTest::receiptTimesAndEvidence()
{
    QFETCH(bool, chunked);
    NTRIPHttpTransport transport(config(), {.whitelist = QStringLiteral("1005")});
    QSignalSpy observed(&transport, &NTRIPTransport::correctionFrameReceived);
    QList<RTCMFrameDecoder::Result> queued;
    connect(
        &transport, &NTRIPTransport::correctionFrameReceived, this,
        [&](const RTCMFrameDecoder::Result& frame) { queued.append(frame); }, Qt::QueuedConnection);
    const auto first = GpsTestHelpers::buildRtcmFrame(1005);
    const auto second = GpsTestHelpers::buildRtcmFrame(1077);
    if (chunked) {
        transport._processHttpBytes("HTTP/1.1 200 OK\r\nTransfer-Encoding: chu", 50);
        transport._processHttpBytes("nked\r\n\r\n1\r\n" + first.first(1), 100);
        transport._processHttpBytes("\r\n" + QByteArray::number(first.size() - 1 + second.size(), 16) + "\r\n" +
                                        first.sliced(1) + second + "\r\n",
                                    200);
    } else {
        transport._processHttpBytes("HTTP/1.1 200 OK\r\n\r\n" + first.first(1), 100);
        transport._processHttpBytes(first.sliced(1) + second, 200);
    }
    QCOMPARE(observed.size(), 2);
    const auto accepted = qvariant_cast<RTCMFrameDecoder::Result>(observed[0][0]);
    QVERIFY(accepted.valid && !accepted.filtered);
    QCOMPARE(accepted.receivedAtMs, 100);
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
    QVERIFY(peer);
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
    QSignalSpy observed(&transport, &NTRIPTransport::correctionFrameReceived);
    transport.start();
    QTRY_VERIFY(server.hasPendingConnections());
    std::unique_ptr<QTcpSocket> peer(server.nextPendingConnection());
    QTRY_VERIFY(peer->bytesAvailable() > 0);
    QVERIFY(peer->readAll().startsWith("GET /TEST HTTP/1.1"));
    const auto frames = GpsTestHelpers::buildRtcmFrame(1005) + GpsTestHelpers::buildRtcmFrame(1077);
    const auto response = QByteArrayLiteral("HTTP/1.1 200 OK\r\n\r\n") + frames;
    QCOMPARE(peer->write(response), response.size());
    QTRY_COMPARE(observed.size(), 2);
    auto first = qvariant_cast<RTCMFrameDecoder::Result>(observed[0][0]);
    auto second = qvariant_cast<RTCMFrameDecoder::Result>(observed[1][0]);
    QVERIFY(first.valid && !first.filtered);
    QVERIFY(second.valid && second.filtered);
    QCOMPARE(first.messageId, 1005);
    QCOMPARE(second.messageId, 1077);

    const auto socket = transport._socket;
    const auto attempt = transport._attempt;
    const NTRIPRtcmFilterConfig replacement{.whitelist = QStringLiteral("1077")};
    transport.setRtcmWhitelist(replacement.messageIds());
    QCOMPARE(peer->write(frames), frames.size());
    QTRY_COMPARE(observed.size(), 4);
    first = qvariant_cast<RTCMFrameDecoder::Result>(observed[2][0]);
    second = qvariant_cast<RTCMFrameDecoder::Result>(observed[3][0]);
    QVERIFY(first.valid && first.filtered);
    QVERIFY(second.valid && !second.filtered);
    QCOMPARE(second.messageId, 1077);

    transport.setRtcmWhitelist(NTRIPRtcmFilterConfig{}.messageIds());
    QCOMPARE(peer->write(frames), frames.size());
    QTRY_COMPARE(observed.size(), 6);
    first = qvariant_cast<RTCMFrameDecoder::Result>(observed[4][0]);
    second = qvariant_cast<RTCMFrameDecoder::Result>(observed[5][0]);
    QVERIFY(first.valid && !first.filtered);
    QVERIFY(second.valid && !second.filtered);
    QCOMPARE(transport.config(), connection);
    QCOMPARE(transport._socket, socket);
    QCOMPARE(transport._attempt, attempt);
    QCOMPARE(connected.size(), 1);
    QCOMPARE(socket->state(), QAbstractSocket::ConnectedState);
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

void NTRIPReentrancyTest::ggaSourceChangesPreserveCadence()
{
    using Source = NTRIPGgaProvider::PositionSource;
    constexpr std::chrono::milliseconds INTERVAL{100};
    NTRIPGgaProvider provider;
    MockNTRIPTransport transport;
    Source selected = Source::VehicleGPS;
    bool usedSelectedSource = true;
    for (const auto source : {Source::VehicleGPS, Source::RTKBase}) {
        provider.setPositionProvider(source, [&, source]() {
            usedSelectedSource &= source == selected;
            return PositionResult{QGeoCoordinate(47, 8, 450), QString::number(static_cast<int>(source))};
        });
    }
    provider.configure({selected, INTERVAL});
    provider.start(&transport);
    QCOMPARE(transport.sentNmea.size(), 1);

    int sourceChanges = 0;
    QTimer reconfigure;
    connect(&reconfigure, &QTimer::timeout, this, [&]() {
        if (transport.sentNmea.size() >= 4) {
            reconfigure.stop();
            provider.stop();
            return;
        }
        selected = selected == Source::VehicleGPS ? Source::RTKBase : Source::VehicleGPS;
        ++sourceChanges;
        const auto sentBefore = transport.sentNmea.size();
        provider.configure({selected, INTERVAL});
        QCOMPARE(transport.sentNmea.size(), sentBefore);
    });
    // Keep changing sources until three scheduled sends survive reconfiguration.
    reconfigure.start(0);
    QTRY_COMPARE(transport.sentNmea.size(), 4);
    reconfigure.stop();
    provider.stop();
    QVERIFY(sourceChanges > 3);
    QVERIFY(usedSelectedSource);
}

void NTRIPReentrancyTest::ggaIntervalChangesRestartCadence_data()
{
    QTest::addColumn<int>("initialIntervalMs");
    QTest::addColumn<int>("updatedIntervalMs");
    QTest::newRow("shorter") << 3600000 << 100;
    QTest::newRow("longer") << 100 << 1000;
}

void NTRIPReentrancyTest::ggaIntervalChangesRestartCadence()
{
    QFETCH(int, initialIntervalMs);
    QFETCH(int, updatedIntervalMs);
    using Source = NTRIPGgaProvider::PositionSource;
    NTRIPGgaProvider provider;
    MockNTRIPTransport transport;
    provider.setPositionProvider(Source::VehicleGPS,
                                 []() { return PositionResult{QGeoCoordinate(47, 8, 450), QStringLiteral("GPS")}; });
    provider.setPositionProvider(Source::RTKBase,
                                 []() { return PositionResult{QGeoCoordinate(48, 9, 460), QStringLiteral("RTK")}; });
    provider.configure({Source::VehicleGPS, std::chrono::milliseconds{initialIntervalMs}});
    provider.start(&transport);
    QCOMPARE(transport.sentNmea.size(), 1);

    QSignalSpy sourceChanged(&provider, &NTRIPGgaProvider::sourceChanged);
    QElapsedTimer elapsed;
    elapsed.start();
    provider.configure({Source::RTKBase, std::chrono::milliseconds{updatedIntervalMs}});
    QCOMPARE(transport.sentNmea.size(), 1);
    QVERIFY(sourceChanged.wait());
    provider.stop();
    QCOMPARE(transport.sentNmea.size(), 2);
    QCOMPARE(sourceChanged.first().first().toString(), QStringLiteral("RTK"));
    // Allow coarse-timer early delivery, without constraining late CI scheduling.
    QVERIFY(elapsed.elapsed() >= updatedIntervalMs * 4 / 5);
}

void NTRIPReentrancyTest::ggaConfigurationPreservesFastRetry()
{
    using Source = NTRIPGgaProvider::PositionSource;
    NTRIPGgaProvider provider;
    MockNTRIPTransport transport;
    provider.configure({Source::VehicleGPS, std::chrono::hours{1}});
    provider.setPositionProvider(Source::RTKBase,
                                 []() { return PositionResult{QGeoCoordinate(47, 8, 450), QStringLiteral("RTK")}; });
    QSignalSpy sourceChanged(&provider, &NTRIPGgaProvider::sourceChanged);
    QElapsedTimer elapsed;
    elapsed.start();
    provider.start(&transport);
    QVERIFY(transport.sentNmea.isEmpty());
    provider.configure({Source::RTKBase, std::chrono::milliseconds{100}});
    QVERIFY(transport.sentNmea.isEmpty());
    QVERIFY(sourceChanged.wait());
    QVERIFY(elapsed.elapsed() >= NTRIPGgaProvider::kFastRetryInterval.count() * 4 / 5);
    QCOMPARE(transport.sentNmea.size(), 1);
    QCOMPARE(provider.currentSource(), QStringLiteral("RTK"));
    QTRY_COMPARE(transport.sentNmea.size(), 2);
    provider.stop();
}

void NTRIPReentrancyTest::ggaCallbackStopsProvider()
{
    NTRIPGgaProvider provider;
    MockNTRIPTransport transport;
    provider.setPositionProvider(NTRIPGgaProvider::PositionSource::VehicleGPS, [&]() {
        provider.stop();
        return PositionResult{QGeoCoordinate(47, 8, 450), QStringLiteral("Vehicle GPS")};
    });
    provider.start(&transport);
    QVERIFY(transport.sentNmea.isEmpty());
    QVERIFY(provider.currentSource().isEmpty());
}

QGC_REGISTER_PORTABLE_TEST(NTRIPReentrancyTest, TestLabel::Unit)
#include "NTRIPReentrancyTest.moc"
