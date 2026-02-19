#include "DataFlashParserTest.h"

#include <QtCore/QTemporaryFile>
#include <QtTest/QTest>

#include "DataFlashParser.h"
#include "DataFlashTestGenerator.h"
#include "GeoTagData.h"

namespace {

QByteArray generateTestDataFlash(int numEvents = 20)
{
    QTemporaryFile tempFile;
    tempFile.setAutoRemove(false);  // Don't auto-remove on close
    if (!tempFile.open()) {
        return QByteArray();
    }
    const QString tempPath = tempFile.fileName();
    tempFile.close();

    const auto events = DataFlashTestGenerator::generateSampleEvents(numEvents);
    if (!DataFlashTestGenerator::generateDataFlashLog(tempPath, events)) {
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

void DataFlashParserTest::_getTagsFromLogTest()
{
    const QByteArray logBuffer = generateTestDataFlash(20);
    QVERIFY(!logBuffer.isEmpty());

    QList<GeoTagData> cameraFeedback;
    QString errorMessage;
    QVERIFY(DataFlashParser::getTagsFromLog(logBuffer, cameraFeedback, errorMessage));
    QVERIFY(errorMessage.isEmpty());
    QVERIFY(!cameraFeedback.isEmpty());

    const GeoTagData firstCameraFeedback = cameraFeedback.constFirst();
    QCOMPARE(firstCameraFeedback.imageSequence, 0u);
}

void DataFlashParserTest::_getTagsFromLogEmptyTest()
{
    const QByteArray emptyBuffer;
    QList<GeoTagData> cameraFeedback;
    QString errorMessage;

    QVERIFY(!DataFlashParser::getTagsFromLog(emptyBuffer, cameraFeedback, errorMessage));
    QVERIFY(!errorMessage.isEmpty());
    QVERIFY(cameraFeedback.isEmpty());
}

void DataFlashParserTest::_getTagsFromLogInvalidTest()
{
    const QByteArray invalidBuffer("This is not a valid DataFlash file");
    QList<GeoTagData> cameraFeedback;
    QString errorMessage;

    QVERIFY(!DataFlashParser::getTagsFromLog(invalidBuffer, cameraFeedback, errorMessage));
    QVERIFY(!errorMessage.isEmpty());
    QVERIFY(cameraFeedback.isEmpty());
}

void DataFlashParserTest::_parseGeoTagDataFieldsTest()
{
    const QByteArray logBuffer = generateTestDataFlash(20);
    QVERIFY(!logBuffer.isEmpty());

    QList<GeoTagData> cameraFeedback;
    QString errorMessage;
    QVERIFY(DataFlashParser::getTagsFromLog(logBuffer, cameraFeedback, errorMessage));
    QVERIFY(!cameraFeedback.isEmpty());

    for (const GeoTagData& data : cameraFeedback) {
        QVERIFY(data.timestamp >= 0);

        if (data.captureResult == GeoTagData::CaptureResult::Success) {
            QVERIFY(data.coordinate.isValid());
            QVERIFY(data.coordinate.latitude() >= -90 && data.coordinate.latitude() <= 90);
            QVERIFY(data.coordinate.longitude() >= -180 && data.coordinate.longitude() <= 180);
        }

        const float quatLength = data.attitude.length();
        if (quatLength > 0) {
            QVERIFY(qAbs(quatLength - 1.0f) < 0.01f);
        }
    }
}

void DataFlashParserTest::_generatedDataFlashTest()
{
    const int numEvents = 20;
    const auto events = DataFlashTestGenerator::generateSampleEvents(numEvents);
    QCOMPARE(events.size(), numEvents);

    QTemporaryFile tempFile;
    tempFile.setAutoRemove(true);
    QVERIFY(tempFile.open());
    const QString tempPath = tempFile.fileName();
    tempFile.close();

    QVERIFY(DataFlashTestGenerator::generateDataFlashLog(tempPath, events));

    QFile file(tempPath);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QByteArray logBuffer = file.readAll();
    file.close();
    QVERIFY(!logBuffer.isEmpty());

    QList<GeoTagData> cameraFeedback;
    QString errorMessage;
    QVERIFY(DataFlashParser::getTagsFromLog(logBuffer, cameraFeedback, errorMessage));
    QCOMPARE(cameraFeedback.size(), numEvents);

    int validCount = 0;
    for (const GeoTagData& data : cameraFeedback) {
        QVERIFY(data.coordinate.isValid());
        QCOMPARE(data.captureResult, GeoTagData::CaptureResult::Success);
        QVERIFY(data.isValid());
        validCount++;
    }
    QCOMPARE(validCount, numEvents);

    for (int i = 0; i < numEvents; ++i) {
        const GeoTagData& parsed = cameraFeedback[i];
        const auto& original = events[i];

        QVERIFY(qAbs(parsed.coordinate.latitude() - original.coordinate.latitude()) < 0.0001);
        QVERIFY(qAbs(parsed.coordinate.longitude() - original.coordinate.longitude()) < 0.0001);
        QCOMPARE(parsed.imageSequence, original.seq);
    }
}

void DataFlashParserTest::_benchmarkGetTagsFromLog()
{
    const QByteArray logBuffer = generateTestDataFlash(50);
    QVERIFY(!logBuffer.isEmpty());

    QList<GeoTagData> cameraFeedback;
    QString errorMessage;

    QBENCHMARK
    {
        cameraFeedback.clear();
        errorMessage.clear();
        (void)DataFlashParser::getTagsFromLog(logBuffer, cameraFeedback, errorMessage);
    }

    QVERIFY(!cameraFeedback.isEmpty());
}

UT_REGISTER_TEST(DataFlashParserTest, TestLabel::Unit, TestLabel::AnalyzeView)
