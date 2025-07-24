/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "QGCVideoStreamInfo.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(QGCVideoStreamInfoLog, "qgc.camera.qgcvideostreaminfo")

QGCVideoStreamInfo::QGCVideoStreamInfo(const mavlink_video_stream_information_t &info, QObject *parent)
    : QObject(parent)
{
    (void) memcpy(&_streamInfo, &info, sizeof(mavlink_video_stream_information_t));
}

qreal QGCVideoStreamInfo::aspectRatio() const
{
    qreal ar = 1.0;
    if (!resolution().isNull()) {
        ar = static_cast<double>(_streamInfo.resolution_h) / static_cast<double>(_streamInfo.resolution_v);
    }
    return ar;
}

bool QGCVideoStreamInfo::update(const mavlink_video_stream_status_t &status)
{
    bool changed = false;

    if (_streamInfo.hfov != status.hfov) {
        changed = true;
        _streamInfo.hfov = status.hfov;
    }

    if (_streamInfo.flags != status.flags) {
        changed = true;
        _streamInfo.flags = status.flags;
    }

    if (_streamInfo.bitrate != status.bitrate) {
        changed = true;
        _streamInfo.bitrate = status.bitrate;
    }

    if (_streamInfo.rotation != status.rotation) {
        changed = true;
        _streamInfo.rotation = status.rotation;
    }

    if (_streamInfo.framerate != status.framerate) {
        changed = true;
        _streamInfo.framerate = status.framerate;
    }

    if (_streamInfo.resolution_h != status.resolution_h) {
        changed = true;
        _streamInfo.resolution_h = status.resolution_h;
    }

    if (_streamInfo.resolution_v != status.resolution_v) {
        changed = true;
        _streamInfo.resolution_v = status.resolution_v;
    }

    if (changed) {
        emit infoChanged();
    }

    return changed;
}
