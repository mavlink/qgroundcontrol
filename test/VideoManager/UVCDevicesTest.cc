#include "UVCDevicesTest.h"

#include <QtMultimedia/QCameraDevice>
#include <QtMultimedia/private/qcameradevice_p.h>

#include "UVCDevices.h"

namespace {

QCameraFormat cameraFormat(QSize resolution, float maxFrameRate, QVideoFrameFormat::PixelFormat pixelFormat)
{
    auto* const format = new QCameraFormatPrivate;
    format->resolution = resolution;
    format->minFrameRate = maxFrameRate;
    format->maxFrameRate = maxFrameRate;
    format->pixelFormat = pixelFormat;
    return format->create();
}

/// A camera offering, in a driver's arbitrary order, one format twice.
QCameraDevice camera()
{
    auto* const device = new QCameraDevicePrivate;
    device->id = QByteArrayLiteral("/dev/video0");
    device->description = QStringLiteral("Test Camera");
    device->videoFormats = {
        cameraFormat(QSize(640, 480), 30.0F, QVideoFrameFormat::Format_YUYV),
        cameraFormat(QSize(1920, 1080), 30.0F, QVideoFrameFormat::Format_Jpeg),
        cameraFormat(QSize(1920, 1080), 60.0F, QVideoFrameFormat::Format_Jpeg),
        cameraFormat(QSize(1280, 720), 7.5F, QVideoFrameFormat::Format_NV12),
        cameraFormat(QSize(640, 480), 30.0F, QVideoFrameFormat::Format_YUYV),
    };
    return device->create();
}

}  // namespace

void UVCDevicesTest::_testPixelFormatName()
{
    QCOMPARE(UVCDevices::pixelFormatName(QVideoFrameFormat::Format_Jpeg), QStringLiteral("MJPEG"));
    QCOMPARE(UVCDevices::pixelFormatName(QVideoFrameFormat::Format_YUYV), QStringLiteral("YUYV"));
    QCOMPARE(UVCDevices::pixelFormatName(QVideoFrameFormat::Format_NV12), QStringLiteral("NV12"));
    QVERIFY(UVCDevices::pixelFormatName(QVideoFrameFormat::Format_Invalid).isEmpty());
}

void UVCDevicesTest::_testFormatNamesBestFirst()
{
    // More pixels first, then the higher frame rate; a format offered twice is listed once.
    QCOMPARE(UVCDevices::formatNames(camera()), QStringList({
                                                    QStringLiteral("1920x1080 60 fps MJPEG"),
                                                    QStringLiteral("1920x1080 30 fps MJPEG"),
                                                    QStringLiteral("1280x720 7.5 fps NV12"),
                                                    QStringLiteral("640x480 30 fps YUYV"),
                                                }));
    QVERIFY(UVCDevices::formatNames(QCameraDevice()).isEmpty());
}

void UVCDevicesTest::_testFormatByName()
{
    const QCameraFormat format = UVCDevices::format(camera(), QStringLiteral("1280x720 7.5 fps NV12"));
    QVERIFY(!format.isNull());
    QCOMPARE(format.resolution(), QSize(1280, 720));
    QCOMPARE(format.maxFrameRate(), 7.5F);
    QCOMPARE(format.pixelFormat(), QVideoFrameFormat::Format_NV12);

    // Auto, and formats the camera doesn't offer, are no format: Qt chooses.
    QVERIFY(UVCDevices::format(camera(), QString()).isNull());
    QVERIFY(UVCDevices::format(camera(), QStringLiteral("3840x2160 30 fps MJPEG")).isNull());
}

UT_REGISTER_TEST(UVCDevicesTest, TestLabel::Unit)
