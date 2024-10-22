#include "PX4LogParserTest.h"
#include "PX4LogParser.h"
#include "GeoTagWorker.h"

#include <QtTest/QTest>

void PX4LogParserTest::_getTagsFromLogTest()
{
    /*QFile file("SamplePX4Log.");
    QVERIFY(file.open(QIODevice::ReadOnly));

    const QByteArray logBuffer = file.readAll();
    file.close();

    QList<GeoTagWorker::CameraFeedbackPacket> cameraFeedback;
    QVERIFY(PX4LogParser::getTagsFromLog(logBuffer, cameraFeedback));
    QVERIFY(!cameraFeedback.isEmpty());

    GeoTagWorker::CameraFeedbackPacket firstCameraFeedback = cameraFeedback.first();
    QVERIFY(!qFuzzyIsNull(firstCameraFeedback.timestamp));
    QVERIFY(firstCameraFeedback.imageSequence != 0);*/
}
