#include "QGCVideoStreamInfo.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(QGCVideoStreamInfoLog, "Camera.QGCVideoStreamInfo")

QGCVideoStreamInfo::QGCVideoStreamInfo(const mavlink_video_stream_information_t &info, QObject *parent)
    : QObject(parent)
{
    qCDebug(QGCVideoStreamInfoLog) << this;

    (void) memcpy(&_streamInfo, &info, sizeof(mavlink_video_stream_information_t));
}

qreal QGCVideoStreamInfo::aspectRatio() const
{
    qreal ar = 1.0;
    if (!resolution().isEmpty()) {
        ar = static_cast<double>(_streamInfo.resolution_h) / static_cast<double>(_streamInfo.resolution_v);
    }
    return ar;
}

bool QGCVideoStreamInfo::update(const mavlink_video_stream_status_t &status)
{
    bool changed = false;
    const auto assign = [&changed](auto& field, auto value) {
        if (field != value) {
            field = value;
            changed = true;
        }
    };

    assign(_streamInfo.hfov, status.hfov);
    assign(_streamInfo.flags, status.flags);
    assign(_streamInfo.bitrate, status.bitrate);
    assign(_streamInfo.rotation, status.rotation);
    assign(_streamInfo.framerate, status.framerate);
    assign(_streamInfo.resolution_h, status.resolution_h);
    assign(_streamInfo.resolution_v, status.resolution_v);

    if (changed) {
        emit infoChanged();
    }

    return changed;
}
