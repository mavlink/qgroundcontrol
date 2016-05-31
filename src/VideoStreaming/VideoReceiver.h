/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


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
