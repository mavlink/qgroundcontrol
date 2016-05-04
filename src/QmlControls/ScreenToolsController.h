/*=====================================================================

 QGroundControl Open Source Ground Control Station

 (c) 2009, 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

 This file is part of the QGROUNDCONTROL project

 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

 ======================================================================*/

/// @file
///     @author Gus Grubba <mavlink@grubba.com>

#ifndef ScreenToolsController_H
#define ScreenToolsController_H

#include "QGCApplication.h"
#include <QQuickItem>
#include <QCursor>

/*!
    @brief Screen helper tools for QML widgets
*/

/// This Qml control is used to return screen parameters
class ScreenToolsController : public QQuickItem
{
    Q_OBJECT
public:
    ScreenToolsController();

    Q_PROPERTY(bool     isAndroid           READ isAndroid      CONSTANT)
    Q_PROPERTY(bool     isiOS               READ isiOS          CONSTANT)
    Q_PROPERTY(bool     isMobile            READ isMobile       CONSTANT)
    Q_PROPERTY(bool     testHighDPI         READ testHighDPI    CONSTANT)
    Q_PROPERTY(bool     isDebug             READ isDebug        CONSTANT)
    Q_PROPERTY(bool     isMacOS             READ isMacOS        CONSTANT)
    Q_PROPERTY(bool     isLinux             READ isLinux        CONSTANT)

    // Returns current mouse position
    Q_INVOKABLE int mouseX(void) { return QCursor::pos().x(); }
    Q_INVOKABLE int mouseY(void) { return QCursor::pos().y(); }

#if defined (__android__)
    bool    isAndroid           () { return true;  }
    bool    isiOS               () { return false; }
    bool    isMobile            () { return true;  }
    bool    isLinux             () { return false; }
    bool    isMacOS             () { return false; }
#elif defined(__ios__)
    bool    isAndroid           () { return false; }
    bool    isiOS               () { return true; }
    bool    isMobile            () { return true; }
    bool    isLinux             () { return false; }
    bool    isMacOS             () { return false; }
#elif defined(__macos__)
    bool    isAndroid           () { return false; }
    bool    isiOS               () { return false; }
    bool    isMobile            () { return qgcApp()->fakeMobile(); }
    bool    isLinux             () { return false; }
    bool    isMacOS             () { return true; }
#elif defined(Q_OS_LINUX)
    bool    isAndroid           () { return false; }
    bool    isiOS               () { return false; }
    bool    isMobile            () { return qgcApp()->fakeMobile(); }
    bool    isLinux             () { return true; }
    bool    isMacOS             () { return false; }
#else
    bool    isAndroid           () { return false; }
    bool    isiOS               () { return false; }
    bool    isMobile            () { return qgcApp()->fakeMobile(); }
    bool    isLinux             () { return false; }
    bool    isMacOS             () { return false; }
#endif

#ifdef QT_DEBUG
    bool testHighDPI            () { return qgcApp()->testHighDPI(); }
    bool isDebug                () { return true; }
#else
    bool isDebug                () { return false; }
    bool testHighDPI            () { return false; }
#endif

};

#endif
