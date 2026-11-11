#include "RTCMConformanceTest.h"

#include <cmath>
#include <limits>
#include <optional>
#include <span>

#include <QtCore/QByteArray>
#include <QtCore/QEvent>
#include <QtCore/QList>
#include <QtCore/QPair>
#include <QtTest/QAbstractItemModelTester>
#include <QtTest/QSignalSpy>

#include "Checksums.h"
#include "Protocols/Support/GPSProtocolTestBase.h"
#include "Protocols/Support/ProtocolTestPackets.h"
#include "Protocols/fixtures/GPSFixtureExpectations.h"
#include "RTCMFramer.h"
#include "RTCMMessageCountModel.h"
#include "RTCMStationPosition.h"

namespace {
// Fixed CRCs keep expectations independent of the framing implementation.
const QByteArray SHORT_FRAME = QByteArray::fromHex("d300023ed0a4e000");
const QByteArray LONG_FRAME = QByteArray::fromHex("d300134350000102030405060708090a0b0c0d0e0f10955042");
const QByteArray PREAMBLE_PAYLOAD = QByteArray::fromHex("d3000a4350d3d3d3d3d3d3d3d3ecb2b5");
const QByteArray NESTED_PAYLOAD = QByteArray::fromHex("d3000a4350d300023ed0a4e0000d07f0");

QByteArray maximumFrame()
{
    QByteArray frame = QByteArray::fromHex("d303ff4ce0");
    for (int i = 0; i < 1021; ++i) {
        frame.append(static_cast<char>(i & 0xff));
    }
    return frame + QByteArray::fromHex("eebabe");
}

/// An RTCM 1005 frame, or 1006 with @a antennaHeight, whose ECEF coordinates and antenna height count 0.1 mm. Every
/// other field has all its bits set, so a coordinate read one bit off is wrong.
QByteArray stationFrame(uint16_t messageId, int64_t x, int64_t y, int64_t z,
                        std::optional<uint16_t> antennaHeight = std::nullopt)
{
    QByteArray payload;
    int size = 0;
    const auto put = [&payload, &size](uint64_t value, int width) {
        for (int bit = width - 1; bit >= 0; --bit, ++size) {
            if (size % 8 == 0) {
                payload.append('\0');
            }
            payload.back() = static_cast<char>(payload.back() | (((value >> bit) & 1) << (7 - size % 8)));
        }
    };
    put(messageId, 12);
    put(0xfff, 12);  // station ID
    put(0x3f, 6);    // ITRF realization year
    put(0xf, 4);     // GPS, GLONASS, Galileo and reference-station indicators
    put(static_cast<uint64_t>(x), 38);
    put(0x3, 2);     // single receiver oscillator indicator and a reserved bit
    put(static_cast<uint64_t>(y), 38);
    put(0x3, 2);     // quarter cycle indicator
    put(static_cast<uint64_t>(z), 38);
    if (antennaHeight) {
        put(*antennaHeight, 16);
    }
    return GPSTest::rtcmBytes(payload);
}

/// The 1005 frame of the recording fixtures/mixed.gps, which a u-blox base station sent; empty when unreadable.
QByteArray recordedStationFrame()
{
    const auto recording = GPSTest::fixtureBytes("mixed.gps");
    const auto& slice = GPSFixture::corrections[0];
    if (!recording || slice.id != 1005 || recording->size() < slice.offset + slice.size) {
        return {};
    }
    return GPSTest::toByteArray(std::span<const uint8_t>(*recording).subspan(slice.offset, slice.size));
}
}  // namespace

void RTCMConformanceTest::_crc24q_data()
{
    QTest::addColumn<QByteArray>("bytes");
    QTest::addColumn<quint32>("expected");
    QTest::newRow("empty") << QByteArray() << 0U;
    QTest::newRow("zero") << QByteArray::fromHex("00") << 0U;
    QTest::newRow("one") << QByteArray::fromHex("01") << 0x864cfbU;
    QTest::newRow("standard-check") << QByteArrayLiteral("123456789") << 0xcde703U;
    QTest::newRow("frame-prefix") << SHORT_FRAME.first(5) << 0xa4e000U;
    QTest::newRow("frame-residue") << SHORT_FRAME << 0U;
    QTest::newRow("maximum-prefix") << maximumFrame().chopped(3) << 0xeebabeU;
}

void RTCMConformanceTest::_crc24q()
{
    QFETCH(QByteArray, bytes);
    QFETCH(quint32, expected);
    QCOMPARE(QGC::crc24q({reinterpret_cast<const uint8_t*>(bytes.constData()), static_cast<size_t>(bytes.size())}),
             expected);
}

void RTCMConformanceTest::_sharedCorpus_data()
{
    QTest::addColumn<QList<QByteArray>>("chunks");
    QTest::addColumn<QList<int>>("resetBeforeChunk");
    QTest::addColumn<QList<QByteArray>>("expectedFrames");
    QTest::addColumn<QList<qint64>>("expectedReceiptTimes");
    QTest::addColumn<QList<QByteArray>>("expectedRejected");
    QTest::addColumn<QList<qint64>>("expectedRejectionTimes");
    QTest::addColumn<qint64>("chunkIntervalMs");

    const auto addCase = [](const char* name, QList<QByteArray> chunks, QList<QByteArray> frames,
                            QList<qint64> receiptTimes, QList<QByteArray> rejected = {},
                            QList<qint64> rejectionTimes = {}, QList<int> resetBefore = {},
                            qint64 chunkIntervalMs = 1000) {
        QTest::newRow(name) << chunks << resetBefore << frames << receiptTimes << rejected << rejectionTimes
                            << chunkIntervalMs;
    };
    addCase("short-frame", {SHORT_FRAME}, {SHORT_FRAME}, {1000});
    addCase(
        "split-header-payload-crc",
        {LONG_FRAME.first(1), LONG_FRAME.mid(1, 2), LONG_FRAME.mid(3, 7), LONG_FRAME.mid(10, 12), LONG_FRAME.last(3)},
        {LONG_FRAME}, {1000});

    QList<QByteArray> individualBytes;
    for (const char byte : LONG_FRAME) {
        individualBytes.append(QByteArray(1, byte));
    }
    addCase("byte-at-a-time", individualBytes, {LONG_FRAME}, {1000});

    QByteArray badCrc = SHORT_FRAME;
    badCrc.back() = static_cast<char>(badCrc.back() ^ 1);
    addCase("bad-crc", {badCrc}, {}, {}, {badCrc}, {1000});
    addCase("recover-after-bad-crc", {badCrc + LONG_FRAME}, {LONG_FRAME}, {1000}, {badCrc}, {1000});
    addCase("skip-noise", {QByteArray::fromHex("00b562ff010203") + SHORT_FRAME}, {SHORT_FRAME}, {1000});
    addCase("repeated-frames", {SHORT_FRAME + LONG_FRAME + SHORT_FRAME}, {SHORT_FRAME, LONG_FRAME, SHORT_FRAME},
            {1000, 1000, 1000});
    addCase("reset-partial-header", {LONG_FRAME.first(2), SHORT_FRAME}, {SHORT_FRAME}, {2000}, {}, {}, {1});
    addCase("reset-partial-payload", {LONG_FRAME.first(12), SHORT_FRAME}, {SHORT_FRAME}, {2000}, {}, {}, {1});
    addCase("reset-partial-crc", {LONG_FRAME.first(LONG_FRAME.size() - 1), SHORT_FRAME}, {SHORT_FRAME}, {2000}, {}, {},
            {1});
    const QByteArray maximum = maximumFrame();
    addCase("maximum-payload-and-next-frame",
            {maximum.first(300), maximum.mid(300, 700), maximum.mid(1000) + SHORT_FRAME}, {maximum, SHORT_FRAME},
            {1000, 3000});
    addCase("stray-preamble", {QByteArray::fromHex("d3") + SHORT_FRAME}, {SHORT_FRAME}, {1000},
            {QByteArray::fromHex("d3d3")}, {1000});
    addCase("stray-preamble-across-chunks", {QByteArray::fromHex("d3"), SHORT_FRAME.first(1), SHORT_FRAME.mid(1)},
            {SHORT_FRAME}, {2000}, {QByteArray::fromHex("d3d3")}, {1000});
    addCase("invalid-nonzero-advertised-length", {QByteArray::fromHex("d3fcff"), SHORT_FRAME}, {SHORT_FRAME}, {2000},
            {QByteArray::fromHex("d3fc")}, {1000});
    for (int bit = 2; bit < 8; ++bit) {
        QByteArray header = QByteArray::fromHex("d303ff");
        header[1] = static_cast<char>((1 << bit) | 3);
        const QByteArray name = "reserved-bit-" + QByteArray::number(bit);
        addCase(name.constData(), {header, SHORT_FRAME}, {SHORT_FRAME}, {2000}, {header.first(2)}, {1000});
    }
    addCase("zero-length", {QByteArray::fromHex("d30000"), SHORT_FRAME}, {SHORT_FRAME}, {2000},
            {QByteArray::fromHex("d30000")}, {1000});
    addCase("one-byte-length", {QByteArray::fromHex("d30001"), SHORT_FRAME}, {SHORT_FRAME}, {2000},
            {QByteArray::fromHex("d30001")}, {1000});
    addCase("preambles-in-valid-payload", {PREAMBLE_PAYLOAD}, {PREAMBLE_PAYLOAD}, {1000});
    addCase("complete-frame-in-valid-payload", {NESTED_PAYLOAD.first(13), NESTED_PAYLOAD.mid(13)}, {NESTED_PAYLOAD},
            {1000});

    QByteArray badLength = SHORT_FRAME;
    badLength[2] = 3;
    addCase("corrupt-length-consumes-next-preamble", {badLength, LONG_FRAME.first(1), LONG_FRAME.mid(1)}, {LONG_FRAME},
            {2000}, {badLength + LONG_FRAME.first(1)}, {1000});
    addCase("interrupted-crc", {SHORT_FRAME.first(7), LONG_FRAME}, {LONG_FRAME}, {2000},
            {SHORT_FRAME.first(7) + LONG_FRAME.first(1)}, {1000});
    const QByteArray interrupted = LONG_FRAME.first(5) + SHORT_FRAME + SHORT_FRAME + QByteArray(4, '\0');
    addCase("recover-multiple-buffered-frames",
            {LONG_FRAME.first(5), SHORT_FRAME.first(3), SHORT_FRAME.mid(3), SHORT_FRAME, QByteArray(4, '\0')},
            {SHORT_FRAME, SHORT_FRAME}, {2000, 4000}, {interrupted}, {1000});
    const QByteArray interruptedWithFalseHeader =
        QByteArray::fromHex("d300134350d300023ed0a4e000d303ffd300023ed0a4e00000");
    individualBytes.clear();
    for (const char byte : interruptedWithFalseHeader) {
        individualBytes.append(QByteArray(1, byte));
    }
    addCase("recover-past-false-header-between-frames", individualBytes, {SHORT_FRAME, SHORT_FRAME}, {1005, 1016},
            {interruptedWithFalseHeader}, {1000}, {}, 1);
    addCase("recover-past-false-header-across-chunks",
            {interruptedWithFalseHeader.first(5), interruptedWithFalseHeader.mid(5, 11),
             interruptedWithFalseHeader.mid(16)},
            {SHORT_FRAME, SHORT_FRAME}, {2000, 3000}, {interruptedWithFalseHeader}, {1000});
    const QByteArray interruptedWithOuterPrefix = LONG_FRAME.first(5) + SHORT_FRAME + NESTED_PAYLOAD.first(12);
    addCase("preserve-outer-beyond-rejected-evidence",
            {LONG_FRAME.first(5), SHORT_FRAME, NESTED_PAYLOAD.first(12), NESTED_PAYLOAD.mid(12)},
            {SHORT_FRAME, NESTED_PAYLOAD}, {2000, 3000}, {interruptedWithOuterPrefix}, {1000});
    addCase("reset-retained-recovery", {interruptedWithOuterPrefix, SHORT_FRAME + NESTED_PAYLOAD},
            {SHORT_FRAME, SHORT_FRAME, NESTED_PAYLOAD}, {1000, 2000, 2000}, {interruptedWithOuterPrefix}, {1000}, {1});
    const QByteArray rejectedSuffixes = LONG_FRAME.first(5) + badCrc + QByteArray(12, '\0');
    addCase("discard-known-bad-suffixes", {rejectedSuffixes, SHORT_FRAME}, {SHORT_FRAME}, {2000}, {rejectedSuffixes},
            {1000});

    QByteArray corruptMaximum(RTCMFramer::MAX_FRAME_SIZE, '\0');
    corruptMaximum.replace(0, 3, QByteArray::fromHex("d303ff"));
    corruptMaximum.replace(16, 3, QByteArray::fromHex("d303ff"));
    corruptMaximum.replace(100, SHORT_FRAME.size(), SHORT_FRAME);
    corruptMaximum.replace(200, 3, QByteArray::fromHex("d303ff"));
    corruptMaximum.replace(1000, SHORT_FRAME.size(), SHORT_FRAME);
    addCase("maximum-recovery-skips-false-preamble",
            {QByteArray(RTCMFramer::MAX_FRAME_SIZE + 17, '\0'), corruptMaximum.first(100), corruptMaximum.mid(100, 1),
             corruptMaximum.mid(101)},
            {SHORT_FRAME, SHORT_FRAME}, {3000, 4000}, {corruptMaximum}, {2000});
}

void RTCMConformanceTest::_sharedCorpus()
{
    QFETCH(QList<QByteArray>, chunks);
    QFETCH(QList<int>, resetBeforeChunk);
    QFETCH(QList<QByteArray>, expectedFrames);
    QFETCH(QList<qint64>, expectedReceiptTimes);
    QFETCH(QList<QByteArray>, expectedRejected);
    QFETCH(QList<qint64>, expectedRejectionTimes);
    QFETCH(qint64, chunkIntervalMs);

    RTCMFrameDecoder decoder;
    RTCMFramer framer;
    QList<QByteArray> decodedFrames;
    QList<QByteArray> framedFrames;
    QList<int> decodedIds;
    QList<int> framedIds;
    QList<QByteArray> decodedRejected;
    QList<QByteArray> framedRejected;
    QList<qint64> receiptTimes;
    QList<qint64> rejectionTimes;
    for (qsizetype chunkIndex = 0; chunkIndex < chunks.size(); ++chunkIndex) {
        if (resetBeforeChunk.contains(static_cast<int>(chunkIndex))) {
            decoder.reset();
            framer.reset();
        }
        for (const char rawByte : chunks[chunkIndex]) {
            const auto byte = static_cast<uint8_t>(rawByte);
            int resultCount = 0;
            for (auto decoded = decoder.addByte(byte, 1000 + chunkIndex * chunkIntervalMs); decoded;
                 decoded = decoder.nextFrame()) {
                QVERIFY(++resultCount <= RTCMFramer::MAX_FRAME_SIZE);
                QVERIFY(!decoded->filtered);
                if (decoded->valid) {
                    decodedFrames.append(decoded->data);
                    decodedIds.append(decoded->messageId);
                    receiptTimes.append(decoded->receivedAtMs);
                } else {
                    decodedRejected.append(decoded->data);
                    rejectionTimes.append(decoded->receivedAtMs);
                }
            }
            for (bool complete = framer.addByte(byte); complete; complete = framer.nextFrame()) {
                const auto view = framer.frame();
                const QByteArray candidate(reinterpret_cast<const char*>(view.data()),
                                           static_cast<qsizetype>(view.size()));
                if (framer.valid()) {
                    framedFrames.append(candidate);
                    framedIds.append(framer.messageId());
                } else {
                    framedRejected.append(candidate);
                }
            }
            QVERIFY(framer.bufferedSize() <= RTCMFramer::MAX_FRAME_SIZE);
        }
    }
    QCOMPARE(decodedFrames, expectedFrames);
    QCOMPARE(framedFrames, expectedFrames);
    QCOMPARE(decodedIds, framedIds);
    QCOMPARE(decodedRejected, expectedRejected);
    QCOMPARE(framedRejected, expectedRejected);
    QCOMPARE(receiptTimes, expectedReceiptTimes);
    QCOMPARE(rejectionTimes, expectedRejectionTimes);
}

void RTCMConformanceTest::_strictValidation_data()
{
    QTest::addColumn<QByteArray>("candidate");
    QTest::newRow("reserved-header-bits") << QByteArray::fromHex("d304023ed0");
    QTest::newRow("one-byte-payload") << QByteArray::fromHex("d300013e");
    QTest::newRow("zero-byte-payload") << QByteArray::fromHex("d30000");
}

void RTCMConformanceTest::_strictValidation()
{
    QFETCH(QByteArray, candidate);
    const auto crc =
        QGC::crc24q({reinterpret_cast<const uint8_t*>(candidate.constData()), static_cast<size_t>(candidate.size())});
    candidate.append(static_cast<char>(crc >> 16));
    candidate.append(static_cast<char>(crc >> 8));
    candidate.append(static_cast<char>(crc));
    QVERIFY(!RTCMFramer::isValidFrame(candidate));
    QVERIFY(!RTCMFramer::isValidFrame(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(candidate.constData()),
                                                               static_cast<size_t>(candidate.size()))));
}

void RTCMConformanceTest::_frameFields_data()
{
    QTest::addColumn<int>("messageId");
    QTest::addColumn<int>("extraPayload");
    QTest::newRow("minimum-id") << 0 << 0;
    QTest::newRow("maximum-id") << 4095 << 0;
    QTest::newRow("maximum-length") << 1230 << 1021;
}

void RTCMConformanceTest::_frameFields()
{
    QFETCH(int, messageId);
    QFETCH(int, extraPayload);
    const auto frame = GPSTest::rtcmMessage(static_cast<uint16_t>(messageId), extraPayload);
    RTCMFramer framer;
    QVERIFY(!framer.hasPartialFrame());
    QVERIFY(!framer.nextFrame());
    // A completed frame stays readable until the next byte, which starts the following frame.
    for (int pass = 0; pass < 2; ++pass) {
        for (qsizetype index = 0; index < frame.size() - 1; ++index) {
            QVERIFY(!framer.addByte(static_cast<uint8_t>(frame[index])));
            QVERIFY(framer.hasPartialFrame());
            QCOMPARE(framer.bufferedSize(), index + 1);
        }
        QVERIFY(framer.addByte(static_cast<uint8_t>(frame.back())));
        QVERIFY(framer.valid());
        QCOMPARE(framer.messageId(), messageId);
        QCOMPARE(framer.payloadLength(), extraPayload + 2);
        const auto view = framer.frame();
        QCOMPARE(QByteArray(reinterpret_cast<const char*>(view.data()), static_cast<qsizetype>(view.size())), frame);
    }
    QVERIFY(RTCMFramer::isValidFrame(frame));

    framer.reset();
    QVERIFY(framer.frame().empty());
    QCOMPARE(framer.bufferedSize(), 0);
    QCOMPARE(framer.payloadLength(), 0);
    QCOMPARE(framer.messageId(), 0);
    QVERIFY(!framer.valid());
}

void RTCMConformanceTest::_fragmentReceiptAndFiltering()
{
    RTCMFrameDecoder decoder;
    decoder.setWhitelist({1077, 1077});
    QVERIFY(!decoder.addByte(RTCMFramer::PREAMBLE, 500));
    const auto rejected = decoder.addByte(0x04, 500);
    QVERIFY(rejected && !rejected->valid && !rejected->filtered);
    QVERIFY(!decoder.nextFrame());
    for (const char byte : SHORT_FRAME.first(4)) {
        QVERIFY(!decoder.addByte(static_cast<uint8_t>(byte), 1000));
    }
    std::optional<RTCMDecodedFrame> decoded;
    for (const char byte : SHORT_FRAME.sliced(4)) {
        decoded = decoder.addByte(static_cast<uint8_t>(byte), 8000);
    }
    QVERIFY(decoded && decoded->valid && decoded->filtered);
    QCOMPARE(decoded->messageId, 1005);
    QCOMPARE(decoded->receivedAtMs, qint64(1000));
    QCOMPARE(decoded->data, SHORT_FRAME);
    for (const char byte : LONG_FRAME) {
        decoded = decoder.addByte(static_cast<uint8_t>(byte), 8500);
    }
    QVERIFY(decoded && decoded->valid && !decoded->filtered);
    QCOMPARE(decoded->messageId, 1077);
    decoder.reset();
    for (const char byte : SHORT_FRAME) {
        decoded = decoder.addByte(static_cast<uint8_t>(byte), 9000);
    }
    QVERIFY(decoded && decoded->valid && decoded->filtered);
    QCOMPARE(decoded->receivedAtMs, qint64(9000));
    decoder.setWhitelist({});
    for (const char byte : SHORT_FRAME) {
        decoded = decoder.addByte(static_cast<uint8_t>(byte), 10000);
    }
    QVERIFY(decoded && decoded->valid && !decoded->filtered);
    QCOMPARE(decoded->receivedAtMs, qint64(10000));
}

void RTCMConformanceTest::_repeatedMalformedPreambles()
{
    RTCMFrameDecoder decoder;
    for (int index = 0; index < RTCMFramer::MAX_FRAME_SIZE * 4; ++index) {
        const auto result = decoder.addByte(RTCMFramer::PREAMBLE, index + 1000);
        if (!index) {
            QVERIFY(!result);
        } else {
            QVERIFY(result);
            QVERIFY(!result->valid);
            QVERIFY(!result->filtered);
            QCOMPARE(result->data, QByteArray::fromHex("d3d3"));
            QCOMPARE(result->receivedAtMs, index + 999);
        }
        QVERIFY(!decoder.nextFrame());
    }
    decoder.reset();
    std::optional<RTCMDecodedFrame> result;
    for (const char byte : SHORT_FRAME) {
        result = decoder.addByte(static_cast<uint8_t>(byte), 9000);
    }
    QVERIFY(result && result->valid);
    QCOMPARE(result->receivedAtMs, qint64(9000));
}

void RTCMConformanceTest::_queuedResultDelivery()
{
    QObject receiver;
    std::optional<RTCMDecodedFrame> received;
    connect(
        this, &RTCMConformanceTest::decoded, &receiver,
        [&received](const RTCMDecodedFrame& result) { received = result; }, Qt::QueuedConnection);
    emit decoded({SHORT_FRAME, 1005, 1234, true, true});
    QVERIFY(!received);
    QCoreApplication::sendPostedEvents(&receiver, QEvent::MetaCall);
    QVERIFY(received);
    QCOMPARE(received->data, SHORT_FRAME);
    QCOMPARE(received->messageId, 1005);
    QCOMPARE(received->receivedAtMs, qint64(1234));
    QVERIFY(received->valid);
    QVERIFY(received->filtered);
}

void RTCMConformanceTest::_messageCountsChangeInPlace()
{
    using Rows = QList<QPair<int, quint64>>;
    RTCMMessageCountModel model;
    QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::QtTest);
    QSignalSpy inserted(&model, &QAbstractItemModel::rowsInserted);
    QSignalSpy changed(&model, &QAbstractItemModel::dataChanged);
    QSignalSpy resets(&model, &QAbstractItemModel::modelReset);
    const auto rows = [&model]() {
        Rows result;
        for (int row = 0; row < model.rowCount(); ++row) {
            const QModelIndex index = model.index(row);
            result.append({index.data(RTCMMessageCountModel::MessageIdRole).toInt(),
                           index.data(RTCMMessageCountModel::CountRole).toULongLong()});
        }
        return result;
    };

    // Each new ID inserts one row at its ascending position; ID 0 counts unidentified frames.
    QList<int> insertedAt;
    for (const int messageId : {1077, 1005, 1230, 0}) {
        model.record(messageId);
        insertedAt.append(inserted.last().at(1).toInt());
    }
    QCOMPARE(insertedAt, QList<int>({0, 0, 2, 0}));
    QCOMPARE(rows(), Rows({{0, 1}, {1005, 1}, {1077, 1}, {1230, 1}}));
    QVERIFY(changed.isEmpty());

    // A repeated ID changes only the count of its row.
    model.record(1077);
    model.record(1077);
    QCOMPARE(rows(), Rows({{0, 1}, {1005, 1}, {1077, 3}, {1230, 1}}));
    QCOMPARE(inserted.size(), 4);
    QCOMPARE(changed.size(), 2);
    QCOMPARE(changed.last().at(0).toModelIndex(), model.index(2));
    QCOMPARE(changed.last().at(1).toModelIndex(), model.index(2));
    QCOMPARE(changed.last().at(2).value<QList<int>>(), QList<int>{RTCMMessageCountModel::CountRole});
    QVERIFY(resets.isEmpty());

    model.clear();
    QCOMPARE(model.rowCount(), 0);
    QCOMPARE(resets.size(), 1);
    model.clear();
    QCOMPARE(resets.size(), 1);
}

void RTCMConformanceTest::_stationPosition_data()
{
    QTest::addColumn<QByteArray>("frame");
    QTest::addColumn<bool>("decoded");
    QTest::addColumn<double>("latitude");
    QTest::addColumn<double>("longitude");
    QTest::addColumn<double>("height");
    QTest::addColumn<double>("antennaHeight");

    // The geodetic positions and their ECEF coordinates come from the WGS84 closed-form conversion, worked outside
    // QGC; between them the coordinates cover both signs of each axis.
    constexpr double NONE = std::numeric_limits<double>::quiet_NaN();
    const auto valid = [](const char* name, const QByteArray& frame, double latitude, double longitude, double height,
                          double antennaHeight) {
        QTest::newRow(name) << frame << true << latitude << longitude << height << antennaHeight;
    };
    valid("1005", stationFrame(1005, 42775513130, 6427646826, 46721688253), 47.3977419, 8.5455938, 488.2, NONE);
    valid("1005-negative-y-z", stationFrame(1005, 17635692337, -50273165025, -34959199469), -33.4488897, -70.6692655,
          570.0, NONE);
    valid("1006-negative-x-z", stationFrame(1006, -46460925734, 25532295142, -35344068039, 0x8001), -33.8688197,
          151.2092955, 58.5, 3.2769);
    valid("recorded-1005", recordedStationFrame(), 32.0658325449, 34.7738190354, 72.1342, NONE);

    const auto invalid = [](const char* name, const QByteArray& frame) {
        QTest::newRow(name) << frame << false << 0.0 << 0.0 << 0.0 << 0.0;
    };
    invalid("other-message", stationFrame(1007, 42775513130, 6427646826, 46721688253));
    invalid("1005-of-1006-length", stationFrame(1005, 42775513130, 6427646826, 46721688253, 0));
    invalid("1006-of-1005-length", stationFrame(1006, 42775513130, 6427646826, 46721688253));
    QByteArray badCrc = stationFrame(1005, 42775513130, 6427646826, 46721688253);
    badCrc.back() = static_cast<char>(badCrc.back() ^ 1);
    invalid("bad-crc", badCrc);
    invalid("unset-base", stationFrame(1005, 0, 0, 0));
    invalid("empty", {});
}

void RTCMConformanceTest::_stationPosition()
{
    QFETCH(QByteArray, frame);
    QFETCH(bool, decoded);
    QFETCH(double, latitude);
    QFETCH(double, longitude);
    QFETCH(double, height);
    QFETCH(double, antennaHeight);
    const auto station = RTCMStationPosition::fromFrame(frame);
    QCOMPARE(station.has_value(), decoded);
    if (!decoded) {
        return;
    }
    // The coordinates are in 0.1 mm, which is below 1e-8 degrees.
    QVERIFY2(GPSTest::matches(station->position.latitudeDegrees, latitude, 1e-8),
             qPrintable(QString::number(station->position.latitudeDegrees, 'f', 10)));
    QVERIFY2(GPSTest::matches(station->position.longitudeDegrees, longitude, 1e-8),
             qPrintable(QString::number(station->position.longitudeDegrees, 'f', 10)));
    QVERIFY2(GPSTest::matches(station->position.altitudeMeters, height, 1e-3),
             qPrintable(QString::number(station->position.altitudeMeters, 'f', 5)));
    QCOMPARE(station->antennaHeightMeters.has_value(), !std::isnan(antennaHeight));
    if (station->antennaHeightMeters) {
        QCOMPARE(*station->antennaHeightMeters, antennaHeight);
    }
}

UT_REGISTER_TEST_LIGHTWEIGHT(RTCMConformanceTest, TestLabel::Unit)
