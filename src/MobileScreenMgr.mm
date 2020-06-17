/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "MobileScreenMgr.h"

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

void MobileScreenMgr::setKeepScreenOn(bool keepScreenOn)
{
    [[UIApplication sharedApplication] setIdleTimerDisabled: (keepScreenOn ? YES : NO)];
}
