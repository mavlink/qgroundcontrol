/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Gus Grubba <gus@auterion.com>

#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtQmlIntegration/QtQmlIntegration>

Q_DECLARE_LOGGING_CATEGORY(ScreenToolsControllerLog)

/// This Qml control is used to return screen parameters
class ScreenToolsController : public QObject
{
    Q_OBJECT
    // QML_ELEMENT
    // TODO: Q_NAMESPACE
    Q_PROPERTY(bool     isAndroid           READ isAndroid          CONSTANT)
    Q_PROPERTY(bool     isiOS               READ isiOS              CONSTANT)
    Q_PROPERTY(bool     isMobile            READ isMobile           CONSTANT)
    Q_PROPERTY(bool     fakeMobile          READ fakeMobile         CONSTANT)
    Q_PROPERTY(bool     isDebug             READ isDebug            CONSTANT)
    Q_PROPERTY(bool     isMacOS             READ isMacOS            CONSTANT)
    Q_PROPERTY(bool     isLinux             READ isLinux            CONSTANT)
    Q_PROPERTY(bool     isWindows           READ isWindows          CONSTANT)
    Q_PROPERTY(bool     isSerialAvailable   READ isSerialAvailable  CONSTANT)
    Q_PROPERTY(bool     hasTouch            READ hasTouch           CONSTANT)
    Q_PROPERTY(QString  iOSDevice           READ iOSDevice          CONSTANT)
    Q_PROPERTY(QString  fixedFontFamily     READ fixedFontFamily    CONSTANT)
    Q_PROPERTY(QString  normalFontFamily    READ normalFontFamily   CONSTANT)

public:
    explicit ScreenToolsController(QObject *parent = nullptr);
    ~ScreenToolsController();

    /// Returns current mouse position
    Q_INVOKABLE static int mouseX();
    Q_INVOKABLE static int mouseY();

    // QFontMetrics::descent for default font
    Q_INVOKABLE static double defaultFontDescent(int pointSize);

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    static bool isMobile() { return true;  }
    static bool fakeMobile() { return false; }
#else
    static bool isMobile() { return fakeMobile(); }
    static bool fakeMobile();
#endif

#if defined (Q_OS_ANDROID)
    static bool isAndroid() { return true;  }
    static bool isiOS() { return false; }
    static bool isLinux() { return false; }
    static bool isMacOS() { return false; }
    static bool isWindows() { return false; }
#elif defined(Q_OS_IOS)
    static bool isAndroid() { return false; }
    static bool isiOS() { return true; }
    static bool isLinux() { return false; }
    static bool isMacOS() { return false; }
    static bool isWindows() { return false; }
#elif defined(Q_OS_MACOS)
    static bool isAndroid() { return false; }
    static bool isiOS() { return false; }
    static bool isLinux() { return false; }
    static bool isMacOS() { return true; }
    static bool isWindows() { return false; }
#elif defined(Q_OS_LINUX)
    static bool isAndroid() { return false; }
    static bool isiOS() { return false; }
    static bool isLinux() { return true; }
    static bool isMacOS() { return false; }
    static bool isWindows() { return false; }
#elif defined(Q_OS_WIN)
    static bool isAndroid() { return false; }
    static bool isiOS() { return false; }
    static bool isLinux() { return false; }
    static bool isMacOS() { return false; }
    static bool isWindows() { return true; }
#else
    static bool isAndroid() { return false; }
    static bool isiOS() { return false; }
    static bool isLinux() { return false; }
    static bool isMacOS() { return false; }
    static bool isWindows() { return false; }
#endif

#if defined(QGC_NO_SERIAL_LINK)
    static bool isSerialAvailable() { return false; }
#else
    static bool isSerialAvailable() { return true; }
#endif

#ifdef QT_DEBUG
    static bool isDebug() { return true; }
#else
    static bool isDebug() { return false; }
#endif

    static bool hasTouch();
    static QString iOSDevice();
    static QString fixedFontFamily();
    static QString normalFontFamily();
};
