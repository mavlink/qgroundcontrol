#include "SharedVideoFrameTest.h"

#include <QtGui/QColor>
#include <QtGui/QImage>
#include <QtMultimedia/QVideoFrame>

#include "SharedVideoFrame.h"

void SharedVideoFrameTest::_testOwnMetadataLeavesFrameAlone()
{
    QImage image(QSize(16, 8), QImage::Format_RGBA8888);
    image.fill(Qt::green);
    QVideoFrame frame(image);
    frame.setRotation(QtVideo::Rotation::Clockwise270);
    frame.setStartTime(1000);
    frame.setEndTime(2000);

    // The new frame starts with the frame's metadata, and changing its own leaves the frame, and so its copies (a
    // display's, a recorder's), alone.
    QVideoFrame shared = SharedVideoFrame::withOwnMetadata(frame);
    QCOMPARE(shared.rotation(), QtVideo::Rotation::Clockwise270);
    QCOMPARE(shared.startTime(), 1000);
    QCOMPARE(shared.endTime(), 2000);
    QCOMPARE(shared.surfaceFormat(), frame.surfaceFormat());
    shared.setRotation(QtVideo::Rotation::Clockwise90);
    shared.setMirrored(true);
    shared.setStartTime(5000);
    QCOMPARE(frame.rotation(), QtVideo::Rotation::Clockwise270);
    QVERIFY(!frame.mirrored());
    QCOMPARE(frame.startTime(), 1000);

    // Its picture is the frame's buffer, mapped through the frame.
    QVERIFY(shared.map(QVideoFrame::ReadOnly));
    QCOMPARE(shared.planeCount(), 1);
    QCOMPARE(shared.bytesPerLine(0), 16 * 4);
    const uchar* const pixel = shared.bits(0);
    QCOMPARE(QColor(pixel[0], pixel[1], pixel[2]), QColor(Qt::green));
    shared.unmap();

    QVERIFY(!SharedVideoFrame::withOwnMetadata(QVideoFrame()).isValid());
}

UT_REGISTER_TEST(SharedVideoFrameTest, TestLabel::Unit)
