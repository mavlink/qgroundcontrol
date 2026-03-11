#include "MobileScreenMgr.h"

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

void MobileScreenMgr::setKeepScreenOn(bool keepScreenOn)
{
    [[UIApplication sharedApplication] setIdleTimerDisabled: (keepScreenOn ? YES : NO)];
}
