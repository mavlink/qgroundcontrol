// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSCREEN_PLATFORM_H
#define QSCREEN_PLATFORM_H

//
//  W A R N I N G
//  -------------
//
// This file is part of the native interface APIs. Usage of
// this API may make your code source and binary incompatible
// with future versions of Qt.
//

#include <QtGui/qtguiglobal.h>

#include <QtCore/qnativeinterface.h>
#include <QtGui/qguiapplication.h>

#if defined(Q_OS_WIN32)
#include <QtGui/qwindowdefs_win.h>
#endif

#if QT_CONFIG(wayland)
struct wl_output;
#endif

QT_BEGIN_NAMESPACE

namespace QNativeInterface {

#if defined(Q_OS_WIN32) || defined(Q_QDOC)
struct Q_GUI_EXPORT QWindowsScreen
{
    QT_DECLARE_NATIVE_INTERFACE(QWindowsScreen, 1, QScreen)
    virtual HMONITOR handle() const = 0;
};
#endif

#if QT_CONFIG(wayland) || defined(Q_QDOC)
struct Q_GUI_EXPORT QWaylandScreen
{
    QT_DECLARE_NATIVE_INTERFACE(QWaylandScreen, 1, QScreen)
    virtual wl_output *output() const = 0;
};
#endif

#if defined(Q_OS_ANDROID) || defined(Q_QDOC)
struct Q_GUI_EXPORT QAndroidScreen
{
    QT_DECLARE_NATIVE_INTERFACE(QAndroidScreen, 1, QScreen)
    virtual int displayId() const = 0;
};
#endif

} // namespace QNativeInterface

QT_END_NAMESPACE

#endif
