/****************************************************************************
 *
 *   (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QtSystemDetection>

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)

namespace MobileScreenMgr {
    /// Turns on/off screen sleep on mobile devices
    void setKeepScreenOn(bool keepScreenOn);
};

#endif
