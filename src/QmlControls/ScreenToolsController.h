#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QLocale>
#include <QtQmlIntegration/QtQmlIntegration>

Q_DECLARE_LOGGING_CATEGORY(ScreenToolsControllerLog)

/// This Qml control is used to return screen parameters
class ScreenToolsController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
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
    Q_PROPERTY(QString  fixedFontFamily     READ fixedFontFamily    NOTIFY fontFamiliesChanged)
    Q_PROPERTY(QString  normalFontFamily    READ normalFontFamily   NOTIFY fontFamiliesChanged)

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

    static constexpr bool isAndroid()   {
#ifdef Q_OS_ANDROID
        return true;
#else
        return false;
#endif
    }

    static constexpr bool isiOS()      {
#ifdef Q_OS_IOS
        return true;
#else
        return false;
#endif
    }

    static constexpr bool isMacOS()    {
#ifdef Q_OS_MACOS
        return true;
#else
        return false;
#endif
    }

    static constexpr bool isLinux()    {
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
        return true;
#else
        return false;
#endif
    }

    static constexpr bool isWindows()  {
#ifdef Q_OS_WIN
        return true;
#else
        return false;
#endif
    }

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
    QString fixedFontFamily() const;
    QString normalFontFamily() const;
    static QString normalFontFamilyForLanguage(QLocale::Language language);

signals:
    void fontFamiliesChanged();

private:
    static QLocale::Language effectiveLanguage();
};
