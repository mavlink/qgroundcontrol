#include "ExifParserTest.h"
#include "ExifParser.h"
#include "GeoTagWorker.h"

#include <QtTest/QTest>

void ExifParserTest::_readTimeTest()
{
    QFile file(":/DSCN0010.jpg");
    QVERIFY(file.open(QIODevice::ReadOnly));

    const QByteArray imageBuffer = file.readAll();
    file.close();

    const qint64 imageTime = ExifParser::readTime(imageBuffer).toSecsSinceEpoch();

    const QDate date(2008, 10, 22);
    const QTime time(16, 28, 39);

    const QDateTime tagTime(date, time);
    const qint64 expectedTime = tagTime.toSecsSinceEpoch();

    QCOMPARE(imageTime, expectedTime);
}

void ExifParserTest::_writeTest()
{
    QFile file(":/DSCN0010.jpg");
    QVERIFY(file.open(QIODevice::ReadOnly));

    QByteArray imageBuffer = file.readAll();
    file.close();

    struct GeoTagWorker::CameraFeedbackPacket data;

    data.latitude = 37.225;
    data.longitude = -80.425;
    data.altitude = 618.4392;

    QVERIFY(ExifParser::write(imageBuffer, data));
    // TODO: Read this back

    // QFile outputFile("./result.jpg");
    // QVERIFY(outputFile.open(QIODevice::WriteOnly | QIODevice::Truncate));
    // QCOMPARE(outputFile.write(imageBuffer), imageBuffer.size());
}
