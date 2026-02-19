#include "ULogParserTest.h"

#include <QtCore/QDir>
#include <QtCore/QTemporaryFile>
#include <QtTest/QTest>

#include "GeoTagData.h"
#include "ULogParser.h"
#include "ULogTestGenerator.h"

namespace {

QByteArray generateTestULog(const QString& tempDirPath, int numEvents = 20)
{
    QTemporaryFile tempFile(QDir(tempDirPath).filePath(QStringLiteral("ulog_parser_XXXXXX.ulg")));
    tempFile.setAutoRemove(false);  // Don't auto-remove on close
    if (!tempFile.open()) {
        return QByteArray();
    }
    const QString tempPath = tempFile.fileName();
    tempFile.close();

    const auto events = ULogTestGenerator::generateSampleEvents(numEvents);
    if (!ULogTestGenerator::generateULog(tempPath, events)) {
        QFile::remove(tempPath);
        return QByteArray();
    }

    QFile file(tempPath);
    if (!file.open(QIODevice::ReadOnly)) {
        QFile::remove(tempPath);
        return QByteArray();
    }
    const QByteArray data = file.readAll();
    file.close();
    QFile::remove(tempPath);  // Clean up after reading
    return data;
}

}  // namespace

void ULogParserTest::_getTagsFromLogTest()
{
    const QByteArray logBuffer = generateTestULog(tempDirPath(), 20);
    QVERIFY(!logBuffer.isEmpty());

    QList<GeoTagData> cameraFeedback;
    QString errorMessage;
    QVERIFY(ULogParser::getTagsFromLog(logBuffer, cameraFeedback, errorMessage));
    QVERIFY(errorMessage.isEmpty());
    QVERIFY(!cameraFeedback.isEmpty());

    const GeoTagData firstCameraFeedback = cameraFeedback.constFirst();
    QCOMPARE(firstCameraFeedback.imageSequence, 0u);  // First event has seq=0
}

void ULogParserTest::_getTagsFromLogEmptyTest()
{
    const QByteArray emptyBuffer;
    QList<GeoTagData> cameraFeedback;
    QString errorMessage;

    QVERIFY(!ULogParser::getTagsFromLog(emptyBuffer, cameraFeedback, errorMessage));
    QVERIFY(!errorMessage.isEmpty());
    QVERIFY(cameraFeedback.isEmpty());
}

void ULogParserTest::_getTagsFromLogInvalidTest()
{
    const QByteArray invalidBuffer("This is not a valid ULog file");
    QList<GeoTagData> cameraFeedback;
    QString errorMessage;

    QVERIFY(!ULogParser::getTagsFromLog(invalidBuffer, cameraFeedback, errorMessage));
    QVERIFY(!errorMessage.isEmpty());
    QVERIFY(cameraFeedback.isEmpty());
}

void ULogParserTest::_parseGeoTagDataFieldsTest()
{
    const QByteArray logBuffer = generateTestULog(tempDirPath(), 20);
    QVERIFY(!logBuffer.isEmpty());

    QList<GeoTagData> cameraFeedback;
    QString errorMessage;
    QVERIFY(ULogParser::getTagsFromLog(logBuffer, cameraFeedback, errorMessage));
    QVERIFY(!cameraFeedback.isEmpty());

    // Verify parsed fields are reasonable
    for (const GeoTagData& data : cameraFeedback) {
        // Timestamp should be positive
        QVERIFY(data.timestamp >= 0);

        // Coordinates should be valid if capture was successful
        if (data.captureResult == GeoTagData::CaptureResult::Success) {
            QVERIFY(data.coordinate.isValid());
            QVERIFY(data.coordinate.latitude() >= -90 && data.coordinate.latitude() <= 90);
            QVERIFY(data.coordinate.longitude() >= -180 && data.coordinate.longitude() <= 180);
        }

        // Quaternion should be normalized (approximately)
        const float quatLength = data.attitude.length();
        if (quatLength > 0) {
            QVERIFY(qAbs(quatLength - 1.0f) < 0.01f);
        }
    }
}

void ULogParserTest::_generatedULogTest()
{
    // Generate test ULog with valid camera captures
    const int numEvents = 20;
    const auto events = ULogTestGenerator::generateSampleEvents(numEvents);
    QCOMPARE(events.size(), numEvents);

    const QString generatedPath = tempPath(QStringLiteral("generated.ulg"));
    QVERIFY(ULogTestGenerator::generateULog(generatedPath, events));

    // Read back and verify
    QFile file(generatedPath);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QByteArray logBuffer = file.readAll();
    file.close();
    QFile::remove(generatedPath);  // Clean up after reading
    QVERIFY(!logBuffer.isEmpty());

    QList<GeoTagData> cameraFeedback;
    QString errorMessage;
    QVERIFY(ULogParser::getTagsFromLog(logBuffer, cameraFeedback, errorMessage));
    QCOMPARE(cameraFeedback.size(), numEvents);

    // Verify all events are valid
    int validCount = 0;
    for (const GeoTagData& data : cameraFeedback) {
        QVERIFY(data.coordinate.isValid());
        QCOMPARE(data.captureResult, GeoTagData::CaptureResult::Success);
        QVERIFY(data.isValid());
        validCount++;
    }
    QCOMPARE(validCount, numEvents);

    // Verify coordinates match what we generated
    for (int i = 0; i < numEvents; ++i) {
        const GeoTagData& parsed = cameraFeedback[i];
        const auto& original = events[i];

        QVERIFY(qAbs(parsed.coordinate.latitude() - original.coordinate.latitude()) < 0.0001);
        QVERIFY(qAbs(parsed.coordinate.longitude() - original.coordinate.longitude()) < 0.0001);
        QCOMPARE(parsed.imageSequence, original.seq);
    }
}

void ULogParserTest::_benchmarkGetTagsFromLog()
{
    const QByteArray logBuffer = generateTestULog(tempDirPath(), 50);
    QVERIFY(!logBuffer.isEmpty());

    QList<GeoTagData> cameraFeedback;
    QString errorMessage;

    QBENCHMARK
    {
        cameraFeedback.clear();
        errorMessage.clear();
        (void)ULogParser::getTagsFromLog(logBuffer, cameraFeedback, errorMessage);
    }

    QVERIFY(!cameraFeedback.isEmpty());
}

UT_REGISTER_TEST(ULogParserTest, TestLabel::Unit, TestLabel::AnalyzeView)
