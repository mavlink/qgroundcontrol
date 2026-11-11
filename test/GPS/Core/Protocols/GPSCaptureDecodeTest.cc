#include "GPSCaptureDecodeTest.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <span>
#include <vector>

#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <QtCore/QStringList>

#include "GPSProtocolMath.h"
#include "GPSProtocolRuntime.h"
#include "GPSReceiverFamilies.h"
#include "Protocols/Support/GPSEventSummary.h"
#include "Protocols/Support/ReceiverBench.h"
#include "Protocols/fixtures/GPSCaptureExpectations.h"
#include "RTCMFramer.h"
#include "RTCMStationPosition.h"
#include "UBX/UBXFrame.h"
#include "UBX/UBXMessageSchema.h"

using namespace GPSTest;

namespace {

using Fix = GPSFixQuality;

/// Chunk sizes every capture is delivered in; 0 delivers it whole.
constexpr size_t CHUNKS[] = {0, 1, 7};

/// Longer than any family holds an epoch open, and shorter than its reports stay fresh.
constexpr std::chrono::milliseconds EPOCH_END{1000};

constexpr double KNOTS_TO_METERS_PER_SECOND = 1852.0 / 3600.0;

const char* SESSION = "pyubx2-zed-f9p-session.cap";
const char* REJECTED = "pyubx2-zed-f9p-rejected.cap";
const char* BASE = "gpsd-zed-f9t-base.cap";
const char* TRIMBLE = "gpsd-trimble-rtk.cap";
const char* ASHTECH = "gpsd-ac12-pashr.cap";

template <typename Row, size_t Size>
std::vector<Row> rowsOf(const Row (&rows)[Size], const char* file)
{
    std::vector<Row> result;
    std::ranges::copy_if(rows, std::back_inserter(result),
                         [file](const Row& row) { return std::strcmp(row.file, file) == 0; });
    return result;
}

/// A receiver family decoding a capture without a receiver: armed for the navigation output and corrections of a
/// receiver configured elsewhere where the family needs that.
struct CaptureDecoder
{
    CaptureDecoder(const GPSReceiverFamily& family, GPSTestClock& testClock)
        : clock(testClock)
        , runtime(family, makeDecoderOnlyIO(testClock))
    {
        runtime.armNavigationDecode({.corrections = true});
    }

    /// Every event of @a bytes delivered in chunks of @a chunk bytes, and of the epoch that ends after them.
    std::vector<GPSProtocolEvent> decode(std::span<const uint8_t> bytes, size_t chunk)
    {
        std::vector<GPSProtocolEvent> events;
        const auto take = [&events](const GPSEventBatch& batch) {
            events.insert(events.end(), batch.events.begin(), batch.events.end());
        };
        const size_t step = chunk ? chunk : std::max<size_t>(bytes.size(), 1);
        for (size_t offset = 0; offset < bytes.size(); offset += step) {
            take(runtime.decode(bytes.subspan(offset, std::min(step, bytes.size() - offset))));
        }
        clock.advanceBy(EPOCH_END);
        take(runtime.decode({}));
        return events;
    }

    GPSTestClock& clock;
    GPSProtocolRuntime runtime;
};

template <typename T>
std::vector<T> reports(const std::vector<GPSProtocolEvent>& events)
{
    std::vector<T> result;
    for (const auto& event : events) {
        if (const auto* report = std::get_if<T>(&event)) {
            result.push_back(*report);
        }
    }
    return result;
}

QString describe(const std::vector<GPSProtocolEvent>& events)
{
    QStringList lines;
    for (const auto& event : events) {
        if (!std::holds_alternative<GPSRTCMFrame>(event) && !std::holds_alternative<GPSDecodedSatellites>(event)) {
            lines.append(std::visit([](const auto& report) { return summary(report); }, event));
        }
    }
    return lines.join(u'\n');
}

const GPSReceiverFamily& familyNamed(const QString& name)
{
    return name == QLatin1String("ublox")     ? UBX::FAMILY
           : name == QLatin1String("trimble") ? Ashtech::FAMILY
                                              : Passive::FAMILY;
}

/// The capture of the current row.
QByteArray rowFile()
{
    return static_cast<const QString*>(QTest::qData("file", qMetaTypeId<QString>()))->toLatin1();
}

void addChunkRows(std::initializer_list<const char*> files, std::initializer_list<const char*> families)
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<QString>("family");
    QTest::addColumn<int>("chunk");
    for (const char* file : files) {
        for (const char* family : families) {
            for (const size_t chunk : CHUNKS) {
                QTest::addRow("%s %s chunk=%d", file, family, static_cast<int>(chunk))
                    << QString::fromLatin1(file) << QString::fromLatin1(family) << static_cast<int>(chunk);
            }
        }
    }
}

/// The events of the current row: its file decoded by its family in its chunk size. Empty when the file is missing.
std::vector<GPSProtocolEvent> decodeRow()
{
    const QByteArray file = rowFile();
    const auto family = *static_cast<const QString*>(QTest::qData("family", qMetaTypeId<QString>()));
    const auto chunk = *static_cast<const int*>(QTest::qData("chunk", qMetaTypeId<int>()));
    const auto bytes = fixtureBytes(file.constData());
    if (!bytes) {
        return {};
    }
    GPSTestClock clock(GPSTestClock::START_US);
    CaptureDecoder decoder(familyNamed(family), clock);
    return decoder.decode(*bytes, static_cast<size_t>(chunk));
}

/// NAV-PVT as the u-blox interface descriptions define it: without gnssFixOK there is no fix; fixType 3 is a 3D fix,
/// which carrSoln 1 makes a float and 2 a fixed carrier solution, and diffSoln a differential one; fixType 5 is the
/// time-only fix of a receiver in time mode, which holds its surveyed position.
Fix expectedFix(const GPSCapture::Epoch& epoch)
{
    if (!epoch.fixOk) {
        return Fix::NoFix;
    }
    if (epoch.fixType == 5) {
        return Fix::Fix3D;
    }
    if (epoch.fixType != 3) {
        return Fix::Unknown;
    }
    return epoch.carrierSolution == 1   ? Fix::RTKFloat
           : epoch.carrierSolution == 2 ? Fix::RTKFixed
           : epoch.differential         ? Fix::Differential
                                        : Fix::Fix3D;
}

/// GGA quality as NMEA 0183 defines it for the values the captures carry.
Fix expectedFix(const GPSCapture::Gga& gga)
{
    switch (gga.quality) {
        case 1:
            return Fix::Fix3D;
        case 2:
            return Fix::Differential;
        case 4:
            return Fix::RTKFixed;
        case 5:
            return Fix::RTKFloat;
        default:
            return Fix::Unknown;
    }
}

}  // namespace

void GPSCaptureDecodeTest::_ubloxPositions_data()
{
    addChunkRows({BASE, "gpsd-zed-f9p-rtk.cap", "gpsd-neo-m8p-rover.cap", SESSION}, {"ublox"});
}

void GPSCaptureDecodeTest::_ubloxPositions()
{
    const QByteArray name = rowFile();
    const auto events = decodeRow();
    const auto positions = reports<GPSDecodedPosition>(events);
    const auto expected = rowsOf(GPSCapture::epochs, name.constData());
    QVERIFY2(positions.size() == expected.size(), qPrintable(describe(events)));
    QVERIFY(!expected.empty());
    for (size_t index = 0; index < expected.size(); ++index) {
        const auto& actual = positions[index].navigation;
        const auto& epoch = expected[index];
        QVERIFY2(matches(actual.latitudeDegrees, epoch.latitude, 1e-9), qPrintable(describe(events)));
        QVERIFY(matches(actual.longitudeDegrees, epoch.longitude, 1e-9));
        QVERIFY(matches(actual.altitudeMslMeters, epoch.altitudeMsl));
        QVERIFY(matches(actual.altitudeEllipsoidMeters, epoch.altitudeEllipsoid));
        QCOMPARE(actual.fixType, expectedFix(epoch));
        QCOMPARE(actual.satellitesUsed, std::optional<uint8_t>(epoch.satellites));
    }
}

void GPSCaptureDecodeTest::_corrections_data()
{
    addChunkRows({BASE}, {"ublox", "passive"});
}

void GPSCaptureDecodeTest::_corrections()
{
    const QByteArray name = rowFile();
    const auto events = decodeRow();
    const auto frames = reports<GPSRTCMFrame>(events);
    const auto bytes = fixtureBytes(name.constData());
    QVERIFY(bytes);
    const auto expected = rowsOf(GPSCapture::corrections, name.constData());
    QCOMPARE(frames.size(), expected.size());
    QVERIFY(!expected.empty());
    for (size_t index = 0; index < expected.size(); ++index) {
        const auto& row = expected[index];
        const QByteArray recorded(reinterpret_cast<const char*>(bytes->data()) + row.offset, row.size);
        QCOMPARE(frames[index].bytes, recorded);
        QCOMPARE(RTCMFramer::frameMessageId(frames[index].bytes), row.id);
    }
}

void GPSCaptureDecodeTest::_stationPositions()
{
    QVERIFY(std::size(GPSCapture::stations) > 0);
    for (const auto& expected : GPSCapture::stations) {
        const auto bytes = fixtureBytes(expected.file);
        QVERIFY(bytes);
        const QByteArray frame(reinterpret_cast<const char*>(bytes->data()) + expected.offset, expected.size);
        const auto station = RTCMStationPosition::fromFrame(frame);
        QVERIFY(station);
        // Message 1005 gives coordinates to 0.1 mm.
        QVERIFY(matches(station->position.latitudeDegrees, expected.latitude, 1e-9));
        QVERIFY(matches(station->position.longitudeDegrees, expected.longitude, 1e-9));
        QVERIFY(matches(station->position.altitudeMeters, expected.height, 1e-4));
        QCOMPARE(station->antennaHeightMeters.has_value(), !std::isnan(expected.antennaHeight));
    }
}

void GPSCaptureDecodeTest::_surveyStatus_data()
{
    addChunkRows({SESSION}, {"ublox"});
}

void GPSCaptureDecodeTest::_surveyStatus()
{
    const auto events = decodeRow();
    const auto statuses = reports<GPSBaseStatus>(events);
    const auto expected = rowsOf(GPSCapture::surveys, SESSION);
    QVERIFY2(statuses.size() == expected.size(), qPrintable(describe(events)));
    QVERIFY(!expected.empty());
    for (size_t index = 0; index < expected.size(); ++index) {
        const auto& survey = expected[index];
        const GPSBaseState state = survey.active  ? GPSBaseState::Active
                                   : survey.valid ? GPSBaseState::Valid
                                                  : GPSBaseState::Inactive;
        QCOMPARE(statuses[index].state, state);
        QCOMPARE(statuses[index].duration.count(), survey.duration);
        QVERIFY(statuses[index].meanAccuracyMeters);
        QVERIFY(matches(*statuses[index].meanAccuracyMeters, survey.meanAccuracy));
    }
}

void GPSCaptureDecodeTest::_identity()
{
    QVERIFY(std::size(GPSCapture::versions) > 0);
    for (const auto& version : GPSCapture::versions) {
        const auto bytes = fixtureBytes(version.file);
        QVERIFY(bytes);
        const auto payload =
            std::span(*bytes).subspan(version.offset + UBX::HEADER_SIZE, version.size - UBX::FRAME_OVERHEAD);
        const auto decoded = Wire::decode<UBX::MonVer>(payload);
        QCOMPARE(QByteArray(reinterpret_cast<const char*>(decoded.swVersion)), QByteArray(version.software));
        QCOMPARE(QByteArray(reinterpret_cast<const char*>(decoded.hwVersion)), QByteArray(version.hardware));

        GPSTestClock clock(GPSTestClock::START_US);
        CaptureDecoder decoder(UBX::FAMILY, clock);
        (void) decoder.decode(*bytes, 0);
        const QString identity = decoder.runtime.identity();
        QVERIFY2(identity.contains(QLatin1String(version.module)), qPrintable(identity));
        QVERIFY2(identity.contains(QLatin1String(version.firmware)), qPrintable(identity));
    }
}

void GPSCaptureDecodeTest::_acknowledgements_data()
{
    QTest::addColumn<QString>("file");
    QTest::newRow(SESSION) << QString::fromLatin1(SESSION);
    QTest::newRow(REJECTED) << QString::fromLatin1(REJECTED);
}

void GPSCaptureDecodeTest::_acknowledgements()
{
    const QByteArray name = rowFile();
    const auto bytes = fixtureBytes(name.constData());
    QVERIFY(bytes);
    const auto expected = rowsOf(GPSCapture::acknowledgements, name.constData());
    QVERIFY(!expected.empty());
    UBX::FrameDecoder framer;
    size_t found = 0;
    for (size_t index = 0; index < bytes->size(); ++index) {
        const auto frame = framer.consume((*bytes)[index]);
        if (!frame || (frame->message != UBX::Msg::ACK_ACK.value() && frame->message != UBX::Msg::ACK_NAK.value())) {
            continue;
        }
        QVERIFY(found < expected.size());
        const auto& row = expected[found++];
        QCOMPARE(index + 1 - frame->bytes.size(), row.offset);
        QCOMPARE(frame->message == UBX::Msg::ACK_ACK.value(), row.acknowledged);
        const UBX::MessageId answered{static_cast<uint8_t>(row.messageClass), static_cast<uint8_t>(row.messageId)};
        QCOMPARE(Wire::decode<UBX::Ack>(frame->payload).msg, answered.value());
    }
    QCOMPARE(found, expected.size());
}

void GPSCaptureDecodeTest::_ggaPositions_data()
{
    addChunkRows({TRIMBLE}, {"trimble", "passive"});
}

void GPSCaptureDecodeTest::_ggaPositions()
{
    const auto events = decodeRow();
    const auto positions = reports<GPSDecodedPosition>(events);
    const auto expected = rowsOf(GPSCapture::ggas, TRIMBLE);
    QVERIFY2(positions.size() == expected.size(), qPrintable(describe(events)));
    QVERIFY(!expected.empty());
    for (size_t index = 0; index < expected.size(); ++index) {
        const auto& actual = positions[index].navigation;
        const auto& gga = expected[index];
        QVERIFY2(matches(actual.latitudeDegrees, gga.latitude, 1e-9), qPrintable(describe(events)));
        QVERIFY(matches(actual.longitudeDegrees, gga.longitude, 1e-9));
        QVERIFY(matches(actual.altitudeMslMeters, gga.altitude));
        QVERIFY(matches(actual.altitudeEllipsoidMeters, gga.altitude + gga.geoid));
        QVERIFY(matches(actual.horizontalDop, gga.hdop, 1e-5));
        QCOMPARE(actual.fixType, expectedFix(gga));
        QCOMPARE(actual.satellitesUsed, std::optional<uint8_t>(gga.satellites));
    }
}

void GPSCaptureDecodeTest::_ashtechPositions_data()
{
    addChunkRows({ASHTECH}, {"trimble"});
}

void GPSCaptureDecodeTest::_ashtechPositions()
{
    const auto events = decodeRow();
    const auto positions = reports<GPSDecodedPosition>(events);
    const auto expected = rowsOf(GPSCapture::ashtechPositions, ASHTECH);
    QVERIFY(!expected.empty());
    // GGA carries the position until the receiver's first $PASHR,POS, which follows the first GGA of this capture.
    const auto ggas = rowsOf(GPSCapture::ggas, ASHTECH);
    QVERIFY(!ggas.empty() && ggas.front().offset < expected.front().offset);
    QVERIFY(ggas.size() == 1 || ggas[1].offset > expected.front().offset);
    QVERIFY2(positions.size() == expected.size() + 1, qPrintable(describe(events)));
    const auto& first = positions.front().navigation;
    QVERIFY2(matches(first.latitudeDegrees, ggas.front().latitude, 1e-9), qPrintable(describe(events)));
    QVERIFY(matches(first.longitudeDegrees, ggas.front().longitude, 1e-9));
    QVERIFY(matches(first.altitudeMslMeters, ggas.front().altitude));
    for (size_t index = 0; index < expected.size(); ++index) {
        const auto& actual = positions[index + 1].navigation;
        const auto& position = expected[index];
        QVERIFY2(matches(actual.latitudeDegrees, position.latitude, 1e-9), qPrintable(describe(events)));
        QVERIFY(matches(actual.longitudeDegrees, position.longitude, 1e-9));
        QVERIFY(matches(actual.altitudeEllipsoidMeters, position.altitudeEllipsoid));
        QVERIFY(std::isnan(actual.altitudeMslMeters));
        QVERIFY(matches(actual.horizontalDop, position.hdop, 1e-5));
        QVERIFY(matches(actual.verticalDop, position.vdop, 1e-5));
        QVERIFY(matches(actual.courseRadians, position.course * GPSProtocolMath::DEG_TO_RAD, 1e-5));
        QVERIFY(matches(actual.speedMetersPerSecond, position.speedKnots * KNOTS_TO_METERS_PER_SECOND, 1e-5));
        QCOMPARE(actual.satellitesUsed, std::optional<uint8_t>(position.satellites));
        // Position mode 0 is a standalone fix.
        QCOMPARE(position.mode, 0u);
        QCOMPARE(actual.fixType, Fix::Fix3D);
    }
}

UT_REGISTER_TEST_LIGHTWEIGHT(GPSCaptureDecodeTest, TestLabel::Unit)
