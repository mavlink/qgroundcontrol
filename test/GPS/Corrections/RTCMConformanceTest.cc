#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtTest/QTest>

#include "RTCMFrameDecoder.h"
#include "RTCMFramer.h"
#include "RTCMParser.h"

namespace {
// Fixed CRCs keep the corpus independent of both implementations under test.
const QByteArray SHORT_FRAME = QByteArray::fromHex("d300023ed0a4e000");
const QByteArray LONG_FRAME = QByteArray::fromHex("d300134350000102030405060708090a0b0c0d0e0f10955042");

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
    void _sharedCorpus_data();
    void _sharedCorpus();
    void _strictValidation_data();
    void _strictValidation();
    void _fragmentReceiptAndFiltering();
};

void RTCMConformanceTest::_sharedCorpus_data()
{
    QTest::addColumn<QList<QByteArray>>("chunks");
    QTest::addColumn<QList<int>>("resetBeforeChunk");
    QTest::addColumn<QList<QByteArray>>("expectedFrames");
    QTest::addColumn<int>("expectedInvalidFrames");

    QTest::newRow("short-frame") << QList<QByteArray>{SHORT_FRAME} << QList<int>{} << QList<QByteArray>{SHORT_FRAME}
                                 << 0;
    QTest::newRow("split-header-payload-crc")
        << QList<QByteArray>{LONG_FRAME.first(1), LONG_FRAME.mid(1, 2), LONG_FRAME.mid(3, 7), LONG_FRAME.mid(10, 12),
                             LONG_FRAME.last(3)}
        << QList<int>{} << QList<QByteArray>{LONG_FRAME} << 0;

    QList<QByteArray> individualBytes;
    for (const char byte : LONG_FRAME) {
        individualBytes.append(QByteArray(1, byte));
    }
    QTest::newRow("byte-at-a-time") << individualBytes << QList<int>{} << QList<QByteArray>{LONG_FRAME} << 0;

    QByteArray badCrc = SHORT_FRAME;
    badCrc.back() = static_cast<char>(badCrc.back() ^ 1);
    QTest::newRow("bad-crc") << QList<QByteArray>{badCrc} << QList<int>{} << QList<QByteArray>{} << 1;
    QTest::newRow("recover-after-bad-crc")
        << QList<QByteArray>{badCrc + LONG_FRAME} << QList<int>{} << QList<QByteArray>{LONG_FRAME} << 1;
    QTest::newRow("skip-noise") << QList<QByteArray>{QByteArray::fromHex("00b562ff010203") + SHORT_FRAME}
                                << QList<int>{} << QList<QByteArray>{SHORT_FRAME} << 0;
    QTest::newRow("repeated-frames") << QList<QByteArray>{SHORT_FRAME + LONG_FRAME + SHORT_FRAME} << QList<int>{}
                                     << QList<QByteArray>{SHORT_FRAME, LONG_FRAME, SHORT_FRAME} << 0;
    QTest::newRow("reset-partial-header")
        << QList<QByteArray>{LONG_FRAME.first(2), SHORT_FRAME} << QList<int>{1} << QList<QByteArray>{SHORT_FRAME} << 0;
    QTest::newRow("reset-partial-payload")
        << QList<QByteArray>{LONG_FRAME.first(12), SHORT_FRAME} << QList<int>{1} << QList<QByteArray>{SHORT_FRAME} << 0;
    QTest::newRow("reset-partial-crc") << QList<QByteArray>{LONG_FRAME.first(LONG_FRAME.size() - 1), SHORT_FRAME}
                                       << QList<int>{1} << QList<QByteArray>{SHORT_FRAME} << 0;
    const QByteArray maximum = maximumFrame();
    QTest::newRow("maximum-payload-and-next-frame")
        << QList<QByteArray>{maximum.first(300), maximum.mid(300, 700), maximum.mid(1000) + SHORT_FRAME} << QList<int>{}
        << QList<QByteArray>{maximum, SHORT_FRAME} << 0;
}

void RTCMConformanceTest::_sharedCorpus()
{
    QFETCH(QList<QByteArray>, chunks);
    QFETCH(QList<int>, resetBeforeChunk);
    QFETCH(QList<QByteArray>, expectedFrames);
    QFETCH(int, expectedInvalidFrames);

    RTCMParser qgc;
    RTCMFramer px4;
    QList<QByteArray> qgcFrames;
    QList<QByteArray> px4Frames;
    QList<int> qgcIds;
    QList<int> px4Ids;
    int invalidFrames = 0;
    for (qsizetype chunkIndex = 0; chunkIndex < chunks.size(); ++chunkIndex) {
        if (resetBeforeChunk.contains(static_cast<int>(chunkIndex))) {
            qgc.reset();
            px4.reset();
        }
        for (const char rawByte : chunks[chunkIndex]) {
            const auto byte = static_cast<uint8_t>(rawByte);
            if (qgc.addByte(byte)) {
                // QGC exposes every complete frame so callers can count CRC failures.
                if (qgc.validateCrc()) {
                    qgcFrames.append(qgc.currentFrame());
                    qgcIds.append(qgc.messageId());
                    QCOMPARE(qgc.currentFrame().size(), qgc.messageLength() + 6);
                } else {
                    ++invalidFrames;
                }
                qgc.reset();
            }
            if (px4.addByte(byte) && px4.valid()) {
                // Native receivers publish only validated frames.
                px4Frames.append(QByteArray(reinterpret_cast<const char*>(px4.message()), px4.messageLength()));
                px4Ids.append(px4.messageId());
                px4.reset();
            }
        }
    }
    QCOMPARE(qgcFrames, expectedFrames);
    QCOMPARE(px4Frames, expectedFrames);
    QCOMPARE(qgcIds, px4Ids);
    QCOMPARE(invalidFrames, expectedInvalidFrames);
}

void RTCMConformanceTest::_strictValidation_data()
{
    QTest::addColumn<QByteArray>("candidate");
    QTest::newRow("reserved-header-bits") << QByteArray::fromHex("d304023ed0");
    QTest::newRow("one-byte-payload") << QByteArray::fromHex("d300013e");
}

void RTCMConformanceTest::_strictValidation()
{
    QFETCH(QByteArray, candidate);
    const auto crc = RTCMParser::crc24q(reinterpret_cast<const uint8_t*>(candidate.constData()), candidate.size());
    candidate.append(static_cast<char>(crc >> 16));
    candidate.append(static_cast<char>(crc >> 8));
    candidate.append(static_cast<char>(crc));
    QVERIFY(!RTCMParser::isValidFrame(candidate));
    RTCMFrameDecoder decoder;
    std::optional<RTCMFrameDecoder::Result> decoded;
    RTCMFramer native;
    bool nativeCompleted = false;
    for (const char byte : candidate) {
        if (const auto result = decoder.addByte(static_cast<uint8_t>(byte), 1000)) {
            decoded = result;
        }
        nativeCompleted |= native.addByte(static_cast<uint8_t>(byte));
    }
    QVERIFY(decoded);
    QVERIFY(!decoded->valid);
    QCOMPARE(decoded->data, candidate);
    // Both adapters use the same validation contract.
    QVERIFY(nativeCompleted);
    QVERIFY(!native.valid());
    const QByteArray nativeFrame(reinterpret_cast<const char*>(native.message()), native.messageLength());
    QVERIFY(!RTCMParser::isValidFrame(nativeFrame));
    for (const char byte : SHORT_FRAME) {
        decoded = decoder.addByte(static_cast<uint8_t>(byte), 8000);
    }
    QVERIFY(decoded && decoded->valid);
    QCOMPARE(decoded->receivedAtMs, qint64(8000));
}

void RTCMConformanceTest::_fragmentReceiptAndFiltering()
{
    RTCMFrameDecoder decoder;
    decoder.setWhitelist({1077});
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
    decoder.reset();
    decoder.setWhitelist({});
    for (const char byte : SHORT_FRAME) {
        decoded = decoder.addByte(static_cast<uint8_t>(byte), 9000);
    }
    QVERIFY(decoded && decoded->valid && !decoded->filtered);
    QCOMPARE(decoded->receivedAtMs, qint64(9000));
}

QTEST_GUILESS_MAIN(RTCMConformanceTest)

#include "RTCMConformanceTest.moc"
