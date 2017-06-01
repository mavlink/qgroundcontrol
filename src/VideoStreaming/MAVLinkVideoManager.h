/****************************************************************************
 *
 * Copyright (c) 2017, Intel Corporation
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/**
 * @file
 *   @brief QGC MAVLink video streaming Manager
 */

#ifndef MAVLINK_VIDEO_MANAGER_H
#define MAVLINK_VIDEO_MANAGER_H

#include <QObject>
#include <QStringList>

#include "MAVLinkProtocol.h"

class Stream : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString text MEMBER name NOTIFY nameChanged)
public:
    Stream(int _cameraId, QString _name)
        : cameraId(_cameraId)
        , name(_name)
        , uri("") {}

    ~Stream() { }
    int cameraId;
    QString name;
    QString uri;
    signals:
        void nameChanged();
};

class MAVLinkVideoManager : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QList<QObject*>  streamList              READ streamList             NOTIFY streamListChanged)
    Q_PROPERTY(int              selectedStream          READ selectedStream         WRITE  setSelectedStream        NOTIFY selectedStreamChanged)
    Q_PROPERTY(QString          currentUri              READ getVideoURI  NOTIFY currentUriChanged)

public:
    MAVLinkVideoManager();
    ~MAVLinkVideoManager();

    QList<QObject *> streamList() {
        return _streamList;
    }

    int selectedStream() {
        return _selectedStream;
    }

    QString getVideoURI();
    void setSelectedStream(int index);

signals:
    void streamListChanged();
    void selectedStreamChanged();
    void currentUriChanged();

private slots:
    void _mavlinkMessageReceived(LinkInterface *link, mavlink_message_t message);
    void _videoHeartbeatInfo(LinkInterface *link, int systemId);

private:
    int _cameraSysid;
    MAVLinkProtocol *_mavlink;
    LinkInterface *_cameraLink;
    QList<QObject*> _streamList;
    int _selectedStream;

    Stream *_find_camera_by_id(int cameraId);
};

#endif // MAVLINK_VIDEO_MANAGER_H
