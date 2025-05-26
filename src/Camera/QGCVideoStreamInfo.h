/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QSize>

#include "MAVLinkLib.h"

Q_DECLARE_LOGGING_CATEGORY(QGCVideoStreamInfoLog)

/// Encapsulates the contents of a [VIDEO_STREAM_INFORMATION](https://mavlink.io/en/messages/common.html#VIDEO_STREAM_INFORMATION) message
class QGCVideoStreamInfo : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString  uri         READ uri            NOTIFY infoChanged)
    Q_PROPERTY(QString  name        READ name           NOTIFY infoChanged)
    Q_PROPERTY(quint8   streamID    READ streamID       NOTIFY infoChanged)
    Q_PROPERTY(quint8   type        READ type           NOTIFY infoChanged)
    Q_PROPERTY(qreal    aspectRatio READ aspectRatio    NOTIFY infoChanged)
    Q_PROPERTY(quint16  hfov        READ hfov           NOTIFY infoChanged)
    Q_PROPERTY(bool     isThermal   READ isThermal      NOTIFY infoChanged)
    Q_PROPERTY(bool     isActive    READ isActive       NOTIFY infoChanged)
    Q_PROPERTY(QSize    resolution  READ resolution     NOTIFY infoChanged)
    Q_PROPERTY(quint8   encoding    READ encoding       NOTIFY infoChanged)
    Q_PROPERTY(quint16  rotation    READ rotation       NOTIFY infoChanged)
    Q_PROPERTY(quint32  bitrate     READ bitrate        NOTIFY infoChanged)
    Q_PROPERTY(qreal    framerate   READ framerate      NOTIFY infoChanged)
    Q_ENUM(VIDEO_STREAM_TYPE)
    Q_ENUM(VIDEO_STREAM_ENCODING)
    Q_FLAGS(QVIDEO_STREAM_STATUS_FLAGS)

public:
    explicit QGCVideoStreamInfo(const mavlink_video_stream_information_t &info, QObject *parent = nullptr);

    Q_DECLARE_FLAGS(QVIDEO_STREAM_STATUS_FLAGS, VIDEO_STREAM_STATUS_FLAGS)

    QString uri() const { return QString(_streamInfo.uri);  }
    QString name() const { return QString(_streamInfo.name); }
    qreal aspectRatio() const;
    quint16 hfov() const { return _streamInfo.hfov; }
    quint8 type() const { return _streamInfo.type; }
    quint8 streamID() const { return _streamInfo.stream_id; }
    quint8 encoding() const { return _streamInfo.encoding; }
    bool isThermal() const { return (_streamInfo.flags & VIDEO_STREAM_STATUS_FLAGS_THERMAL); }
    QSize resolution() const { return QSize(_streamInfo.resolution_h, _streamInfo.resolution_v); }
    quint16 rotation() const { return _streamInfo.rotation; }
    bool isActive() const { return (_streamInfo.flags & VIDEO_STREAM_STATUS_FLAGS_RUNNING); }
    uint32_t bitrate() const { return _streamInfo.bitrate; }
    qreal framerate() const { return _streamInfo.framerate; }

    bool update(const mavlink_video_stream_status_t &info);

signals:
    void infoChanged();

private:
    mavlink_video_stream_information_t _streamInfo{};
};
Q_DECLARE_METATYPE(VIDEO_STREAM_TYPE)
Q_DECLARE_METATYPE(VIDEO_STREAM_ENCODING)
Q_DECLARE_METATYPE(QGCVideoStreamInfo::QVIDEO_STREAM_STATUS_FLAGS)
