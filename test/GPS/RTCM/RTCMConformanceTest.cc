#include <QtCore/QByteArray>
#include <QtCore/QEvent>
#include <QtCore/QList>
#include <QtTest/QTest>

#include "RTCMFrame.h"
#include "RTCMFrameDecoder.h"
#include "RTCMFramer.h"
#include "RTCMMavlinkPacket.h"

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
}  // namespace

class RTCMConformanceTest : public QObject
{
    Q_OBJECT

private slots:
    void _crc24q_data();
    void _crc24q();
    void _sharedCorpus_data();
    void _sharedCorpus();
    void _strictValidation_data();
    void _strictValidation();
    void _fragmentReceiptAndFiltering();
    void _repeatedMalformedPreambles();
    void _queuedResultDelivery();
    void _packetization_data();
    void _packetization();

signals:
    void decoded(RTCMFrameDecoder::Result result);
};

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
    QCOMPARE(
        RTCMFramer::crc24q({reinterpret_cast<const uint8_t*>(bytes.constData()), static_cast<size_t>(bytes.size())}),
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
    const auto crc = RTCMFramer::crc24q(
        {reinterpret_cast<const uint8_t*>(candidate.constData()), static_cast<size_t>(candidate.size())});
    candidate.append(static_cast<char>(crc >> 16));
    candidate.append(static_cast<char>(crc >> 8));
    candidate.append(static_cast<char>(crc));
    QVERIFY(!RTCM::isValidFrame(candidate));
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
    std::optional<RTCMFrameDecoder::Result> decoded;
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
    std::optional<RTCMFrameDecoder::Result> result;
    for (const char byte : SHORT_FRAME) {
        result = decoder.addByte(static_cast<uint8_t>(byte), 9000);
    }
    QVERIFY(result && result->valid);
    QCOMPARE(result->receivedAtMs, qint64(9000));
}

void RTCMConformanceTest::_queuedResultDelivery()
{
    QObject receiver;
    std::optional<RTCMFrameDecoder::Result> received;
    connect(
        this, &RTCMConformanceTest::decoded, &receiver,
        [&received](const RTCMFrameDecoder::Result& result) { received = result; }, Qt::QueuedConnection);
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

void RTCMConformanceTest::_packetization_data()
{
    QTest::addColumn<int>("size");
    QTest::addColumn<int>("sequence");
    QTest::addColumn<int>("packetCount");
    QTest::addColumn<bool>("fragmented");
    QTest::addColumn<int>("nextSequence");

    QTest::newRow("empty") << 0 << 31 << 0 << false << 31;
    QTest::newRow("single-byte") << 1 << 31 << 1 << false << 32;
    QTest::newRow("exact-single-fragment") << 180 << 31 << 1 << false << 32;
    QTest::newRow("split-tail") << 181 << 31 << 2 << true << 32;
    QTest::newRow("two-full-and-terminator") << 360 << 31 << 3 << true << 32;
    QTest::newRow("three-full-and-terminator") << 540 << 31 << 4 << true << 32;
    QTest::newRow("four-with-tail") << 719 << 31 << 4 << true << 32;
    QTest::newRow("four-full-no-terminator") << 720 << 31 << 4 << true << 32;
    QTest::newRow("oversized-unfragmented-stream") << 721 << 31 << 5 << false << 36;
    QTest::newRow("maximum-rtcm-frame") << 1029 << 31 << 6 << false << 37;
    QTest::newRow("sequence-counter-wrap") << 181 << 255 << 2 << true << 0;
    QTest::newRow("oversized-counter-wrap") << 721 << 255 << 5 << false << 4;
}

void RTCMConformanceTest::_packetization()
{
    QFETCH(int, size);
    QFETCH(int, sequence);
    QFETCH(int, packetCount);
    QFETCH(bool, fragmented);
    QFETCH(int, nextSequence);
    QByteArray bytes(size, Qt::Uninitialized);
    for (int index = 0; index < size; ++index) {
        bytes[index] = static_cast<char>(index & 0xff);
    }
    const auto packed = RTCMMavlinkPacket::pack(bytes, static_cast<uint8_t>(sequence));
    QCOMPARE(packed.packets.size(), packetCount);
    QCOMPARE(packed.nextSequenceId, nextSequence);
    QByteArray assembled;
    for (qsizetype index = 0; index < packed.packets.size(); ++index) {
        const auto& packet = packed.packets[index];
        QCOMPARE(bool(packet.flags & 1), fragmented);
        QCOMPARE((packet.flags >> 1) & 3, fragmented ? index : 0);
        QCOMPARE(packet.flags >> 3, (sequence + (fragmented ? 0 : index)) & 31);
        QVERIFY(packet.data.size() <= RTCMMavlinkPacket::kFragmentLen);
        QCOMPARE(packet.data.size(), qMin(qsizetype(size) - assembled.size(), RTCMMavlinkPacket::kFragmentLen));
        assembled.append(packet.data);
    }
    QCOMPARE(assembled, bytes);
}

QTEST_GUILESS_MAIN(RTCMConformanceTest)

#include "RTCMConformanceTest.moc"
