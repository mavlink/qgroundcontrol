// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QPLATFORMACCESSIBILITY_H
#define QPLATFORMACCESSIBILITY_H

//
//  W A R N I N G
//  -------------
//
// This file is part of the QPA API and is not meant to be used
// in applications. Usage of this API may make your code
// source and binary incompatible with future versions of Qt.
//

#include <QtGui/qtguiglobal.h>

#if QT_CONFIG(accessibility)

#include <QtCore/qobject.h>
#include <QtGui/qaccessible.h>

#include <optional>

QT_BEGIN_NAMESPACE


class Q_GUI_EXPORT QPlatformAccessibility
{
public:
    QPlatformAccessibility();

    virtual ~QPlatformAccessibility();
    virtual void notifyAccessibilityUpdate(QAccessibleEvent *event);
    virtual void setRootObject(QObject *o);
    virtual void initialize();
    virtual void cleanup();

    inline bool isActive() const { return m_active; }
    void setActive(bool active);
    void clearActiveNotificationState();

private:
    bool m_active = false;
    std::optional<bool> m_activeNotificationState = std::nullopt;
};

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)

#endif // QPLATFORMACCESSIBILITY_H
