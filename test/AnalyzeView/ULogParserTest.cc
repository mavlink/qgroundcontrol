#include "ULogParserTest.h"
#include "ULogParser.h"
#include "GeoTagWorker.h"

#include <QtTest/QTest>

void ULogParserTest::_getTagsFromLogTest()
{
    QFile file(":/SampleULog.ulg");
    QVERIFY(file.open(QIODevice::ReadOnly));

    const QByteArray logBuffer = file.readAll();
    file.close();

    QList<GeoTagWorker::cameraFeedbackPacket> cameraFeedback;
    QString errorMessage;
    QVERIFY(ULogParser::getTagsFromLog(logBuffer, cameraFeedback, errorMessage));
    QVERIFY(errorMessage.isEmpty());
    QVERIFY(!cameraFeedback.isEmpty());

    const GeoTagWorker::cameraFeedbackPacket firstCameraFeedback = cameraFeedback.constFirst();
    // QVERIFY(!qFuzzyIsNull(firstCameraFeedback.timestamp));
    QVERIFY(firstCameraFeedback.imageSequence != 0);
}
