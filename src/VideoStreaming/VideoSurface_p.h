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
 *   @brief QGC Video Surface (Private Interface)
 *   @author Gus Grubba <mavlink@grubba.com>
 */

#ifndef VIDEO_SURFACE_P_H
#define VIDEO_SURFACE_P_H

#include "VideoSurface.h"
#include "VideoItem.h"

class VideoSurfacePrivate
{
public:
    VideoSurfacePrivate()
        : videoSink(NULL)
    {
    }
    QSet<VideoItem*> items;
    GstElement* videoSink;
};

#endif // VIDEO_SURFACE_P_H
