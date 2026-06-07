// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QABSTRACTLAYOUTSTYLEINFO_P_H
#define QABSTRACTLAYOUTSTYLEINFO_P_H

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
#include <QtCore/qnamespace.h>
#include "qlayoutpolicy_p.h"

QT_BEGIN_NAMESPACE


class Q_GUI_EXPORT QAbstractLayoutStyleInfo {
public:

    QAbstractLayoutStyleInfo() : m_isWindow(false) {}
    virtual ~QAbstractLayoutStyleInfo() {}
    virtual qreal combinedLayoutSpacing(QLayoutPolicy::ControlTypes /*controls1*/,
                                        QLayoutPolicy::ControlTypes /*controls2*/, Qt::Orientation /*orientation*/) const {
        return -1;
    }

    virtual qreal perItemSpacing(QLayoutPolicy::ControlType /*control1*/,
                                 QLayoutPolicy::ControlType /*control2*/,
                                 Qt::Orientation /*orientation*/) const {
        return -1;
    }

    virtual qreal spacing(Qt::Orientation orientation) const = 0;

    virtual bool hasChangedCore() const { return false; }   // ### Remove when usage is gone from subclasses

    virtual void invalidate() { }

    virtual qreal windowMargin(Qt::Orientation orientation) const = 0;

    bool isWindow() const {
        return m_isWindow;
    }

protected:
    unsigned m_isWindow : 1;
    mutable unsigned m_hSpacingState: 2;
    mutable unsigned m_vSpacingState: 2;
    mutable qreal m_spacing[2];
};

QT_END_NAMESPACE

#endif // QABSTRACTLAYOUTSTYLEINFO_P_H
