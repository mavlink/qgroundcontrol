// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPLATFORMMENU_P_H
#define QPLATFORMMENU_P_H

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

#include <QtGui/private/qtguiglobal_p.h>

#include <QtCore/qnativeinterface.h>

QT_BEGIN_NAMESPACE

// ----------------- QNativeInterface -----------------

#if !defined(Q_OS_MACOS) && defined(Q_QDOC)
typedef void NSMenu;
#else
QT_END_NAMESPACE
Q_FORWARD_DECLARE_OBJC_CLASS(NSMenu);
QT_BEGIN_NAMESPACE
#endif

namespace QNativeInterface::Private {

#if defined(Q_OS_MACOS) || defined(Q_QDOC)
struct Q_GUI_EXPORT QCocoaMenu
{
    QT_DECLARE_NATIVE_INTERFACE(QCocoaMenu)
    virtual NSMenu *nsMenu() const = 0;
    virtual void setAsDockMenu() const = 0;
};

struct Q_GUI_EXPORT QCocoaMenuBar
{
    QT_DECLARE_NATIVE_INTERFACE(QCocoaMenuBar)
    virtual NSMenu *nsMenu() const = 0;
};
#endif

} // QNativeInterface::Private

QT_END_NAMESPACE

#endif // QPLATFORMMENU_P_H
