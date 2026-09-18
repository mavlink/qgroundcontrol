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
    void bodyPublicationRetiresAttempt_data();
    void bodyPublicationRetiresAttempt();
    void receiptTimesAndEvidence();
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
    QCOMPARE(qvariant_cast<NTRIPError>(errors.first().first()), NTRIPError::SocketError);
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
    transport->_httpResponseBuf =
        (icy ? QByteArrayLiteral("ICY 200 OK\r\n") : QByteArrayLiteral("HTTP/1.1 200 OK\r\n\r\n")) +
        GpsTestHelpers::buildRtcmFrame(1005);
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
    transport->_handleHttpResponse();
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
    transport->_fail(NTRIPError::SocketError, QStringLiteral("test failure"));
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
    transport->_parseRtcm(frame + frame, 123);
    QCOMPARE(frames, 1);
    QCOMPARE(errors, 0);
    if (action == 2) {
        QVERIFY(transport->_connectTimeoutTimer.isActive());
        QVERIFY(!transport->_dataWatchdogTimer.isActive());
    }
}

void NTRIPReentrancyTest::receiptTimesAndEvidence()
{
    NTRIPHttpTransport transport(config(), {.whitelist = QStringLiteral("1005")});
    QSignalSpy observed(&transport, &NTRIPTransport::correctionFrameReceived);
    QList<RTCMFrameDecoder::Result> queued;
    connect(
        &transport, &NTRIPTransport::correctionFrameReceived, this,
        [&](const RTCMFrameDecoder::Result& frame) { queued.append(frame); }, Qt::QueuedConnection);
    const auto first = GpsTestHelpers::buildRtcmFrame(1005);
    const auto second = GpsTestHelpers::buildRtcmFrame(1077);
    transport._parseRtcm(first.first(1), 100);
    transport._parseRtcm(first.sliced(1) + second, 200);
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
