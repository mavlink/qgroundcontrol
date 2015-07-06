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

/**
 * @file
 *   @brief QGC Video Receiver
 *   @author Gus Grubba <mavlink@grubba.com>
 */

#ifndef VIDEORECEIVER_H
#define VIDEORECEIVER_H

#include <QObject>
#if defined(QGC_GST_STREAMING)
#include <gst/gst.h>
#endif

class VideoReceiver : public QObject
{
    Q_OBJECT
public:
    explicit VideoReceiver(QObject* parent = 0);
    ~VideoReceiver();

#if defined(QGC_GST_STREAMING)
    void setVideoSink(GstElement* sink);
#endif

public Q_SLOTS:
    void start  ();
    void stop   ();
    void setUri (const QString& uri);

private:

#if defined(QGC_GST_STREAMING)
    void            _onBusMessage(GstMessage* message);
    static gboolean _onBusMessage(GstBus* bus, GstMessage* msg, gpointer data);
#endif

    QString     _uri;

#if defined(QGC_GST_STREAMING)
    GstElement* _pipeline;
    GstElement* _videoSink;
#endif

};

#endif // VIDEORECEIVER_H
