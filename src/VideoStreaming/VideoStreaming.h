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
 *   @brief QGC Video Streaming Initialization
 *   @author Gus Grubba <mavlink@grubba.com>
 */

#ifndef VIDEO_STREAMING_H
#define VIDEO_STREAMING_H

extern void initializeVideoStreaming    (int &argc, char *argv[]);
extern void shutdownVideoStreaming      ();

#endif // VIDEO_STREAMING_H
