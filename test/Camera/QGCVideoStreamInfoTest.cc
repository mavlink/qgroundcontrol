#include "QGCVideoStreamInfoTest.h"

#include <QtTest/QSignalSpy>

#include "QGCVideoStreamInfo.h"

namespace {

mavlink_video_stream_information_t _makeVideoStreamInfo()
{
    mavlink_video_stream_information_t info{};
    info.stream_id = 3;
    info.type = VIDEO_STREAM_TYPE_RTSP;
    info.flags = VIDEO_STREAM_STATUS_FLAGS_RUNNING;
    info.encoding = VIDEO_STREAM_ENCODING_H264;
    info.resolution_h = 1920;
    info.resolution_v = 1080;
    info.framerate = 30.0f;
    info.bitrate = 8000000;
    info.rotation = 0;
    info.hfov = 90;
    qstrncpy(info.name, "Main Stream", sizeof(info.name));
    qstrncpy(info.uri, "rtsp://127.0.0.1/main", sizeof(info.uri));
    return info;
}

mavlink_video_stream_status_t _makeVideoStreamStatusFromInfo(const mavlink_video_stream_information_t &info)
{
    mavlink_video_stream_status_t status{};
    status.stream_id = info.stream_id;
    status.flags = info.flags;
    status.framerate = info.framerate;
    status.resolution_h = info.resolution_h;
    status.resolution_v = info.resolution_v;
    status.bitrate = info.bitrate;
    status.rotation = info.rotation;
    status.hfov = info.hfov;
    return status;
}

} // namespace

void QGCVideoStreamInfoTest::_aspectRatioFromResolution_test()
{
    const mavlink_video_stream_information_t info = _makeVideoStreamInfo();
    QGCVideoStreamInfo streamInfo(info);

    QCOMPARE(streamInfo.aspectRatio(), static_cast<qreal>(1920.0 / 1080.0));
}

void QGCVideoStreamInfoTest::_aspectRatioDefaultsToOneForNullResolution_test()
{
    mavlink_video_stream_information_t info = _makeVideoStreamInfo();
    info.resolution_h = 0;
    info.resolution_v = 0;

    QGCVideoStreamInfo streamInfo(info);
    QCOMPARE(streamInfo.aspectRatio(), 1.0);
}

void QGCVideoStreamInfoTest::_updateNoChange_test()
{
    const mavlink_video_stream_information_t info = _makeVideoStreamInfo();
    QGCVideoStreamInfo streamInfo(info);

    QSignalSpy spy(&streamInfo, &QGCVideoStreamInfo::infoChanged);
    QVERIFY(spy.isValid());

    const bool changed = streamInfo.update(_makeVideoStreamStatusFromInfo(info));
    QVERIFY(!changed);
    QCOMPARE(spy.count(), 0);
}

void QGCVideoStreamInfoTest::_updateChangedFieldsEmitsSignal_test()
{
    const mavlink_video_stream_information_t info = _makeVideoStreamInfo();
    QGCVideoStreamInfo streamInfo(info);

    QSignalSpy spy(&streamInfo, &QGCVideoStreamInfo::infoChanged);
    QVERIFY(spy.isValid());

    mavlink_video_stream_status_t updated = _makeVideoStreamStatusFromInfo(info);
    updated.flags = VIDEO_STREAM_STATUS_FLAGS_RUNNING | VIDEO_STREAM_STATUS_FLAGS_THERMAL;
    updated.hfov = 110;
    updated.rotation = 270;
    updated.bitrate = 5000000;
    updated.framerate = 24.0f;
    updated.resolution_h = 1280;
    updated.resolution_v = 720;

    const bool changed = streamInfo.update(updated);
    QVERIFY(changed);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(streamInfo.hfov(), 110);
    QCOMPARE(streamInfo.rotation(), 270);
    QCOMPARE(streamInfo.bitrate(), 5000000U);
    QCOMPARE(streamInfo.resolution(), QSize(1280, 720));
    QCOMPARE(streamInfo.framerate(), 24.0);
    QVERIFY(streamInfo.isActive());
    QVERIFY(streamInfo.isThermal());
}

UT_REGISTER_TEST(QGCVideoStreamInfoTest, TestLabel::Unit)

