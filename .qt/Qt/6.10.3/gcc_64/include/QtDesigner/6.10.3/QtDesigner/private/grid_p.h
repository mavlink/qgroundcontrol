// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of Qt Designer.  This header
// file may change from version to version without notice, or even be removed.
//
// We mean it.
//

#ifndef QDESIGNER_GRID_H
#define QDESIGNER_GRID_H

#include "shared_global_p.h"

#include <QtCore/qcompare.h>
#include <QtCore/qvariant.h>

QT_BEGIN_NAMESPACE

class QWidget;
class QPaintEvent;
class QPainter;

namespace qdesigner_internal {

// Designer grid which is able to serialize to QVariantMap
class QDESIGNER_SHARED_EXPORT Grid
{
public:
    Grid();

    bool fromVariantMap(const QVariantMap& vm);

    void addToVariantMap(QVariantMap& vm, bool forceKeys = false) const;
    QVariantMap toVariantMap(bool forceKeys = false) const;

    inline bool visible() const   { return m_visible; }
    void setVisible(bool visible) { m_visible = visible; }

    inline bool snapX() const     { return m_snapX; }
    void setSnapX(bool snap)      { m_snapX = snap; }

    inline bool snapY() const     { return m_snapY; }
    void setSnapY(bool snap)      { m_snapY = snap; }

    inline int deltaX() const     { return m_deltaX; }
    void setDeltaX(int dx)        { m_deltaX = dx; }

    inline int deltaY() const     { return m_deltaY; }
    void setDeltaY(int dy)        { m_deltaY = dy; }

    void paint(QWidget *widget, QPaintEvent *e) const;
    void paint(QPainter &p, const QWidget *widget, QPaintEvent *e) const;

    QPoint snapPoint(const QPoint &p) const;

    int widgetHandleAdjustX(int x) const;
    int widgetHandleAdjustY(int y) const;

private:
    friend bool comparesEqual(const Grid &lhs, const Grid &rhs) noexcept
    {
        return lhs.m_visible == rhs.m_visible
            && lhs.m_snapX == rhs.m_snapX && lhs.m_snapY == rhs.m_snapY
            && lhs.m_deltaX == rhs.m_deltaX && lhs.m_deltaY == rhs.m_deltaY;
    }
    Q_DECLARE_EQUALITY_COMPARABLE(Grid)

    int snapValue(int value, int grid) const;
    bool m_visible;
    bool m_snapX;
    bool m_snapY;
    int m_deltaX;
    int m_deltaY;
};
} // namespace qdesigner_internal

QT_END_NAMESPACE

#endif // QDESIGNER_GRID_H
