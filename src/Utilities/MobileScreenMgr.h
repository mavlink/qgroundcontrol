/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#ifndef MobileScreenMgr_H
#define MobileScreenMgr_H

#ifdef __mobile__
class MobileScreenMgr {
    
public:
    /// Turns on/off screen sleep on mobile devices
    static void setKeepScreenOn(bool keepScreenOn);
};
#endif

#endif
