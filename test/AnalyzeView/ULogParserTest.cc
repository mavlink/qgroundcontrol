#include "ULogParserTest.h"
#include "TestHelpers.h"
#include "QtTestExtensions.h"
#include "ULogParser.h"
#include "GeoTagWorker.h"

#include <QtCore/QFile>
#include <QtTest/QTest>

// ============================================================================
// Setup and Helpers
// ============================================================================

QByteArray ULogParserTest::_loadSampleULog()
{
    QFile file(":/unittest/SampleULog.ulg");
    if (!file.open(QIODevice::ReadOnly)) {
        return QByteArray();
    }
    const QByteArray data = file.readAll();
    file.close();
    return data;
}

namespace {

bool compareFeedbackPackets(const GeoTagWorker::CameraFeedbackPacket &a, const GeoTagWorker::CameraFeedbackPacket &b)
{
    constexpr double kEpsilon = 1e-6;
    return TestHelpers::fuzzyCompare(a.timestamp, b.timestamp, kEpsilon) &&
           TestHelpers::fuzzyCompare(a.timestampUTC, b.timestampUTC, kEpsilon) &&
           a.imageSequence == b.imageSequence &&
           TestHelpers::fuzzyCompare(a.latitude, b.latitude, kEpsilon) &&
           TestHelpers::fuzzyCompare(a.longitude, b.longitude, kEpsilon) &&
           TestHelpers::fuzzyCompare(a.altitude, b.altitude, kEpsilon) &&
           TestHelpers::fuzzyCompare(a.groundDistance, b.groundDistance, kEpsilon) &&
           a.captureResult == b.captureResult;
}

} // namespace

void ULogParserTest::init()
{
    UnitTest::init();

    _logBuffer = _loadSampleULog();
    QVERIFY2(!_logBuffer.isEmpty(), "Failed to load test log SampleULog.ulg");
}

// ============================================================================
// Non-Streamed Parser Tests
// ============================================================================

void ULogParserTest::_getTagsFromLogTest()
{
    QList<GeoTagWorker::CameraFeedbackPacket> cameraFeedback;
    QString errorMessage;
    QVERIFY(ULogParser::getTagsFromLog(_logBuffer, cameraFeedback, errorMessage));
    QGC_VERIFY_EMPTY(errorMessage);
    QGC_VERIFY_NOT_EMPTY(cameraFeedback);

    const GeoTagWorker::CameraFeedbackPacket firstCameraFeedback = cameraFeedback.constFirst();
    QCOMPARE_NE(firstCameraFeedback.imageSequence, 0);
}

// ============================================================================
// Streamed Parser Tests
// ============================================================================

void ULogParserTest::_getTagsFromLogStreamedTest()
{
    QList<GeoTagWorker::CameraFeedbackPacket> cameraFeedback;
    QString errorMessage;
    QVERIFY(ULogParser::getTagsFromLogStreamed(_logBuffer, cameraFeedback, errorMessage));
    QGC_VERIFY_EMPTY(errorMessage);
    QGC_VERIFY_NOT_EMPTY(cameraFeedback);

    const GeoTagWorker::CameraFeedbackPacket firstCameraFeedback = cameraFeedback.constFirst();
    QCOMPARE_NE(firstCameraFeedback.imageSequence, 0);
}

// ============================================================================
// Edge Case Tests
// ============================================================================

void ULogParserTest::_emptyBufferTest()
{
    const QByteArray empty;
    QList<GeoTagWorker::CameraFeedbackPacket> cameraFeedback;
    QString errorMessage;

    // Empty buffer should fail gracefully
    const bool result = ULogParser::getTagsFromLog(empty, cameraFeedback, errorMessage);
    QVERIFY(!result);
    QGC_VERIFY_NOT_EMPTY(errorMessage);

    // Streamed parser should also fail
    cameraFeedback.clear();
    errorMessage.clear();
    const bool resultStreamed = ULogParser::getTagsFromLogStreamed(empty, cameraFeedback, errorMessage);
    QVERIFY(!resultStreamed);
    QGC_VERIFY_NOT_EMPTY(errorMessage);
}

void ULogParserTest::_invalidDataTest()
{
    // Plain text - not a ULog file
    const QByteArray textData("This is not a ULog file at all");
    QList<GeoTagWorker::CameraFeedbackPacket> cameraFeedback;
    QString errorMessage;

    const bool result = ULogParser::getTagsFromLog(textData, cameraFeedback, errorMessage);
    QVERIFY(!result);
    QGC_VERIFY_NOT_EMPTY(errorMessage);

    // Streamed parser should also fail
    cameraFeedback.clear();
    errorMessage.clear();
    const bool resultStreamed = ULogParser::getTagsFromLogStreamed(textData, cameraFeedback, errorMessage);
    QVERIFY(!resultStreamed);
    QGC_VERIFY_NOT_EMPTY(errorMessage);
}

void ULogParserTest::_truncatedHeaderTest()
{
    // ULog magic bytes but truncated (ULog starts with "ULog" magic)
    const QByteArray truncated("ULog");
    QList<GeoTagWorker::CameraFeedbackPacket> cameraFeedback;
    QString errorMessage;

    const bool result = ULogParser::getTagsFromLog(truncated, cameraFeedback, errorMessage);
    QVERIFY(!result);
    QGC_VERIFY_NOT_EMPTY(errorMessage);

    // First few bytes of valid log (truncated header)
    const QByteArray partialLog = _logBuffer.left(50);
    cameraFeedback.clear();
    errorMessage.clear();
    const bool partialResult = ULogParser::getTagsFromLog(partialLog, cameraFeedback, errorMessage);
    QVERIFY(!partialResult);
    QGC_VERIFY_NOT_EMPTY(errorMessage);
}

// ============================================================================
// Comparison Tests
// ============================================================================

void ULogParserTest::_compareStreamedAndNonStreamedTest()
{
    QList<GeoTagWorker::CameraFeedbackPacket> feedbackNonStreamed;
    QList<GeoTagWorker::CameraFeedbackPacket> feedbackStreamed;
    QString errorNonStreamed;
    QString errorStreamed;

    QVERIFY(ULogParser::getTagsFromLog(_logBuffer, feedbackNonStreamed, errorNonStreamed));
    QVERIFY(ULogParser::getTagsFromLogStreamed(_logBuffer, feedbackStreamed, errorStreamed));

    QCOMPARE_EQ(feedbackStreamed.size(), feedbackNonStreamed.size());

    for (int i = 0; i < feedbackStreamed.size(); ++i) {
        QVERIFY2(compareFeedbackPackets(feedbackStreamed[i], feedbackNonStreamed[i]),
                 qPrintable(QStringLiteral("Packet %1 differs between streamed and non-streamed").arg(i)));
    }
}

// ============================================================================
// Benchmark Tests
// ============================================================================

void ULogParserTest::_benchmarkNonStreamed()
{
    QList<GeoTagWorker::CameraFeedbackPacket> cameraFeedback;
    QString errorMessage;

    QBENCHMARK {
        cameraFeedback.clear();
        errorMessage.clear();
        (void)ULogParser::getTagsFromLog(_logBuffer, cameraFeedback, errorMessage);
    }

    QGC_VERIFY_NOT_EMPTY(cameraFeedback);
}

void ULogParserTest::_benchmarkStreamed()
{
    QList<GeoTagWorker::CameraFeedbackPacket> cameraFeedback;
    QString errorMessage;

    QBENCHMARK {
        cameraFeedback.clear();
        errorMessage.clear();
        (void)ULogParser::getTagsFromLogStreamed(_logBuffer, cameraFeedback, errorMessage);
    }

    QGC_VERIFY_NOT_EMPTY(cameraFeedback);
}
