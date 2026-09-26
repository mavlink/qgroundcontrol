#include <functional>
#include <memory>
#include <stop_token>

#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QElapsedTimer>
#include <QtCore/QFile>
#include <QtCore/QRegularExpression>

#include "Fixtures/RAIIFixtures.h"
#include "GPSDriver.h"
#include "GPSRecordingTransport.h"
#include "MemoryGPSTransport.h"
#include "Protocols/Support/ScriptedReceiver.h"
#include "Protocols/Support/UBXReceiverModel.h"
#include "RTCMFramer.h"
#include "ReplayGPSTransport.h"
#include "UnitTest.h"

using namespace std::chrono_literals;

namespace {
QByteArray capturePattern(int size)
{
    QByteArray bytes(size, Qt::Uninitialized);
    for (int index = 0; index < size; ++index) {
        bytes[index] = static_cast<char>((index * 17 + 3) & 0xff);
    }
    return bytes;
}

QByteArray captureContents(const QString& path)
{
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

/// Configures through one link, then streams through another, so a configured family can decode a capture.
class PhasedTransport final : public GPSTransport
{
public:
    explicit PhasedTransport(GPSTransport& configuration)
        : GPSTransport(configuration.stopToken())
        , _active(&configuration)
    {}

    void stream(GPSTransport& transport) { _active = &transport; }

    GPSOpenResult open() override { return _active->open(); }

    bool fatalError() const override { return _active->fatalError(); }

    unsigned fixedBaudrate() const override { return _active->fixedBaudrate(); }

    GPSReadResult read(uint8_t* buffer, int length, std::chrono::milliseconds timeout) override
    {
        return _active->read(buffer, length, timeout);
    }

    std::chrono::milliseconds configurationWriteTimeout() const override
    {
        return _active->configurationWriteTimeout();
    }

    bool setBaudrate(unsigned baudrate) override { return _active->setBaudrate(baudrate); }

protected:
    GPSWriteResult writeData(const uint8_t* buffer, int length, QDeadlineTimer deadline) override
    {
        return _active->write(buffer, length, deadline);
    }

private:
    GPSTransport* _active;
};

/// Decoded reports without host receipt times. Coordinates are compared through a hash, so a failure never prints
/// them.
struct DecodedEvents
{
    GPSDriverSinks sinks()
    {
        GPSDriverSinks result;
        result.onPosition = [this](const GPSPositionReport& report) {
            const auto& navigation = report.navigation;
            list.append(QStringLiteral("position fix=%1 used=%2 utc=%3 solution=%4")
                            .arg(static_cast<int>(navigation.fixType))
                            .arg(navigation.satellitesUsed ? int{*navigation.satellitesUsed} : -1)
                            .arg(navigation.utcTimeUs)
                            .arg(qHashMulti(0, navigation.latitudeDegrees, navigation.longitudeDegrees,
                                            navigation.altitudeMslMeters, navigation.altitudeEllipsoidMeters,
                                            navigation.horizontalAccuracyMeters)));
        };
        result.onSatelliteInfo = [this](const GPSSatelliteReport& report) {
            list.append(QStringLiteral("satellites view=%1 used=%2")
                            .arg(report.inView.value_or(-1))
                            .arg(report.used.value_or(-1)));
        };
        result.onRTCM = [this](const QByteArray& frame) {
            list.append(QStringLiteral("rtcm id=%1 bytes=%2 hash=%3")
                            .arg(RTCMFramer::frameMessageId(frame))
                            .arg(frame.size())
                            .arg(qHash(frame)));
        };
        result.onSurveyIn = [this](const GPSSurveyReport& report) {
            list.append(QStringLiteral("survey duration=%1 active=%2 valid=%3")
                            .arg(report.duration.count())
                            .arg(int{report.active})
                            .arg(int{report.valid}));
        };
        return result;
    }

    QStringList list;
};

GPSReceiverConfig replayConfig(GPSType type)
{
    if (type == GPSType::passive) {
        return {.role = GPSReceiverConfig::Role::Passive, .baudRate = 115200};
    }
    return {.base = {.mode = GPSBaseStationConfig::Fixed{
                         .position = {.latitudeDegrees = 47, .longitudeDegrees = 8, .altitudeMeters = 500},
                         .accuracyMeters = 1}}};
}

/// Configures @a type against a fresh scripted receiver, then decodes @a stream until @a spent.
QStringList decodeStream(GPSType type, std::stop_source& stop, GPSTransport& stream, const std::function<bool()>& spent)
{
    GPSTestClock clock;
    UBXReceiverModel model(UBXReceiverModel::Receiver::F9P, clock);
    ScriptedReceiver ubx(stop, model);
    MemoryGPSTransport silent({}, 64, stop.get_token());
    PhasedTransport link(type == GPSType::ublox ? static_cast<GPSTransport&>(ubx) : silent);
    DecodedEvents events;
    GPSDriver driver(type, link, replayConfig(type), events.sinks());
    if (!driver.configure()) {
        return {QStringLiteral("configuration failed: %1").arg(driver.configurationError())};
    }
    events.list.clear();
    if (stream.open().status != GPSOpenStatus::Opened) {
        return {QStringLiteral("stream did not open")};
    }
    link.stream(stream);
    for (int cycle = 0; cycle < 500 && !spent(); ++cycle) {
        if (driver.receiveOutcome(20ms).terminal() && !spent()) {
            events.list.append(QStringLiteral("receive failed"));
            break;
        }
    }
    return events.list;
}
}  // namespace

class ReplayGPSTransportTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _readsChunksUntilEnd();
    void _pacedAtBaudRate();
    void _roundTrip_data();
    void _roundTrip();
};

void ReplayGPSTransportTest::_readsChunksUntilEnd()
{
    TestFixtures::TempDirFixture directory;
    const QByteArray capture = capturePattern(1000);
    std::stop_source stop;
    ReplayGPSTransport replay(directory.createFile(QStringLiteral("capture.ubx"), capture), stop.get_token(), 0, 64);
    uint8_t buffer[100];
    QVERIFY(replay.fatalError());
    QCOMPARE(replay.read(buffer, sizeof(buffer), 0ms).status, GPSReadStatus::Closed);
    QCOMPARE(replay.open().status, GPSOpenStatus::Opened);
    QVERIFY(!replay.fatalError());

    // Configuration commands never fail on I/O; the capture holds the only replies.
    QVERIFY(replay.setBaudrate(38400));
    const QByteArray command = QByteArrayLiteral("\xb5\x62\x0a\x04\x00\x00\x0e\x34");
    const auto written = replay.write(reinterpret_cast<const uint8_t*>(command.constData()),
                                      static_cast<int>(command.size()), QDeadlineTimer(TestTimeout::shortDuration()));
    QCOMPARE(written.status, GPSWriteStatus::Completed);
    QCOMPARE(written.writtenBytes, static_cast<int>(command.size()));

    QByteArray received;
    while (!replay.finished()) {
        const auto result = replay.read(buffer, sizeof(buffer), 0ms);
        QCOMPARE(result.status, GPSReadStatus::Data);
        QVERIFY2(result.bytesRead > 0 && result.bytesRead <= 64, qPrintable(QString::number(result.bytesRead)));
        received.append(reinterpret_cast<const char*>(buffer), result.bytesRead);
    }
    QCOMPARE(received, capture);
    QCOMPARE(replay.bytesDelivered(), qint64{capture.size()});
    QVERIFY(replay.fatalError());

    // The end of the capture reads as an idle link that waits out each read.
    QElapsedTimer idle;
    idle.start();
    QCOMPARE(replay.read(buffer, sizeof(buffer), 30ms).status, GPSReadStatus::TimedOut);
    QVERIFY(idle.elapsed() >= 30);
    stop.request_stop();
    QCOMPARE(replay.read(buffer, sizeof(buffer), TestTimeout::shortDuration()).status, GPSReadStatus::Cancelled);

    ReplayGPSTransport missing(QDir(directory.path()).filePath(QStringLiteral("missing.ubx")), std::stop_token{});
    QCOMPARE(missing.open().status, GPSOpenStatus::Error);
}

void ReplayGPSTransportTest::_pacedAtBaudRate()
{
    TestFixtures::TempDirFixture directory;
    // 2400 baud carries 240 bytes a second, so the last of 120 bytes is due about 496 ms after open().
    const QByteArray capture = capturePattern(120);
    ReplayGPSTransport replay(directory.createFile(QStringLiteral("capture.nmea"), capture), std::stop_token{}, 2400);
    QElapsedTimer elapsed;
    elapsed.start();
    QCOMPARE(replay.open().status, GPSOpenStatus::Opened);
    uint8_t buffer[256];
    QByteArray received;
    int reads = 0;
    while (!replay.finished()) {
        const auto result = replay.read(buffer, sizeof(buffer), TestTimeout::shortDuration());
        QCOMPARE(result.status, GPSReadStatus::Data);
        received.append(reinterpret_cast<const char*>(buffer), result.bytesRead);
        ++reads;
    }
    QCOMPARE(received, capture);
    QVERIFY(reads > 1);
    QVERIFY2(elapsed.elapsed() >= 495, qPrintable(QString::number(elapsed.elapsed())));
}

void ReplayGPSTransportTest::_roundTrip_data()
{
    QTest::addColumn<GPSType>("type");
    QTest::addColumn<QStringList>("captures");
    QTest::addColumn<QStringList>("kinds");
    const QDir corpus(QStringLiteral(GPS_PROTOCOL_CORPUS_DIR));
    const QDir fixtures(QStringLiteral(GPS_FIXTURE_DIR));
    QTest::newRow("passive") << GPSType::passive
                             << QStringList{corpus.filePath(QStringLiteral("gga.nmea")),
                                            corpus.filePath(QStringLiteral("synthetic-gga.nmea")),
                                            fixtures.filePath(QStringLiteral("mixed.gps"))}
                             << QStringList{QStringLiteral("position"), QStringLiteral("rtcm")};
    QTest::newRow("ublox") << GPSType::ublox
                           << QStringList{corpus.filePath(QStringLiteral("synthetic-integrity.ubx")),
                                          corpus.filePath(QStringLiteral("upstream-nav-sat.ubx")),
                                          fixtures.filePath(QStringLiteral("navigation.ubx")),
                                          fixtures.filePath(QStringLiteral("mixed.gps"))}
                           << QStringList{QStringLiteral("position"), QStringLiteral("satellites"),
                                          QStringLiteral("rtcm")};
}

void ReplayGPSTransportTest::_roundTrip()
{
    QFETCH(GPSType, type);
    QFETCH(QStringList, captures);
    QFETCH(QStringList, kinds);
    QByteArray capture;
    for (const QString& path : captures) {
        const QByteArray bytes = captureContents(path);
        QVERIFY2(!bytes.isEmpty(), qPrintable(path));
        capture += bytes;
    }
    TestFixtures::TempDirFixture directory;
    const auto files = GPSRecordingTransport::sessionFiles(directory.path(), type, QDateTime::currentDateTime());
    std::stop_source stop;

    QStringList direct;
    {
        auto source = std::make_unique<MemoryGPSTransport>(capture, 61, stop.get_token());
        const MemoryGPSTransport* const input = source.get();
        GPSRecordingTransport recorder(std::move(source), files);
        direct = decodeStream(type, stop, recorder, [input] { return input->drained(); });
    }
    QCOMPARE(captureContents(files.received), capture);
    for (const QString& kind : kinds) {
        QVERIFY2(!direct.filter(QRegularExpression(QStringLiteral("^%1 ").arg(kind))).isEmpty(), qPrintable(kind));
    }

    ReplayGPSTransport replay(files.received, stop.get_token(), 0, 53);
    const QStringList replayed = decodeStream(type, stop, replay, [&replay] { return replay.finished(); });
    QCOMPARE(replayed, direct);
}

UT_REGISTER_TEST(ReplayGPSTransportTest, TestLabel::Unit)

#include "ReplayGPSTransportTest.moc"
