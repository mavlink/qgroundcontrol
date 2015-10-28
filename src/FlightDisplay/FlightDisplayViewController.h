/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009, 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

This file is part of the QGROUNDCONTROL project

    QGROUNDCONTROL is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    QGROUNDCONTROL is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

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
