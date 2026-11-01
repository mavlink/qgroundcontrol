#include "UVCFrameRelayTest.h"

#include <thread>

#include <QtCore/QRegularExpression>
#include <QtGui/QImage>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoSink>
#include <QtTest/QSignalSpy>

#include "UVCFrameRelay.h"

namespace {

QImage solidImage(const QColor& color)
{
    QImage image(QSize(16, 8), QImage::Format_RGBA8888);
    image.fill(color);
    return image;
}

}  // namespace

void UVCFrameRelayTest::_testTransformedLeavesFrameAlone()
{
    QVideoFrame frame(solidImage(Qt::green));
    frame.setRotation(QtVideo::Rotation::Clockwise270);
    frame.setStartTime(1000);
    frame.setEndTime(2000);

    // The transform adds to the frame's own, in a frame of its own: copies of the camera's frame (the recorder's, the
    // image capture's) keep theirs.
    const QVideoFrame turned = UVCFrameRelay::transformed(frame, QtVideo::Rotation::Clockwise180, true);
    QCOMPARE(turned.rotation(), QtVideo::Rotation::Clockwise90);
    QVERIFY(turned.mirrored());
    QCOMPARE(turned.startTime(), 1000);
    QCOMPARE(turned.endTime(), 2000);
    QCOMPARE(turned.surfaceFormat(), frame.surfaceFormat());
    QCOMPARE(frame.rotation(), QtVideo::Rotation::Clockwise270);
    QVERIFY(!frame.mirrored());

    // Its picture is the frame's buffer, mapped through the frame.
    QVideoFrame mapped(turned);
    QVERIFY(mapped.map(QVideoFrame::ReadOnly));
    QCOMPARE(mapped.planeCount(), 1);
    QCOMPARE(mapped.bytesPerLine(0), 16 * 4);
    const uchar* const pixel = mapped.bits(0);
    QCOMPARE(QColor(pixel[0], pixel[1], pixel[2]), QColor(Qt::green));
    mapped.unmap();

    QVERIFY(!UVCFrameRelay::transformed(QVideoFrame(), QtVideo::Rotation::Clockwise90, true).isValid());
}

void UVCFrameRelayTest::_testRelaysLatestFrameOnItsThread()
{
    UVCFrameRelay relay;
    relay.setTransform(QtVideo::Rotation::Clockwise90, false);
    QVideoSink output;
    QSignalSpy outputSpy(&output, &QVideoSink::videoFrameChanged);

    // Without an output, frames go nowhere.
    relay.input()->setVideoFrame(QVideoFrame(solidImage(Qt::red)));
    QCOMPARE(outputSpy.count(), 0);

    // On the relay's thread a frame goes on at once.
    relay.setOutput(&output);
    relay.input()->setVideoFrame(QVideoFrame(solidImage(Qt::blue)));
    QCOMPARE(outputSpy.count(), 1);
    QCOMPARE(output.videoFrame().rotation(), QtVideo::Rotation::Clockwise90);

    // From the camera's thread, the latest goes on, on the relay's thread; the others it replaced are dropped.
    outputSpy.clear();
    std::thread camera([&relay] {
        for (const QColor& color : {QColor(Qt::red), QColor(Qt::yellow), QColor(Qt::green)}) {
            relay.input()->setVideoFrame(QVideoFrame(solidImage(color)));
        }
    });
    camera.join();
    QCOMPARE(outputSpy.count(), 0);
    QVERIFY(outputSpy.wait(TestTimeout::shortMs()));
    QCOMPARE(outputSpy.count(), 1);
    // Qt Multimedia converts on the CPU when the platform (offscreen here) has no RHI, and says so.
    ignoreLogMessage("default", QtWarningMsg, QRegularExpression(QStringLiteral("No RHI backend")));
    QCOMPARE(output.videoFrame().toImage().pixelColor(0, 0), QColor(Qt::green));

    // Turning applies to the next frame.
    relay.setTransform(QtVideo::Rotation::None, true);
    relay.input()->setVideoFrame(QVideoFrame(solidImage(Qt::blue)));
    QCOMPARE(output.videoFrame().rotation(), QtVideo::Rotation::None);
    QVERIFY(output.videoFrame().mirrored());

    relay.setOutput(nullptr);
    outputSpy.clear();
    relay.input()->setVideoFrame(QVideoFrame(solidImage(Qt::red)));
    QCOMPARE(outputSpy.count(), 0);
}

UT_REGISTER_TEST(UVCFrameRelayTest, TestLabel::Unit)
