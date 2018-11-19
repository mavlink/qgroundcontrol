/****************************************************************************
 *
 *   (c) 2009-2018 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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

#pragma once

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

