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

    Q_PROPERTY(bool     isAndroid           READ isAndroid       CONSTANT)
    Q_PROPERTY(bool     isiOS               READ isiOS           CONSTANT)
    Q_PROPERTY(bool     isMobile            READ isMobile        CONSTANT)
    Q_PROPERTY(bool     isDebug             READ isDebug         CONSTANT)
    Q_PROPERTY(bool     isMacOS             READ isMacOS         CONSTANT)
    Q_PROPERTY(bool     isLinux             READ isLinux         CONSTANT)
    Q_PROPERTY(bool     isWindows           READ isWindows       CONSTANT)
    Q_PROPERTY(QString  iOSDevice           READ iOSDevice       CONSTANT)
    Q_PROPERTY(QString  fixedFontFamily     READ fixedFontFamily CONSTANT)

    // Returns current mouse position
    Q_INVOKABLE int mouseX(void) { return QCursor::pos().x(); }
    Q_INVOKABLE int mouseY(void) { return QCursor::pos().y(); }

#if defined(__mobile__)
    bool    isMobile            () { return true;  }
#else
    bool    isMobile            () { return qgcApp()->fakeMobile(); }
#endif

#if defined (Q_OS_ANDROID)
    bool    isAndroid           () { return true;  }
    bool    isiOS               () { return false; }
    bool    isLinux             () { return false; }
    bool    isMacOS             () { return false; }
    bool    isWindows           () { return false; }
#elif defined(Q_OS_IOS)
    bool    isAndroid           () { return false; }
    bool    isiOS               () { return true; }
    bool    isLinux             () { return false; }
    bool    isMacOS             () { return false; }
    bool    isWindows           () { return false; }
#elif defined(Q_OS_MAC)
    bool    isAndroid           () { return false; }
    bool    isiOS               () { return false; }
    bool    isLinux             () { return false; }
    bool    isMacOS             () { return true; }
    bool    isWindows           () { return false; }
#elif defined(Q_OS_LINUX)
    bool    isAndroid           () { return false; }
    bool    isiOS               () { return false; }
    bool    isLinux             () { return true; }
    bool    isMacOS             () { return false; }
    bool    isWindows           () { return false; }
#elif defined(Q_OS_WIN)
    bool    isAndroid           () { return false; }
    bool    isiOS               () { return false; }
    bool    isLinux             () { return false; }
    bool    isMacOS             () { return false; }
    bool    isWindows           () { return true; }
#else
    bool    isAndroid           () { return false; }
    bool    isiOS               () { return false; }
    bool    isLinux             () { return false; }
    bool    isMacOS             () { return false; }
    bool    isWindows           () { return false; }
#endif

#ifdef QT_DEBUG
    bool isDebug                () { return true; }
#else
    bool isDebug                () { return false; }
#endif

    QString  iOSDevice          () const;
    QString  fixedFontFamily    () const;

};

#endif
