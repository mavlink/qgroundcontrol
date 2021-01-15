/*!
 *   @file
 *   @brief Camera Video Stream Info
 *   @author Gus Grubba <gus@auterion.com>
 *   @author Hugo Trippaers <htrippaers@schubergphilis.com>
 *
 */

#ifndef QGCVIDEOSTREAMINFO_H
#define QGCVIDEOSTREAMINFO_H

#include "QGCApplication.h"

/// Video Stream Info
/// Encapsulates the contents of a [VIDEO_STREAM_INFORMATION](https://mavlink.io/en/messages/common.html#VIDEO_STREAM_INFORMATION) message
class QGCVideoStreamInfo : public QObject
{
    Q_OBJECT
public:
    QGCVideoStreamInfo(QObject* parent, const mavlink_video_stream_information_t* si);

    Q_PROPERTY(QString      uri                 READ uri                NOTIFY infoChanged)
    Q_PROPERTY(QString      name                READ name               NOTIFY infoChanged)
    Q_PROPERTY(int          streamID            READ streamID           NOTIFY infoChanged)
    Q_PROPERTY(int          type                READ type               NOTIFY infoChanged)
    Q_PROPERTY(qreal        aspectRatio         READ aspectRatio        NOTIFY infoChanged)
    Q_PROPERTY(qreal        hfov                READ hfov               NOTIFY infoChanged)
    Q_PROPERTY(bool         isThermal           READ isThermal          NOTIFY infoChanged)

    QString uri             () { return QString(_streamInfo.uri);  }
    QString name            () { return QString(_streamInfo.name); }
    qreal   aspectRatio     ();
    qreal   hfov            () { return _streamInfo.hfov; }
    int     type            () { return _streamInfo.type; }
    int     streamID        () { return _streamInfo.stream_id; }
    bool    isThermal       () { return _streamInfo.flags & VIDEO_STREAM_STATUS_FLAGS_THERMAL; }

    bool    update          (const mavlink_video_stream_status_t* vs);

signals:
    void    infoChanged     ();

private:
    mavlink_video_stream_information_t _streamInfo;
};


#endif // QGCVIDEOSTREAMINFO_H
