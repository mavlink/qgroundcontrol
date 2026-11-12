#include "GPSReceiverSimulatorTest.h"

#include <chrono>
#include <cmath>
#include <memory>
#include <utility>
#include <variant>

#include <QtCore/QElapsedTimer>
#include <QtCore/QSet>
#include <QtNetwork/QTcpSocket>
#include <QtTest/QSignalSpy>

#include "Fixtures/RAIIFixtures.h"
#include "GPSReceiverWorker.h"
#include "GPSRecordingTransport.h"
#include "Protocols/Hardware/GPSReceiverSimulator.h"
#include "Protocols/Support/GPSBaseStationConfigs.h"
#include "Protocols/Support/GPSProtocolTestData.h"
#include "RTCMFramer.h"
#include "RTCMStationPosition.h"
#include "TCPGPSTransport.h"

using namespace std::chrono_literals;
using namespace GPSTest;

namespace {

/// A second of the simulated receiver's time, so that a survey of seconds takes milliseconds.
constexpr std::chrono::milliseconds SECOND_LENGTH{10};
/// The simulator reports positions in 1e-7 degrees and millimetres.
constexpr double DEGREES_TOLERANCE = 1e-6;
constexpr double METERS_TOLERANCE = 0.01;

bool near(const GPSEllipsoidPosition& actual, const GPSEllipsoidPosition& expected)
{
    return std::abs(actual.latitudeDegrees - expected.latitudeDegrees) < DEGREES_TOLERANCE &&
           std::abs(actual.longitudeDegrees - expected.longitudeDegrees) < DEGREES_TOLERANCE &&
           std::abs(actual.altitudeMeters - expected.altitudeMeters) < METERS_TOLERANCE;
}

}  // namespace

void GPSReceiverSimulatorTest::_workerConfiguresBase_data()
{
    using Receiver = UBXReceiverModel::Receiver;
    QTest::addColumn<GPSType>("type");
    QTest::addColumn<Receiver>("receiver");
    QTest::addColumn<GPSBaseStationConfig>("base");
    QTest::addColumn<QString>("identity");
    const GPSBaseStationConfig fixed = fixedBaseAt(-33.4488897, -70.6692655, 566.25, 0.5f);
    const GPSBaseStationConfig survey = surveyInBase(1.5, 3s);
    QTest::newRow("f9p-fixed") << GPSType::UBlox << Receiver::F9P << fixed << QStringLiteral("ZED-F9P HPG 1.32");
    QTest::newRow("f9p-survey") << GPSType::UBlox << Receiver::F9P << survey << QStringLiteral("ZED-F9P HPG 1.32");
    // As the settings connect by default: the family is detected first.
    QTest::newRow("f9p-detected") << GPSType::Automatic << Receiver::F9P << survey
                                  << QStringLiteral("ZED-F9P HPG 1.32");
    // Configured with the commands of the receivers before protocol 27.
    QTest::newRow("m8p-fixed") << GPSType::UBlox << Receiver::M8PBase << fixed << QStringLiteral("NEO-M8P-2 HPG 1.40");
    QTest::newRow("m8p-survey") << GPSType::UBlox << Receiver::M8PBase << survey
                                << QStringLiteral("NEO-M8P-2 HPG 1.40");
}

void GPSReceiverSimulatorTest::_workerConfiguresBase()
{
    QFETCH(GPSType, type);
    QFETCH(UBXReceiverModel::Receiver, receiver);
    QFETCH(GPSBaseStationConfig, base);
    QFETCH(QString, identity);
    const auto* const fixed = std::get_if<GPSBaseStationConfig::Fixed>(&base.mode);
    const auto* const survey = std::get_if<GPSBaseStationConfig::SurveyIn>(&base.mode);
    // A fixed base reports the position it was given, a surveying one where its antenna stands.
    const GPSEllipsoidPosition position = fixed ? fixed->position : GPSReceiverSimulator::ANTENNA;

    GPSReceiverSimulator simulator(
        {.capture = {}, .captureBaudRate = 0, .receiver = receiver, .secondLength = SECOND_LENGTH});
    QVERIFY2(simulator.listen(0), qPrintable(simulator.errorString()));
    const quint16 port = simulator.port();
    GPSReceiverWorker worker(
        [port](GPSCancelToken cancelToken) {
            return std::make_unique<TCPGPSTransport>(QStringLiteral("127.0.0.1"), port, std::move(cancelToken));
        },
        type, GPSReceiverConfig{.base = base});
    QSignalSpy ready(&worker, &GPSReceiverWorker::receiverReady);
    QSignalSpy errors(&worker, &GPSReceiverWorker::connectionError);
    QSignalSpy positions(&worker, &GPSReceiverWorker::positionUpdated);
    QSignalSpy satellites(&worker, &GPSReceiverWorker::satelliteInfoUpdated);
    QSignalSpy baseStatuses(&worker, &GPSReceiverWorker::baseStatusUpdated);
    QSignalSpy corrections(&worker, &GPSReceiverWorker::rtcmDataReceived);
    const auto failure = [&errors] {
        return errors.isEmpty() ? QByteArray("no error reported") : errors.first().at(1).toString().toUtf8();
    };
    worker.start();

    // The worker's driver configures the receiver across the socket.
    QTRY_VERIFY2_WITH_TIMEOUT(!ready.isEmpty() || !errors.isEmpty(), failure().constData(), TestTimeout::longMs());
    QVERIFY2(errors.isEmpty(), failure().constData());
    QCOMPARE(ready.first().first().toString(), identity);

    // The base sends its station position and the observations of four constellations once its position is valid,
    // and reports that position, its fix and its satellites. What the receiver sent before it was configured can
    // arrive first, so the wait is for the latest report of each kind.
    QSet<int> messages;
    GPSEllipsoidPosition station;
    qsizetype counted = 0;
    const QSet<int> expected{1005, 1077, 1087, 1097, 1127, 1230};
    const auto baseStatus = [&baseStatuses] {
        return baseStatuses.isEmpty() ? GPSBaseStatus{} : qvariant_cast<GPSBaseStatus>(baseStatuses.last().first());
    };
    const auto fix = [&positions] {
        return positions.isEmpty() ? GPSNavigationValues{}
                                   : qvariant_cast<GPSPositionReport>(positions.last().first()).navigation;
    };
    const auto fixPosition = [&fix]() -> GPSEllipsoidPosition {
        const GPSNavigationValues navigation = fix();
        return {navigation.latitudeDegrees, navigation.longitudeDegrees, navigation.altitudeEllipsoidMeters};
    };
    const auto settled = [&] {
        for (; counted < corrections.size(); ++counted) {
            const QByteArray frame = corrections.at(counted).first().toByteArray();
            messages.insert(RTCMFramer::frameMessageId(frame));
            if (const auto reported = RTCMStationPosition::fromFrame(frame)) {
                station = reported->position;
            }
        }
        return messages.contains(expected) && near(station, position) && baseStatus().state == GPSBaseState::Valid &&
               near(fixPosition(), position) && !satellites.isEmpty();
    };
    const auto progress = [&] {
        const GPSEllipsoidPosition reported = fixPosition();
        return QStringLiteral(
                   "%1; corrections %2 of station %3 %4 %5; base state %6; fix at %7 %8 %9; %10 "
                   "satellite reports")
            .arg(QString::fromUtf8(failure()))
            .arg(messages.size())
            .arg(station.latitudeDegrees, 0, 'f', 8)
            .arg(station.longitudeDegrees, 0, 'f', 8)
            .arg(station.altitudeMeters, 0, 'f', 4)
            .arg(static_cast<int>(baseStatus().state))
            .arg(reported.latitudeDegrees, 0, 'f', 8)
            .arg(reported.longitudeDegrees, 0, 'f', 8)
            .arg(reported.altitudeMeters, 0, 'f', 4)
            .arg(satellites.size())
            .toUtf8();
    };
    QTRY_VERIFY2_WITH_TIMEOUT(settled() || !errors.isEmpty(), progress().constData(), TestTimeout::longMs());
    QVERIFY2(errors.isEmpty(), failure().constData());

    worker.stop();
    QVERIFY(worker.wait(TestTimeout::mediumDuration()));
    QVERIFY2(errors.isEmpty(), failure().constData());
    QVERIFY(simulator.wireValid());
    QCOMPARE(simulator.connections(), 1);
    QCOMPARE(messages, expected);
    QCOMPARE(fix().fixType, GPSPositionReport::FixType::Fix3D);
    const auto sky = qvariant_cast<GPSSatelliteReport>(satellites.last().first());
    QCOMPARE(sky.inView.value_or(-1), 17);
    QCOMPARE(sky.used.value_or(-1), 14);

    const GPSBaseStatus status = baseStatus();
    QVERIFY(near(status.position, position));
    if (survey) {
        // The survey ran for the time asked for and ended at the accuracy asked for. How much of it the worker saw
        // depends on when it finished configuring, but what it saw counts up to the valid position.
        QCOMPARE(status.duration, survey->duration);
        QVERIFY(status.meanAccuracyMeters.has_value());
        QVERIFY2(std::abs(*status.meanAccuracyMeters - survey->accuracyMeters) < METERS_TOLERANCE,
                 qPrintable(QString::number(*status.meanAccuracyMeters)));
        GPSBaseStatus earlier;
        earlier.state = GPSBaseState::Active;
        for (const auto& arguments : std::as_const(baseStatuses)) {
            const auto later = qvariant_cast<GPSBaseStatus>(arguments.first());
            QVERIFY(later.duration >= earlier.duration);
            QVERIFY(later.state == GPSBaseState::Valid || earlier.state == GPSBaseState::Active);
            earlier = later;
        }
    }
}

void GPSReceiverSimulatorTest::_playsCaptureToEachClient_data()
{
    QTest::addColumn<QByteArray>("capture");
    QTest::addColumn<unsigned>("baudRate");
    QTest::addColumn<QByteArray>("played");
    QTest::addColumn<int>("minimumMs");
    const QByteArray answer = QByteArray::fromHex("b5620a0400000e34");
    const QByteArray fix = dataFile(FIXTURE_DIR, QStringLiteral("gga.nmea"));
    // What the receiver sent, at its recorded times; what the session sent and its baud rate are not played.
    QTest::newRow("recording") << QByteArray(GPSRecordingTransport::FILE_HEADER) + "\n0 BAUD 115200\n1000 TX " +
                                      answer.toHex() + "\n2000 RX " + answer.toHex() + "\n60000 RX " + fix.toHex() +
                                      '\n'
                               << 0U << answer + fix << 60;
    // Ten bits a byte.
    constexpr unsigned BAUD_RATE = 19200;
    QTest::newRow("raw-output-at-a-baud-rate")
        << fix << BAUD_RATE << fix << static_cast<int>(fix.size() * 10 * 1000 / BAUD_RATE) - 1;
}

void GPSReceiverSimulatorTest::_playsCaptureToEachClient()
{
    QFETCH(QByteArray, capture);
    QFETCH(unsigned, baudRate);
    QFETCH(QByteArray, played);
    QFETCH(int, minimumMs);
    QVERIFY(!played.isEmpty());
    TestFixtures::TempDirFixture directory;
    const QString file = directory.createFile(QStringLiteral("session.log"), capture);
    GPSReceiverSimulator simulator({.capture = file,
                                    .captureBaudRate = baudRate,
                                    .receiver = UBXReceiverModel::Receiver::F9P,
                                    .secondLength = SECOND_LENGTH});
    QVERIFY2(simulator.listen(0), qPrintable(simulator.errorString()));

    // Each client gets the capture from its start, and the connection closes at its end.
    for (int connection = 1; connection <= 2; ++connection) {
        QElapsedTimer elapsed;
        elapsed.start();
        QTcpSocket client;
        qint64 closedAfterMs = -1;
        (void) connect(&client, &QTcpSocket::disconnected, &client,
                       [&closedAfterMs, &elapsed] { closedAfterMs = elapsed.elapsed(); });
        client.connectToHost(QHostAddress(QHostAddress::LocalHost), simulator.port());
        QTRY_COMPARE_WITH_TIMEOUT(client.state(), QAbstractSocket::ConnectedState, TestTimeout::mediumMs());
        // A capture answers nothing.
        QCOMPARE(client.write("$PQTMVERNO*58\r\n"), 15);
        QTRY_VERIFY_WITH_TIMEOUT(closedAfterMs >= 0, TestTimeout::mediumMs());
        // Only a lower bound: the capture is never played ahead of its times.
        QVERIFY2(closedAfterMs >= minimumMs, qPrintable(QString::number(closedAfterMs)));
        QCOMPARE(client.readAll(), played);
        QCOMPARE(simulator.connections(), connection);
    }
    QCOMPARE(simulator.bytesSent(), 2 * played.size());

    GPSReceiverSimulator missing({.capture = file + QStringLiteral(".missing"),
                                  .captureBaudRate = 0,
                                  .receiver = UBXReceiverModel::Receiver::F9P,
                                  .secondLength = SECOND_LENGTH});
    QVERIFY(!missing.listen(0));
    QVERIFY(missing.errorString().contains(QStringLiteral("session.log.missing")));
}

UT_REGISTER_TEST_LIGHTWEIGHT(GPSReceiverSimulatorTest, TestLabel::Unit)
