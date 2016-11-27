/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/**
 *   @brief QGC Main Application Interface (used for Dynamic Loaded plugins)
 *   @author Gus Grubba <mavlink@grubba.com>
 */

#pragma once

class IQGCApplication
{
public:
    IQGCApplication() {}
    virtual ~IQGCApplication() {}
    //-- Not yet implemented
};
