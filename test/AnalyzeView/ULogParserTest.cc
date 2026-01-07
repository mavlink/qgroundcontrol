#include "ULogParserTest.h"
#include "ULogParser.h"
#include "GeoTagWorker.h"

#include <QtTest/QTest>

namespace {

QByteArray loadSampleULog()
{
    QFile file(":/unittest/SampleULog.ulg");
    if (!file.open(QIODevice::ReadOnly)) {
        return QByteArray();
    }
    const QByteArray data = file.readAll();
    file.close();
    return data;
}

bool compareFeedbackPackets(const GeoTagWorker::CameraFeedbackPacket &a, const GeoTagWorker::CameraFeedbackPacket &b)
{
    return qFuzzyCompare(a.timestamp, b.timestamp) &&
           qFuzzyCompare(a.timestampUTC, b.timestampUTC) &&
           a.imageSequence == b.imageSequence &&
           qFuzzyCompare(a.latitude, b.latitude) &&
           qFuzzyCompare(a.longitude, b.longitude) &&
           qFuzzyCompare(a.altitude, b.altitude) &&
           qFuzzyCompare(a.groundDistance, b.groundDistance) &&
           a.captureResult == b.captureResult;
}

} // namespace

void ULogParserTest::_getTagsFromLogTest()
{
    const QByteArray logBuffer = loadSampleULog();
    QVERIFY(!logBuffer.isEmpty());

    QList<GeoTagWorker::CameraFeedbackPacket> cameraFeedback;
    QString errorMessage;
    QVERIFY(ULogParser::getTagsFromLog(logBuffer, cameraFeedback, errorMessage));
    QVERIFY(errorMessage.isEmpty());
    QVERIFY(!cameraFeedback.isEmpty());

    const GeoTagWorker::CameraFeedbackPacket firstCameraFeedback = cameraFeedback.constFirst();
    QVERIFY(firstCameraFeedback.imageSequence != 0);
}

void ULogParserTest::_getTagsFromLogStreamedTest()
{
    const QByteArray logBuffer = loadSampleULog();
    QVERIFY(!logBuffer.isEmpty());

    QList<GeoTagWorker::CameraFeedbackPacket> cameraFeedback;
    QString errorMessage;
    QVERIFY(ULogParser::getTagsFromLogStreamed(logBuffer, cameraFeedback, errorMessage));
    QVERIFY(errorMessage.isEmpty());
    QVERIFY(!cameraFeedback.isEmpty());

    const GeoTagWorker::CameraFeedbackPacket firstCameraFeedback = cameraFeedback.constFirst();
    QVERIFY(firstCameraFeedback.imageSequence != 0);
}

void ULogParserTest::_compareStreamedAndNonStreamedTest()
{
    const QByteArray logBuffer = loadSampleULog();
    QVERIFY(!logBuffer.isEmpty());

    QList<GeoTagWorker::CameraFeedbackPacket> feedbackNonStreamed;
    QList<GeoTagWorker::CameraFeedbackPacket> feedbackStreamed;
    QString errorNonStreamed;
    QString errorStreamed;

    QVERIFY(ULogParser::getTagsFromLog(logBuffer, feedbackNonStreamed, errorNonStreamed));
    QVERIFY(ULogParser::getTagsFromLogStreamed(logBuffer, feedbackStreamed, errorStreamed));

    QCOMPARE(feedbackStreamed.size(), feedbackNonStreamed.size());

    for (int i = 0; i < feedbackStreamed.size(); ++i) {
        QVERIFY2(compareFeedbackPackets(feedbackStreamed[i], feedbackNonStreamed[i]),
                 qPrintable(QStringLiteral("Packet %1 differs between streamed and non-streamed").arg(i)));
    }
}

void ULogParserTest::_benchmarkNonStreamed()
{
    const QByteArray logBuffer = loadSampleULog();
    QVERIFY(!logBuffer.isEmpty());

    QList<GeoTagWorker::CameraFeedbackPacket> cameraFeedback;
    QString errorMessage;

    QBENCHMARK {
        cameraFeedback.clear();
        errorMessage.clear();
        (void) ULogParser::getTagsFromLog(logBuffer, cameraFeedback, errorMessage);
    }

    QVERIFY(!cameraFeedback.isEmpty());
}

void ULogParserTest::_benchmarkStreamed()
{
    const QByteArray logBuffer = loadSampleULog();
    QVERIFY(!logBuffer.isEmpty());

    QList<GeoTagWorker::CameraFeedbackPacket> cameraFeedback;
    QString errorMessage;

    QBENCHMARK {
        cameraFeedback.clear();
        errorMessage.clear();
        (void) ULogParser::getTagsFromLogStreamed(logBuffer, cameraFeedback, errorMessage);
    }

    QVERIFY(!cameraFeedback.isEmpty());
}
