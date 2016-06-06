/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


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
    Q_PROPERTY(QString  iOSDevice           READ iOSDevice      CONSTANT)

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

    QString  iOSDevice          ();

};

#endif
