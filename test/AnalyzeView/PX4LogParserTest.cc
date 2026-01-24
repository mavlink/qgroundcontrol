#include "PX4LogParserTest.h"
#include "PX4LogParser.h"
#include "GeoTagWorker.h"

#include <QtCore/QRandomGenerator>
#include <QtTest/QTest>

// ============================================================================
// Edge Case Tests
// ============================================================================

void PX4LogParserTest::_emptyBufferTest()
{
    const QByteArray empty;
    QList<GeoTagWorker::CameraFeedbackPacket> cameraFeedback;

    // Should handle empty buffer gracefully (returns true with empty results)
    const bool result = PX4LogParser::getTagsFromLog(empty, cameraFeedback);
    QVERIFY(result);
    QVERIFY(cameraFeedback.isEmpty());
}

void PX4LogParserTest::_invalidDataTest()
{
    // Plain text - not a PX4 log
    const QByteArray textData("This is not a PX4 log file at all");
    QList<GeoTagWorker::CameraFeedbackPacket> cameraFeedback;

    const bool result = PX4LogParser::getTagsFromLog(textData, cameraFeedback);
    QVERIFY(result);
    QVERIFY(cameraFeedback.isEmpty());
}

void PX4LogParserTest::_truncatedHeaderTest()
{
    // PX4 log header bytes but truncated
    const QByteArray truncated("\xA3\x95\x00", 3);
    QList<GeoTagWorker::CameraFeedbackPacket> cameraFeedback;

    const bool result = PX4LogParser::getTagsFromLog(truncated, cameraFeedback);
    QVERIFY(result);
    QVERIFY(cameraFeedback.isEmpty());
}

void PX4LogParserTest::_headerOnlyTest()
{
    // GPOS header header (defines message format)
    const QByteArray gposHeaderHeader("\xA3\x95\x80\x10\x00", 5);
    QList<GeoTagWorker::CameraFeedbackPacket> cameraFeedback;

    const bool result = PX4LogParser::getTagsFromLog(gposHeaderHeader, cameraFeedback);
    QVERIFY(result);
    QVERIFY(cameraFeedback.isEmpty());

    // Trigger header header
    const QByteArray triggerHeaderHeader("\xA3\x95\x80\x37\x00", 5);
    cameraFeedback.clear();

    const bool result2 = PX4LogParser::getTagsFromLog(triggerHeaderHeader, cameraFeedback);
    QVERIFY(result2);
    QVERIFY(cameraFeedback.isEmpty());
}

void PX4LogParserTest::_randomDataTest()
{
    // Random binary data - should not crash
    QByteArray randomData(1024, '\0');
    QRandomGenerator *gen = QRandomGenerator::global();
    for (int i = 0; i < randomData.size(); ++i) {
        randomData[i] = static_cast<char>(gen->bounded(256));
    }

    QList<GeoTagWorker::CameraFeedbackPacket> cameraFeedback;
    const bool result = PX4LogParser::getTagsFromLog(randomData, cameraFeedback);

    // Parser should handle random data without crashing
    // Result may be true (with empty or partial results) or false
    Q_UNUSED(result);
}
