#include "RTCMStreamContentTest.h"

#include <QtTest/QTest>

#include "Protocols/Support/ProtocolTestPackets.h"
#include "RTCM/Support/RTCMTestFrames.h"
#include "RTCMStreamContent.h"

namespace {
using Observed = RTCMStreamContent::Observed;
using Problem = RTCMStreamContent::Problem;

constexpr qint64 WINDOW = RTCMStreamContent::WINDOW.count();
constexpr qint64 START = 100000;

/// A 1005 frame a base station sent, with a position on the Earth.
const QByteArray STATION_FRAME = QByteArray::fromHex("d300133ed000038a58d9493c872f34109d07d6af48205ad7f7");

/// The 1005 frame of a base station that has no position yet: its coordinates are zero.
QByteArray unsetStationFrame()
{
    QByteArray payload(19, '\0');
    payload[0] = static_cast<char>(1005 >> 4);
    payload[1] = static_cast<char>((1005 & 0x0f) << 4);
    return GPSTest::rtcmBytes(payload);
}

/// Records @a frames every second from START for @a seconds.
void recordEachSecond(RTCMStreamContent& content, const QList<QByteArray>& frames, int seconds, qint64 from = START)
{
    for (int second = 0; second < seconds; ++second) {
        for (const QByteArray& frame : frames) {
            content.record(frame, from + second * 1000);
        }
    }
}
}  // namespace

void RTCMStreamContentTest::_listsObservedConstellations()
{
    RTCMStreamContent content;
    QVERIFY(content.observed(START).isEmpty());

    // Whatever order they arrive in, the constellations are listed in GPSConstellation order. The older observation
    // messages name their constellation without a satellite count, and a message without satellites observes none.
    content.record(GPSTest::rtcmMsmFrame(1127, 11), START);
    content.record(GPSTest::rtcmMsmFrame(1097, 0), START);
    content.record(GPSTest::rtcmMsmFrame(1074, 9), START);
    content.record(GPSTest::rtcmMessage(1012, 20), START);
    content.record(STATION_FRAME, START);
    content.record(GPSTest::rtcmMessage(1230, 8), START);
    const QList<Observed> all{
        {GPSConstellation::GPS, 9}, {GPSConstellation::GLONASS, std::nullopt}, {GPSConstellation::BeiDou, 11}};
    QCOMPARE(content.observed(START), all);

    // A constellation whose observations stop is listed for the window after its last one.
    content.record(GPSTest::rtcmMsmFrame(1074, 8, 1000), START + 20000);
    QCOMPARE(content.observed(START + WINDOW - 1), QList<Observed>({{GPSConstellation::GPS, 8}, all.at(1), all.at(2)}));
    QCOMPARE(content.observed(START + WINDOW), QList<Observed>({{GPSConstellation::GPS, 8}}));
    QVERIFY(content.observed(START + 20000 + WINDOW).isEmpty());
}

void RTCMStreamContentTest::_countsTheSatellitesOfAnEpoch()
{
    RTCMStreamContent content;
    const auto gpsSatellites = [&content](qint64 now) {
        const QList<Observed> observed = content.observed(now);
        return observed.size() == 1 && observed.first().constellation == GPSConstellation::GPS
                   ? observed.first().satellites.value_or(-1)
                   : -1;
    };
    // An epoch that takes two messages has the satellites of both, and so does one sent as MSM4 and as MSM7.
    content.record(GPSTest::rtcmMsmFrame(1077, {.satelliteMask = 0xf800000000000000, .epochTime = 5000}), START);
    QCOMPARE(gpsSatellites(START), 5);
    content.record(GPSTest::rtcmMsmFrame(1077, {.satelliteMask = 0x0700000000000000, .epochTime = 5000}), START);
    QCOMPARE(gpsSatellites(START), 8);
    content.record(GPSTest::rtcmMsmFrame(1074, {.satelliteMask = 0xff00000000000000, .epochTime = 5000}), START);
    QCOMPARE(gpsSatellites(START), 8);

    // The next epoch counts its own satellites.
    content.record(GPSTest::rtcmMsmFrame(1077, {.satelliteMask = 0x00f0000000000000, .epochTime = 6000}), START + 1000);
    QCOMPARE(gpsSatellites(START + 1000), 4);

    // Without MSM messages for the window, the older messages leave the count unknown.
    content.record(GPSTest::rtcmMessage(1004, 20), START + 1000 + WINDOW);
    QCOMPARE(gpsSatellites(START + 1000 + WINDOW), -1);
    QCOMPARE(content.observed(START + 1000 + WINDOW).size(), 1);
}

void RTCMStreamContentTest::_problem_data()
{
    QTest::addColumn<QList<QByteArray>>("everySecond");
    QTest::addColumn<Problem>("problem");

    const QByteArray gps = GPSTest::rtcmMsmFrame(1077, 9);
    QTest::newRow("observations-and-station") << QList<QByteArray>{gps, STATION_FRAME} << Problem::None;
    QTest::newRow("older-observations-and-station")
        << QList<QByteArray>{GPSTest::rtcmMessage(1004, 20), STATION_FRAME} << Problem::None;
    QTest::newRow("station-only") << QList<QByteArray>{STATION_FRAME} << Problem::NoObservations;
    // Ephemerides, a receiver description and proprietary messages are no observations.
    QTest::newRow("no-observation-messages")
        << QList<QByteArray>{GPSTest::rtcmMessage(1019, 59), GPSTest::rtcmMessage(1033, 20),
                             GPSTest::rtcmMessage(4072, 20), STATION_FRAME}
        << Problem::NoObservations;
    QTest::newRow("observations-without-satellites")
        << QList<QByteArray>{GPSTest::rtcmMsmFrame(1077, 0), STATION_FRAME} << Problem::NoObservations;
    QTest::newRow("observations-only") << QList<QByteArray>{gps} << Problem::NoStationPosition;
    QTest::newRow("unset-station") << QList<QByteArray>{gps, unsetStationFrame()} << Problem::NoStationPosition;
}

void RTCMStreamContentTest::_problem()
{
    QFETCH(QList<QByteArray>, everySecond);
    QFETCH(Problem, problem);
    RTCMStreamContent content;
    QCOMPARE(content.problem(START), Problem::None);
    recordEachSecond(content, everySecond, 31);
    // A stream is given the window to send everything.
    QCOMPARE(content.problem(START + WINDOW - 1), Problem::None);
    QCOMPARE(content.problem(START + WINDOW), problem);

    // Whatever it lacked, a stream whose observations stop while its station messages go on has none after the
    // window.
    recordEachSecond(content, {STATION_FRAME}, 31, START + WINDOW + 1000);
    QCOMPARE(content.problem(START + 2 * WINDOW + 1000), Problem::NoObservations);
}

void RTCMStreamContentTest::_glonassWithoutBiases_data()
{
    QTest::addColumn<QList<QByteArray>>("everySecond");
    QTest::addColumn<bool>("withoutBiases");

    const QByteArray gps = GPSTest::rtcmMsmFrame(1077, 9);
    const QByteArray glonass = GPSTest::rtcmMsmFrame(1087, 6);
    const QByteArray biases = GPSTest::rtcmMessage(1230, 8);
    QTest::newRow("glonass") << QList<QByteArray>{gps, glonass, STATION_FRAME} << true;
    QTest::newRow("older-glonass") << QList<QByteArray>{GPSTest::rtcmMessage(1012, 20), STATION_FRAME} << true;
    QTest::newRow("glonass-with-biases") << QList<QByteArray>{gps, glonass, biases, STATION_FRAME} << false;
    QTest::newRow("no-glonass") << QList<QByteArray>{gps, STATION_FRAME} << false;
}

void RTCMStreamContentTest::_glonassWithoutBiases()
{
    QFETCH(QList<QByteArray>, everySecond);
    QFETCH(bool, withoutBiases);
    RTCMStreamContent content;
    recordEachSecond(content, everySecond, 31);
    // The biases come less often than the observations, so the stream is given the window to send them.
    QVERIFY(!content.glonassWithoutBiases(START + WINDOW - 1));
    QCOMPARE(content.glonassWithoutBiases(START + WINDOW), withoutBiases);
}

void RTCMStreamContentTest::_clearStartsAgain()
{
    RTCMStreamContent content;
    recordEachSecond(content, {GPSTest::rtcmMsmFrame(1087, 6)}, 31);
    QCOMPARE(content.problem(START + WINDOW), Problem::NoStationPosition);
    QVERIFY(content.glonassWithoutBiases(START + WINDOW));

    content.clear();
    QVERIFY(content.observed(START + WINDOW).isEmpty());
    QVERIFY(!content.glonassWithoutBiases(START + WINDOW));
    // The window of the next stream starts with its first frame.
    content.record(STATION_FRAME, START + WINDOW);
    QCOMPARE(content.problem(START + 2 * WINDOW - 1), Problem::None);
    QCOMPARE(content.problem(START + 2 * WINDOW), Problem::NoObservations);
}

UT_REGISTER_TEST_LIGHTWEIGHT(RTCMStreamContentTest, TestLabel::Unit)
