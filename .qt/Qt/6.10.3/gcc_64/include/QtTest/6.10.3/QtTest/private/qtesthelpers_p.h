// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTESTHELPERS_P_H
#define QTESTHELPERS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/QFile>
#include <QtCore/QString>
#include <QtCore/QChar>
#include <QtCore/QPoint>
#include <QtCore/private/qglobal_p.h>

#ifdef QT_GUI_LIB
#include <QtGui/QGuiApplication>
#include <QtGui/QScreen>
#include <QtGui/QWindow>
#endif

#ifdef QT_WIDGETS_LIB
#include <QtWidgets/QWidget>
#endif

#ifdef QT_NETWORK_LIB
#if QT_CONFIG(ssl)
#include <QtCore/qoperatingsystemversion.h>
#include <QtCore/qsystemdetection.h>
#include <QtNetwork/qsslsocket.h>
#endif // QT_CONFIG(ssl)
#endif // QT_NETWORK_LIB

QT_BEGIN_NAMESPACE

namespace QTestPrivate {

static inline bool canHandleUnicodeFileNames()
{
#if defined(Q_OS_WIN)
    return true;
#else
    // Check for UTF-8 by converting the Euro symbol (see tst_utf8)
    return QFile::encodeName(QString(QChar(0x20AC))) == QByteArrayLiteral("\342\202\254");
#endif
}

#ifdef QT_WIDGETS_LIB
static inline void centerOnScreen(QWidget *w, const QSize &size)
{
    const QPoint offset = QPoint(size.width() / 2, size.height() / 2);
    w->move(QGuiApplication::primaryScreen()->availableGeometry().center() - offset);
}

static inline void centerOnScreen(QWidget *w)
{
    centerOnScreen(w, w->geometry().size());
}

/*! \internal

    Make a widget frameless to prevent size constraints of title bars from interfering (Windows).
*/
static inline void setFrameless(QWidget *w)
{
    Qt::WindowFlags flags = w->windowFlags();
    flags |= Qt::FramelessWindowHint;
    flags &= ~(Qt::WindowTitleHint | Qt::WindowSystemMenuHint
             | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);
    w->setWindowFlags(flags);
}

static inline void androidCompatibleShow(QWidget *widget)
{
    // On Android QWidget::show() shows the widget maximized, so if we need
    // to move or resize the widget, we need to explicitly call
    // QWidget::setVisible(true) instead, because that's what show() actually
    // does on desktop platforms.
#ifdef Q_OS_ANDROID
    widget->setVisible(true);
#else
    widget->show();
#endif
}
#endif // QT_WIDGETS_LIB

#ifdef QT_GUI_LIB
bool ensurePositionTopLeft(QWindow *window)
{
    // Wayland: QQuickWindow::setPos() and QQuickWindow::setFramePosition() doesn't work.
    if (!QGuiApplication::platformName().compare(QLatin1String("wayland"), Qt::CaseInsensitive))
        return true;

    const QPoint availableTopLeft = QGuiApplication::primaryScreen()->availableGeometry().topLeft();
    window->setFramePosition(availableTopLeft);
#ifdef Q_OS_ANDROID
    return true; // Android QPA handles the position change synchronously
#endif
    bool positionCorrect = true;

    if (!window->flags().testFlag(Qt::FramelessWindowHint))
        positionCorrect = QTest::qWaitFor([&]{ return window->framePosition() != window->position() ;});

    const bool positionUpdated = QTest::qWaitFor([&]{ return window->framePosition() == availableTopLeft ;});
    if (!positionUpdated)
        positionCorrect = false;

    return positionCorrect;
}
#endif

#ifdef QT_NETWORK_LIB
inline bool isSecureTransportBlockingTest()
{
#ifdef Q_OS_MACOS
#if QT_CONFIG(ssl)
    if (QSslSocket::activeBackend() == QLatin1String("securetransport")) {
#if QT_MACOS_IOS_PLATFORM_SDK_EQUAL_OR_ABOVE(150000, 180000)
        // Starting from macOS 15 our temporary keychain is ignored.
        // We have to use kSecImportToMemoryOnly/kCFBooleanTrue key/value
        // instead. This way we don't have to use QT_SSL_USE_TEMPORARY_KEYCHAIN anymore.
        return false;
#else
        if (QOperatingSystemVersion::current() >= QOperatingSystemVersion::MacOSSequoia) {
            // We were built with SDK below 15, and running on/above 15, but file-based
            // keychains are not working anymore on macOS 15, blocking the test execution.
            return true;
        }
#endif // Platform SDK.
    }
#endif // QT_CONFIG(ssl)
#endif // Q_OS_MACOS
    return false;
}
#endif // QT_NETWORK_LIB

} // namespace QTestPrivate

QT_END_NAMESPACE

#endif // QTESTHELPERS_P_H
