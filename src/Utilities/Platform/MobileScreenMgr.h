#pragma once

#include <QtCore/QtSystemDetection>

#ifdef Q_OS_IOS

namespace MobileScreenMgr
{
    /// Turns on/off screen sleep on mobile devices
    void setKeepScreenOn(bool keepScreenOn);
};

#endif
