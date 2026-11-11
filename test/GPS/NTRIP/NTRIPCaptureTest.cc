#include "NTRIPCaptureTest.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

#include <QtCore/QByteArray>
#include <QtCore/QDateTime>
#include <QtCore/QString>

#include "NTRIP/fixtures/NTRIPCaptureExpectations.h"
#include "NTRIPHttpCodec.h"
#include "NTRIPSourceTable.h"
#include "Protocols/Support/GPSProtocolTestData.h"
#include "RTCMFramer.h"
#include "RTCMStationPosition.h"

namespace {

/// Chunk sizes every capture is delivered in; 0 delivers it whole.
constexpr int CHUNKS[] = {0, 1, 7};

QByteArray captureBytes(const char* name)
{
    return GPSTest::readFile(QStringLiteral(GPS_PROTOCOL_TEST_DIR "/../../NTRIP/fixtures/") + QLatin1String(name))
        .value_or(QByteArray());
}

template <typename Row, size_t Size>
std::vector<Row> rowsOf(const Row (&rows)[Size], const QByteArray& file)
{
    std::vector<Row> result;
    std::ranges::copy_if(rows, std::back_inserter(result), [&file](const Row& row) { return file == row.file; });
    return result;
}

struct Response
{
    QByteArray body;
    bool connected = false;
    bool complete = false;
    QString failure;
};

/// What @a decoder makes of @a wire delivered in chunks of @a chunk bytes, without the connection closing.
Response receive(NTRIPHttpDecoder& decoder, const QByteArray& wire, int chunk)
{
    Response response;
    const auto now = QDateTime::fromString(QStringLiteral("2026-09-18T12:00:00Z"), Qt::ISODate);
    const qsizetype step = chunk ? chunk : std::max<qsizetype>(wire.size(), 1);
    for (qsizetype offset = 0; offset < wire.size(); offset += step) {
        const auto result =
            decoder.feed(QByteArrayView(wire).sliced(offset, std::min(step, wire.size() - offset)), now);
        response.body += result.body;
        response.connected |= result.connected;
        response.complete |= result.complete;
        if (result.failure) {
            response.failure = result.failure->detail;
        }
    }
    return response;
}

bool near(double actual, double expected, double tolerance)
{
    return std::abs(actual - expected) < tolerance;
}

}  // namespace

void NTRIPCaptureTest::_sourceTable_data()
{
    QTest::addColumn<int>("chunk");
    for (const int chunk : CHUNKS) {
        QTest::addRow("chunk=%d", chunk) << chunk;
    }
}

void NTRIPCaptureTest::_sourceTable()
{
    QFETCH(int, chunk);
    // gpsd keeps the response with LF line endings. Its Content-Length is the body's size with the CRLF line endings
    // the caster sent, which this restores.
    QByteArray wire = captureBytes("gpsd-sapos-sourcetable.cap");
    QVERIFY(!wire.isEmpty() && !wire.contains('\r'));
    wire.replace("\n", "\r\n");

    NTRIPHttpDecoder decoder(NTRIPHttpPurpose::SourceTable);
    const Response response = receive(decoder, wire, chunk);
    QVERIFY2(response.failure.isEmpty(), qPrintable(response.failure));
    QVERIFY(response.connected);
    QVERIFY(response.complete);
    QCOMPARE(response.body.size(), NTRIPCapture::sourceTableContentLength);
    QVERIFY(ntripSourceTableComplete(response.body));
    QCOMPARE(response.body.count('\n') - 1, NTRIPCapture::sourceTableRows);

    NTRIPSourceTableModel model;
    model.parseSourceTable(QString::fromLatin1(response.body));
    QCOMPARE(model.count(), static_cast<int>(std::size(NTRIPCapture::mountpoints)));
    for (int row = 0; row < model.count(); ++row) {
        const auto& expected = NTRIPCapture::mountpoints[row];
        const auto value = [&model, row](int role) { return model.data(model.index(row, 0), role); };
        QCOMPARE(value(NTRIPSourceTableModel::MountpointRole).toString(), QLatin1String(expected.mountpoint));
        QCOMPARE(value(NTRIPSourceTableModel::FormatRole).toString(), QLatin1String(expected.format));
        QCOMPARE(value(NTRIPSourceTableModel::NavSystemRole).toString(), QLatin1String(expected.navSystem));
        QCOMPARE(value(NTRIPSourceTableModel::CountryRole).toString(), QLatin1String(expected.country));
        QCOMPARE(value(NTRIPSourceTableModel::BitrateRole).toInt(), expected.bitrate);
    }
    // The rows carry the coordinates the caster gave, which the model shows as distances only.
    const QStringList lines = QString::fromLatin1(response.body).split(QStringLiteral("\r\n"));
    size_t parsed = 0;
    for (const QString& line : lines) {
        const auto mountpoint = NTRIPMountpoint::fromSourceTableLine(line);
        if (!mountpoint) {
            continue;
        }
        QVERIFY(parsed < std::size(NTRIPCapture::mountpoints));
        const auto& expected = NTRIPCapture::mountpoints[parsed++];
        QVERIFY(near(mountpoint->latitude, expected.latitude, 1e-9));
        QVERIFY(near(mountpoint->longitude, expected.longitude, 1e-9));
    }
    QCOMPARE(parsed, std::size(NTRIPCapture::mountpoints));
}

void NTRIPCaptureTest::_casterStream_data()
{
    QTest::addColumn<QByteArray>("file");
    QTest::addColumn<int>("chunk");
    for (const char* file : {"gpsd-orgn-caster.cap", "pyrtcm-ntrip-uscl.cap"}) {
        for (const int chunk : CHUNKS) {
            QTest::addRow("%s chunk=%d", file, chunk) << QByteArray(file) << chunk;
        }
    }
}

void NTRIPCaptureTest::_casterStream()
{
    QFETCH(QByteArray, file);
    QFETCH(int, chunk);
    const QByteArray stream = captureBytes(file.constData());
    QVERIFY(!stream.isEmpty());
    // The captures start at the first correction; an NTRIP 1 caster sends this line before it.
    NTRIPHttpDecoder decoder;
    const Response response = receive(decoder, "ICY 200 OK\r\n\r\n" + stream, chunk);
    QVERIFY2(response.failure.isEmpty(), qPrintable(response.failure));
    QVERIFY(response.connected);
    QVERIFY(!response.complete);
    QCOMPARE(response.body, stream);

    const auto expected = rowsOf(NTRIPCapture::corrections, file);
    QVERIFY(!expected.empty());
    std::vector<RTCMDecodedFrame> frames;
    RTCMFrameDecoder framer;
    const qsizetype step = chunk ? chunk : response.body.size();
    for (qsizetype offset = 0; offset < response.body.size(); offset += step) {
        QVERIFY(framer.feed(QByteArrayView(response.body).sliced(offset, std::min(step, response.body.size() - offset)),
                            0, [&frames](const RTCMDecodedFrame& frame) {
                                frames.push_back(frame);
                                return true;
                            }));
    }
    QCOMPARE(frames.size(), expected.size());
    for (size_t index = 0; index < expected.size(); ++index) {
        const auto& row = expected[index];
        QVERIFY(frames[index].valid);
        QCOMPARE(frames[index].messageId, static_cast<int>(row.id));
        QCOMPARE(frames[index].data, stream.sliced(row.offset, row.size));
    }
}

void NTRIPCaptureTest::_stationPositions()
{
    QVERIFY(std::size(NTRIPCapture::stations) > 0);
    for (const auto& expected : NTRIPCapture::stations) {
        const QByteArray stream = captureBytes(expected.file);
        QVERIFY(!stream.isEmpty());
        const auto station = RTCMStationPosition::fromFrame(stream.sliced(expected.offset, expected.size));
        QVERIFY(station);
        // Messages 1005 and 1006 give coordinates to 0.1 mm.
        QVERIFY(near(station->position.latitudeDegrees, expected.latitude, 1e-9));
        QVERIFY(near(station->position.longitudeDegrees, expected.longitude, 1e-9));
        QVERIFY(near(station->position.altitudeMeters, expected.height, 1e-4));
        QCOMPARE(station->antennaHeightMeters.has_value(), expected.id == 1006);
        if (station->antennaHeightMeters) {
            QVERIFY(near(*station->antennaHeightMeters, expected.antennaHeight, 1e-9));
        }
    }
}

UT_REGISTER_TEST_LIGHTWEIGHT(NTRIPCaptureTest, TestLabel::Unit)
