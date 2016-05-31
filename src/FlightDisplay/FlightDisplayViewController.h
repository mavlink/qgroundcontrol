/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#ifndef FlightDisplayViewController_H
#define FlightDisplayViewController_H

#include <QObject>
#include <QTimer>

#include "VideoSurface.h"
#include "VideoReceiver.h"

class FlightDisplayViewController : public QObject
{
    Q_OBJECT

public:
    FlightDisplayViewController(QObject* parent = NULL);
    ~FlightDisplayViewController();

    Q_PROPERTY(bool hasVideo READ hasVideo CONSTANT)

    Q_PROPERTY(VideoSurface*    videoSurface    MEMBER _videoSurface    CONSTANT);
    Q_PROPERTY(VideoReceiver*   videoReceiver   MEMBER _videoReceiver   CONSTANT);

    Q_PROPERTY(bool             videoRunning    READ videoRunning       NOTIFY videoRunningChanged)

#if defined(QGC_GST_STREAMING)
    bool    hasVideo            () { return true; }
#else
    bool    hasVideo            () { return false; }
#endif

    bool videoRunning() { return _videoRunning; }

signals:
    void videoRunningChanged();

private:
    void _updateTimer(void);

private:
    VideoSurface*   _videoSurface;
    VideoReceiver*  _videoReceiver;
    bool            _videoRunning;
#if defined(QGC_GST_STREAMING)
    QTimer          _frameTimer;
#endif
};

#endif
