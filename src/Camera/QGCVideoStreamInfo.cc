/*!
 *   @file
 *   @brief Camera Video Stream Info
 *   @author Gus Grubba <gus@auterion.com>
 *   @author Hugo Trippaers <htrippaers@schubergphilis.com>
 *
 */

#include "QGCVideoStreamInfo.h"

//-----------------------------------------------------------------------------
QGCVideoStreamInfo::QGCVideoStreamInfo(QObject* parent, const mavlink_video_stream_information_t *si)
    : QObject(parent)
{
    memcpy(&_streamInfo, si, sizeof(mavlink_video_stream_information_t));
}

//-----------------------------------------------------------------------------
qreal
QGCVideoStreamInfo::aspectRatio()
{
    qreal ar = 1.0;
    if(_streamInfo.resolution_h && _streamInfo.resolution_v) {
        ar = static_cast<double>(_streamInfo.resolution_h) / static_cast<double>(_streamInfo.resolution_v);
    }
    return ar;
}

//-----------------------------------------------------------------------------
bool
QGCVideoStreamInfo::update(const mavlink_video_stream_status_t* vs)
{
    bool changed = false;
    if(_streamInfo.hfov != vs->hfov) {
        changed = true;
        _streamInfo.hfov = vs->hfov;
    }
    if(_streamInfo.flags != vs->flags) {
        changed = true;
        _streamInfo.flags = vs->flags;
    }
    if(_streamInfo.bitrate != vs->bitrate) {
        changed = true;
        _streamInfo.bitrate = vs->bitrate;
    }
    if(_streamInfo.rotation != vs->rotation) {
        changed = true;
        _streamInfo.rotation = vs->rotation;
    }
    if(_streamInfo.framerate != vs->framerate) {
        changed = true;
        _streamInfo.framerate = vs->framerate;
    }
    if(_streamInfo.resolution_h != vs->resolution_h) {
        changed = true;
        _streamInfo.resolution_h = vs->resolution_h;
    }
    if(_streamInfo.resolution_v != vs->resolution_v) {
        changed = true;
        _streamInfo.resolution_v = vs->resolution_v;
    }
    if(changed) {
        emit infoChanged();
    }
    return changed;
}
