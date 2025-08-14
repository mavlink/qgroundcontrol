/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QtSystemDetection>

namespace Platform {
#ifdef Q_OS_MAC
    void disableAppNapViaInfoDict();
#elif defined(Q_OS_WIN)
    void setWindowsNativeFunctions(bool quietWindowsAsserts);
#endif
}
