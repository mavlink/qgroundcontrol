/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Gus Grubba <gus@auterion.com>

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

    Q_PROPERTY(bool     isAndroid           READ isAndroid          CONSTANT)
    Q_PROPERTY(bool     isiOS               READ isiOS              CONSTANT)
    Q_PROPERTY(bool     isMobile            READ isMobile           CONSTANT)
    Q_PROPERTY(bool     isDebug             READ isDebug            CONSTANT)
    Q_PROPERTY(bool     isMacOS             READ isMacOS            CONSTANT)
    Q_PROPERTY(bool     isLinux             READ isLinux            CONSTANT)
    Q_PROPERTY(bool     isWindows           READ isWindows          CONSTANT)
    Q_PROPERTY(bool     isSerialAvailable   READ isSerialAvailable  CONSTANT)
    Q_PROPERTY(bool     hasTouch            READ hasTouch           CONSTANT)
    Q_PROPERTY(QString  iOSDevice           READ iOSDevice          CONSTANT)
    Q_PROPERTY(QString  fixedFontFamily     READ fixedFontFamily    CONSTANT)
    Q_PROPERTY(QString  normalFontFamily    READ normalFontFamily   CONSTANT)
    Q_PROPERTY(QString  boldFontFamily      READ boldFontFamily     CONSTANT)

    // Returns current mouse position
    Q_INVOKABLE int mouseX(void) { return QCursor::pos().x(); }
    Q_INVOKABLE int mouseY(void) { return QCursor::pos().y(); }

    // QFontMetrics::descent for default font
    Q_INVOKABLE double defaultFontDescent(int pointSize) const;

#if defined(__mobile__)
    bool    isMobile            () const { return true;  }
#else
    bool    isMobile            () const { return qgcApp()->fakeMobile(); }
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

#if defined(NO_SERIAL_LINK)
    bool    isSerialAvailable   () { return false; }
#else
    bool    isSerialAvailable   () { return true; }
#endif

#ifdef QT_DEBUG
    bool isDebug                () { return true; }
#else
    bool isDebug                () { return false; }
#endif

    bool        hasTouch            () const;
    QString     iOSDevice           () const;
    QString     fixedFontFamily     () const;
    QString     normalFontFamily    () const;
    QString     boldFontFamily      () const;
};

#endif
